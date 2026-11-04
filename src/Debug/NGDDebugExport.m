// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDDebugExport.m
//  NaiwaGuo
//
//  只处理不可变采集结果的序列化、类元数据导出与 ZIP 打包。
//
//  两条硬规矩：
//  · 收集到的文件数有上限，超了就截断并在 manifest 标记，不把失败留给打包那一步。
//  · 打包失败时保留工作目录并把路径回传，错误信息里带上 underlying —— 唯一能
//    解释失败原因的东西不能跟着失败一起消失。
//

#import "NGDDebugExport.h"
#import "NGDAudioProbe.h"
#import "NGDClassDump.h"
#import "NGDKeys.h"
#import "NGDZipWriter.h"

NSString *const NGDDebugExportWorkingDirectoryKey = @"NGDDebugExportWorkingDirectory";

static NSString *const NGDExportErrorDomain = @"com.naiwaguo.debug-export";

// 单个导出包的文件数上限。页面导出约 200 个、音频导出约 15 个，
// 这道闸门只用来防止将来某处再写出失控的循环。
static const NSUInteger kDKMaxExportFiles = 8000;

@implementation NGDDebugExportResult
@end

#pragma mark - 写入汇集器

// 把「文件清单 + 错误清单 + 关键失败标记 + 截断计数」收在一起，
// 免得每个写入函数都拖着四个 out 参数。
@interface NGDExportSink : NSObject
@property (nonatomic, copy) NSString *rootDir;
@property (nonatomic, strong) NSMutableArray<NSString *> *files;
@property (nonatomic, strong) NSMutableArray<NSDictionary *> *errors;
@property (nonatomic, assign) BOOL criticalFailed;
@property (nonatomic, assign) NSUInteger droppedFiles;
@end

@implementation NGDExportSink

- (instancetype)initWithRootDir:(NSString *)rootDir {
    if ((self = [super init])) {
        _rootDir = [rootDir copy];
        _files = [NSMutableArray array];
        _errors = [NSMutableArray array];
    }
    return self;
}

@end

#pragma mark - 文件工具

static NSError *NGDMakeExportError(NSInteger code, NSString *message, NSError *underlying) {
    NSMutableDictionary *info = [NSMutableDictionary dictionaryWithObject:message ?: @"导出失败"
                                                                   forKey:NSLocalizedDescriptionKey];
    if (underlying) info[NSUnderlyingErrorKey] = underlying;
    return [NSError errorWithDomain:NGDExportErrorDomain code:code userInfo:info];
}

static NSString *NGDSafeFileName(NSString *name) {
    if (!name.length) return @"Unknown";
    NSCharacterSet *bad = [NSCharacterSet characterSetWithCharactersInString:@"/\\?%*|\"<>:"];
    NSString *safe = [[name componentsSeparatedByCharactersInSet:bad] componentsJoinedByString:@"_"];
    return safe.length ? safe : @"Unknown";
}

static NSString *NGDUniqueFileStem(NSString *name) {
    NSString *safe = NGDSafeFileName(name);
    if (safe.length > 160) {
        NSRange range = [safe rangeOfComposedCharacterSequencesForRange:NSMakeRange(0, 160)];
        safe = [safe substringWithRange:range];
    }
    NSData *utf8 = [(name ?: @"") dataUsingEncoding:NSUTF8StringEncoding] ?: NSData.data;
    uint64_t hash = 1469598103934665603ULL;
    const uint8_t *bytes = utf8.bytes;
    for (NSUInteger i = 0; i < utf8.length; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return [NSString stringWithFormat:@"%@-%08llx", safe, (unsigned long long)(hash & 0xffffffffULL)];
}

static void NGDRecordError(NGDExportSink *sink,
                          NSString *relativePath,
                          NSString *stage,
                          NSError *error,
                          BOOL critical) {
    [sink.errors addObject:@{
        @"path": relativePath ?: @"",
        @"stage": stage ?: @"write",
        @"critical": @(critical),
        @"domain": error.domain ?: @"",
        @"code": @(error.code),
        @"message": error.localizedDescription ?: @"unknown error",
    }];
}

static BOOL NGDWriteData(NGDExportSink *sink, NSString *relativePath, NSData *data, BOOL critical) {
    if (sink.files.count >= kDKMaxExportFiles) {
        sink.droppedFiles++;
        return NO;
    }

    NSString *path = [sink.rootDir stringByAppendingPathComponent:relativePath];
    NSError *writeError = nil;
    NSString *directory = path.stringByDeletingLastPathComponent;
    BOOL ok = data && [NSFileManager.defaultManager createDirectoryAtPath:directory
                                              withIntermediateDirectories:YES
                                                               attributes:nil
                                                                    error:&writeError];
    if (ok) ok = [data writeToFile:path options:NSDataWritingAtomic error:&writeError];
    if (ok) {
        [sink.files addObject:path];
        return YES;
    }
    if (!writeError) writeError = NGDMakeExportError(10, @"没有可写入的数据", nil);
    NGDRecordError(sink, relativePath, @"write", writeError, critical);
    if (critical) sink.criticalFailed = YES;
    return NO;
}

static BOOL NGDWriteString(NGDExportSink *sink, NSString *relativePath, NSString *string, BOOL critical) {
    return NGDWriteData(sink, relativePath, [(string ?: @"") dataUsingEncoding:NSUTF8StringEncoding], critical);
}

static BOOL NGDWriteJSON(NGDExportSink *sink, NSString *relativePath, id object, BOOL critical) {
    NSError *jsonError = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:object ?: @{}
                                                   options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys
                                                     error:&jsonError];
    if (!data) {
        NGDRecordError(sink, relativePath, @"json",
                      jsonError ?: NGDMakeExportError(11, @"JSON 序列化失败", nil), critical);
        if (critical) sink.criticalFailed = YES;
        return NO;
    }
    return NGDWriteData(sink, relativePath, data, critical);
}

static NSString *NGDModeName(NGDDebugExportMode mode) {
    return mode == NGDDebugExportModeAudio ? @"audio" : @"page";
}

#pragma mark - README

static NSString *NGDReadme(NGDDebugExportContext *context, NGDDebugExportMode mode) {
    NSMutableString *text = [NSMutableString string];
    [text appendString:@"NaiwaGuo Debug Export\n"];
    [text appendFormat:@"Generated: %@\n", context.metadata[@"generatedAt"] ?: @""];
    [text appendFormat:@"NaiwaGuo: %@\n", NGD_VERSION];
    [text appendFormat:@"Bundle: %@\n", context.metadata[@"bundleIdentifier"] ?: @""];
    [text appendFormat:@"App: %@ %@ (%@)\n",
     context.metadata[@"bundleName"] ?: @"", context.metadata[@"appVersion"] ?: @"",
     context.metadata[@"buildVersion"] ?: @""];
    [text appendFormat:@"System: %@ %@\n", context.metadata[@"systemName"] ?: @"",
     context.metadata[@"systemVersion"] ?: @""];
    [text appendFormat:@"Mode: %@\n\n", NGDModeName(mode)];

    if (mode == NGDDebugExportModeAudio) {
        NGDAudioProbeCapture *capture = context.audioCapture;
        [text appendString:@"Contents:\n"];
        [text appendString:@"- audio/session.json        采样前后的 AVAudioSession 快照\n"];
        [text appendString:@"- audio/backends.json       输出单元/队列登记表与只读旁路状态\n"];
        [text appendString:@"- audio/timeline.jsonl      实时层与 AVFoundation 事件时间线\n"];
        [text appendString:@"- audio/signal.jsonl        30 Hz 的 RMS/峰值/频谱带时间序列\n"];
        [text appendString:@"- audio/media-snapshots.json 0/2.5/5 秒的播放状态\n"];
        [text appendString:@"- audio/glass-target.json   玻璃底栏几何\n"];
        [text appendString:@"- audio/diagnostics.json    符号覆盖、丢帧与自检结果\n"];
        [text appendString:@"- audio/pcm/                单声道 Float32 WAV 与各自的格式统计\n"];
        [text appendString:@"- probe/tabbar.txt          底栏现场\n- ui/screenshot.png\n\n"];
        [text appendFormat:@"采样阶段固定为 %.0f 秒稳定、%.0f 秒 PCM、%.0f 秒收尾。探针不会改变播放状态。\n",
         NGDAudioProbeWarmupSeconds, NGDAudioProbeRecordSeconds,
         NGDAudioProbeTotalSeconds - NGDAudioProbeWarmupSeconds - NGDAudioProbeRecordSeconds];
        [text appendFormat:@"用户标记: %@\n实际推断: %@\n", capture.declaredState ?: @"unknown",
         capture.actualState ?: @"unknown"];
    } else {
        [text appendString:@"Contents:\n- page/: UI 主线程快照与本页类\n"];
        [text appendString:@"- probe/tabbar.txt: 液态玻璃底栏现场\n- ui/: 目标窗口截图\n"];
    }
    return text;
}

#pragma mark - 页面内容

static void NGDWritePageClasses(NGDExportSink *sink, NSArray<NSString *> *classNames) {
    for (NSString *name in [classNames sortedArrayUsingSelector:@selector(compare:)]) {
        @autoreleasepool {
            @try {
                Class cls = NSClassFromString(name);
                NSString *header = cls ? NGDClassDumpHeaderForClass(cls) : nil;
                if (!header.length) continue;
                NSString *relative = [@"page/classes" stringByAppendingPathComponent:
                                      [NGDUniqueFileStem(name) stringByAppendingString:@".h"]];
                NGDWriteString(sink, relative, header, NO);
            } @catch (NSException *exception) {
                NGDRecordError(sink, name, @"page-class",
                              NGDMakeExportError(20, exception.reason ?: @"类头文件导出异常", nil), NO);
            }
        }
    }
}

static void NGDWritePageFiles(NGDExportSink *sink,
                             NGDDebugExportContext *context,
                             void (^progress)(NSString *text)) {
    if (progress) progress(@"写入页面结构...");
    NGDWriteJSON(sink, @"page/windows.json", context.windowsJSON ?: @[], NO);
    NGDWriteString(sink, @"page/view-tree.txt", context.viewTreeText, NO);
    NGDWriteJSON(sink, @"page/view-tree.json", context.viewTreeJSON ?: @[], NO);
    NGDWriteJSON(sink, @"page/selected-view.json", context.selectedViewJSON ?: @{}, NO);
    NGDWriteString(sink, @"page/view-controllers.txt", context.viewControllersText, NO);
    NGDWriteJSON(sink, @"page/layers.json", context.layersJSON ?: @[], NO);
    if (progress) progress(@"导出本页类头文件...");
    NGDWritePageClasses(sink, context.pageClassNames ?: @[]);
}

#pragma mark - 音频内容

static void NGDWriteAudioFiles(NGDExportSink *sink,
                              NGDAudioProbeCapture *capture,
                              void (^progress)(NSString *text)) {
    if (progress) progress(@"写入音频采集结果...");
    // 这九项是音频导出的全部意义所在，任何一项写不出去都该让导出失败，
    // 而不是交出一个看起来成功、其实缺内容的包。
    NGDWriteString(sink, @"probe/audio-summary.txt", capture.summaryText, YES);
    NGDWriteJSON(sink, @"audio/session.json", capture.sessionJSON ?: @{}, YES);
    NGDWriteJSON(sink, @"audio/backends.json", capture.backendsJSON ?: @[], YES);
    NGDWriteString(sink, @"audio/timeline.jsonl", capture.timelineJSONL, YES);
    NGDWriteString(sink, @"audio/signal.jsonl", capture.signalJSONL, YES);
    NGDWriteJSON(sink, @"audio/media-snapshots.json", capture.mediaSnapshotsJSON ?: @[], YES);
    NGDWriteJSON(sink, @"audio/glass-target.json", capture.glassTargetJSON ?: @{}, YES);
    NGDWriteJSON(sink, @"audio/diagnostics.json", capture.diagnosticsJSON ?: @{}, YES);

    for (NSDictionary *pcm in capture.pcmFiles ?: @[]) {
        NSString *fileName = NGDSafeFileName([pcm[@"fileName"] description]);
        NSData *data = [pcm[@"data"] isKindOfClass:NSData.class] ? pcm[@"data"] : nil;
        NSDictionary *metadata = [pcm[@"metadata"] isKindOfClass:NSDictionary.class] ? pcm[@"metadata"] : @{};
        NGDWriteData(sink, [@"audio/pcm" stringByAppendingPathComponent:fileName], data, NO);
        NSString *jsonName = [[fileName stringByDeletingPathExtension] stringByAppendingPathExtension:@"json"];
        NGDWriteJSON(sink, [@"audio/pcm" stringByAppendingPathComponent:jsonName], metadata, NO);
    }
}

#pragma mark - 导出入口

NGDDebugExportResult *NGDDebugCreateExport(NGDDebugExportContext *context,
                                         NGDDebugExportMode mode,
                                         void (^progress)(NSString *text),
                                         NSError **error) {
    if (!context || (mode == NGDDebugExportModeAudio && !context.audioCapture)) {
        if (error) *error = NGDMakeExportError(1, @"导出上下文不完整", nil);
        return nil;
    }

    NSString *state = mode == NGDDebugExportModeAudio ? (context.audioCapture.declaredState ?: @"unknown") : @"";
    NSString *suffix = state.length ? [NSString stringWithFormat:@"-%@", NGDSafeFileName(state)] : @"";
    NSString *rootName = [NSString stringWithFormat:@"NaiwaGuo-Debug-%@%@-%@-%lld",
                          NGDModeName(mode), suffix, NSBundle.mainBundle.bundleIdentifier ?: @"Eggplant",
                          (long long)NSDate.date.timeIntervalSince1970];
    NSString *temporary = NSTemporaryDirectory();
    NSString *rootDir = [temporary stringByAppendingPathComponent:rootName];
    NSString *zipPath = [temporary stringByAppendingPathComponent:[rootName stringByAppendingString:@".zip"]];
    NSFileManager *fm = NSFileManager.defaultManager;
    [fm removeItemAtPath:rootDir error:nil];
    [fm removeItemAtPath:zipPath error:nil];

    NSError *directoryError = nil;
    if (![fm createDirectoryAtPath:rootDir withIntermediateDirectories:YES attributes:nil error:&directoryError]) {
        if (error) *error = NGDMakeExportError(2, @"无法创建导出目录", directoryError);
        return nil;
    }

    NGDExportSink *sink = [[NGDExportSink alloc] initWithRootDir:rootDir];
    NGDWriteString(sink, @"README.txt", NGDReadme(context, mode), mode == NGDDebugExportModeAudio);
    NGDWriteData(sink, @"ui/screenshot.png", context.screenshotPNG, NO);
    NGDWriteString(sink, @"probe/tabbar.txt", context.probeText, NO);

    if (mode == NGDDebugExportModeAudio) {
        NGDWriteAudioFiles(sink, context.audioCapture, progress);
    } else {
        NGDWritePageFiles(sink, context, progress);
    }

    NSDictionary *manifest = @{
        @"schemaVersion": mode == NGDDebugExportModeAudio ? @"naiwaguo.audio-probe.v2" : @"naiwaguo.debug.v2",
        @"mode": NGDModeName(mode),
        @"naiwaguoVersion": NGD_VERSION,
        @"device": context.metadata ?: @{},
        @"declaredState": context.audioCapture.declaredState ?: @"not-applicable",
        @"actualState": context.audioCapture.actualState ?: @"not-applicable",
        @"sampling": mode == NGDDebugExportModeAudio ? @{
            @"totalSeconds": @(NGDAudioProbeTotalSeconds),
            @"warmupSeconds": @(NGDAudioProbeWarmupSeconds),
            @"recordSeconds": @(NGDAudioProbeRecordSeconds),
            @"snapshotsAtSeconds": @[ @0, @2.5, @5 ],
        } : @{},
        @"integrity": @{
            @"complete": @(!sink.criticalFailed && sink.droppedFiles == 0),
            @"status": sink.criticalFailed ? @"critical-failure"
                     : (sink.droppedFiles ? @"truncated"
                     : (sink.errors.count ? @"complete-with-recoverable-errors" : @"complete")),
            @"fileCount": @(sink.files.count),
            @"fileLimit": @(kDKMaxExportFiles),
            @"truncated": @(sink.droppedFiles > 0),
            @"droppedFiles": @(sink.droppedFiles),
            @"errorCount": @(sink.errors.count),
            @"errors": sink.errors,
        },
    };
    NGDWriteJSON(sink, @"manifest.json", manifest, mode == NGDDebugExportModeAudio);

    if (sink.criticalFailed) {
        if (error) {
            *error = [NSError errorWithDomain:NGDExportErrorDomain code:3 userInfo:@{
                NSLocalizedDescriptionKey: @"关键文件写入失败，工作目录已保留",
                NGDDebugExportWorkingDirectoryKey: rootDir,
            }];
        }
        return nil;
    }

    if (progress) progress(@"压缩 zip...");
    NSError *zipError = nil;
    BOOL zipped = sink.files.count > 0 && [NGDZipWriter createZipAtPath:zipPath
                                                              rootDir:rootDir
                                                                files:sink.files
                                                             progress:nil
                                                                error:&zipError];
    NSError *attributeError = nil;
    NSDictionary *attributes = zipped ? [fm attributesOfItemAtPath:zipPath error:&attributeError] : nil;
    NSNumber *zipSize = attributes[NSFileSize];
    if (!zipped || zipError || attributeError || zipSize.unsignedLongLongValue <= 22) {
        NSError *underlying = zipError ?: attributeError;
        NSString *message = underlying
            ? [NSString stringWithFormat:@"ZIP 生成失败（%lu 个待打包文件）：%@",
               (unsigned long)sink.files.count, underlying.localizedDescription]
            : [NSString stringWithFormat:@"ZIP 未生成或内容为空（%lu 个待打包文件）",
               (unsigned long)sink.files.count];
        [fm removeItemAtPath:zipPath error:nil];
        // 工作目录留着：它是唯一能解释这次失败的现场。
        NSMutableDictionary *info = [NSMutableDictionary dictionaryWithDictionary:@{
            NSLocalizedDescriptionKey: message,
            NGDDebugExportWorkingDirectoryKey: rootDir,
        }];
        if (underlying) info[NSUnderlyingErrorKey] = underlying;
        if (error) *error = [NSError errorWithDomain:NGDExportErrorDomain code:4 userInfo:info];
        return nil;
    }

    NGDDebugExportResult *result = [NGDDebugExportResult new];
    result.zipURL = [NSURL fileURLWithPath:zipPath];
    result.workingDirectoryURL = [NSURL fileURLWithPath:rootDir isDirectory:YES];
    return result;
}

void NGDDebugCleanupExport(NGDDebugExportResult *result) {
    if (!result) return;
    NSFileManager *fm = NSFileManager.defaultManager;
    if (result.workingDirectoryURL.isFileURL) [fm removeItemAtURL:result.workingDirectoryURL error:nil];
    if (result.zipURL.isFileURL) [fm removeItemAtURL:result.zipURL error:nil];
}
