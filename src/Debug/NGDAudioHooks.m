// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDAudioHooks.xm
//  NaiwaGuo
//
//  Apple 高层音频/播放器事件。所有方法保持原调用与返回值，
//  不安装 AVAudioNode tap，也不主动改变播放、会话或路由。
//
//  只保留四类观测点：会话状态（决定静音与中断语义）、AVAudioEngine 启停
//  （判断是否存在第二条音频链路）、AVPlayer 的播放与静音（回答「静音时 PCM
//  是否已归零」）、进程外播放器的身份（回答「图文 BGM 走的是哪条」）。
//  真正的取样在 NGDAudioTap。
//
//  为什么需要第四类：0.5.2-beta4 实测图文类型 feed 的 BGM 在响，而两只 RemoteIO
//  输出单元照常回调、送出的却是数字静音（三秒 rms 全零）。进程内的 AudioUnit /
//  AudioQueue / AVAudioEngine 全部排除，那条音频只可能由 mediaserverd 代为渲染，
//  即 AVPlayer 或 AVAudioPlayer。这两族的入口在此登记，一次导出即可定位。
//

#import "NGDAudioProbe.h"
#import "NGDAudioHooks.h"
#import <AVFAudio/AVFAudio.h>
#import <AVFoundation/AVFoundation.h>
#import <dlfcn.h>
#import <math.h>
#import <objc/runtime.h>

static NSDictionary *NGDAudioHookError(NSError **outError) {
    NSError *error = outError ? *outError : nil;
    return error ? @{ @"domain": error.domain ?: @"", @"code": @(error.code) } : @{};
}

static void NGDRecordSessionChange(NSString *name,
                                  AVAudioSession *session,
                                  BOOL result,
                                  NSError **outError,
                                  NSDictionary *parameters) {
    if (!NGDAudioProbeIsEnabled()) return;
    NGDAudioProbeRecordObjectDetails(name, session, @{
        @"result": @(result),
        @"error": NGDAudioHookError(outError),
        @"parameters": parameters ?: @{},
        @"sessionAfterCall": NGDAudioProbeSessionSnapshot() ?: @{},
    });
}

static NSDictionary *NGDPlayerState(AVPlayer *player) {
    if (!player) return @{};
    double seconds = CMTimeGetSeconds(player.currentTime);
    return @{
        @"rate": @(player.rate), @"volume": @(player.volume), @"muted": @(player.muted),
        @"timeControlStatus": @(player.timeControlStatus),
        @"currentTime": @(isfinite(seconds) ? seconds : -1),
        @"hasCurrentItem": @(player.currentItem != nil),
    };
}

// 只取资源的身份与音轨构成，不读正文、不读 URL 的查询串。
static NSDictionary *NGDAssetDescription(AVPlayerItem *item) {
    AVAsset *asset = item.asset;
    if (!asset) return @{};
    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    info[@"assetClass"] = NSStringFromClass(asset.class);
    if ([asset isKindOfClass:AVURLAsset.class]) {
        NSURL *url = ((AVURLAsset *)asset).URL;
        info[@"scheme"] = url.scheme ?: @"";
        info[@"pathExtension"] = url.pathExtension ?: @"";
        info[@"isFileURL"] = @(url.isFileURL);
    }
    info[@"audioTracks"] = @([asset tracksWithMediaType:AVMediaTypeAudio].count);
    info[@"videoTracks"] = @([asset tracksWithMediaType:AVMediaTypeVideo].count);
    info[@"hasAudioMix"] = @(item.audioMix != nil);
    return info;
}

static NSDictionary *NGDAudioPlayerState(AVAudioPlayer *player) {
    if (!player) return @{};
    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    info[@"playerClass"] = NSStringFromClass(player.class);
    info[@"playing"] = @(player.isPlaying);
    info[@"volume"] = @(player.volume);
    info[@"rate"] = @(player.rate);
    info[@"numberOfLoops"] = @(player.numberOfLoops);
    info[@"duration"] = @(player.duration);
    info[@"currentTime"] = @(player.currentTime);
    info[@"numberOfChannels"] = @(player.numberOfChannels);
    info[@"meteringEnabled"] = @(player.isMeteringEnabled);
    info[@"scheme"] = player.url.scheme ?: @"";
    info[@"pathExtension"] = player.url.pathExtension ?: @"";
    return info;
}

#pragma mark - 覆盖自检

// IMP 落在 NaiwaGuo 自己的镜像里，就说明这个方法确实被我们接管了。
static NSMutableDictionary<NSString *,NSValue *> *NGDExpectedAudioIMPs;
NSArray<NSDictionary *> *NGDAudioHooksCoverageJSON(void) {
    NSArray<NSArray *> *table = @[
        @[ @"AVAudioSession", @"setActive:error:", @"setActive:withOptions:error:",
           @"setCategory:mode:options:error:", @"setCategory:error:" ],
        @[ @"AVAudioEngine", @"init", @"startAndReturnError:", @"stop" ],
        @[ @"AVPlayer", @"init", @"initWithURL:", @"initWithPlayerItem:", @"play", @"pause",
           @"setVolume:", @"setMuted:", @"setRate:", @"replaceCurrentItemWithPlayerItem:" ],
        @[ @"AVQueuePlayer", @"initWithItems:" ],
        @[ @"AVPlayerItem", @"setAudioMix:" ],
        @[ @"AVAudioPlayer", @"initWithContentsOfURL:error:", @"initWithData:error:",
           @"play", @"playAtTime:", @"pause", @"stop", @"setVolume:" ],
    ];

    NSMutableArray<NSDictionary *> *result = [NSMutableArray array];
    for (NSArray *row in table) {
        NSString *className = row.firstObject;
        Class cls = NSClassFromString(className);
        NSMutableArray<NSDictionary *> *methods = [NSMutableArray array];
        for (NSUInteger i = 1; i < row.count; i++) {
            NSString *name = row[i];
            SEL selector = NSSelectorFromString(name);
            Method method = cls ? class_getInstanceMethod(cls, selector) : NULL;
            [methods addObject:@{
                @"selector": name,
                @"exists": @(method != NULL),
                @"hooked": @(method && (NGDExpectedAudioIMPs[[className stringByAppendingFormat:@".%@",name]].pointerValue == method_getImplementation(method))),
            }];
        }
        [result addObject:@{ @"class": className, @"classFound": @(cls != nil), @"methods": methods }];
    }
    return result;
}

#pragma mark - 会话

@interface AVAudioSession (NaiwaGuoAudioObservation)
- (BOOL)ngd_setActive:(BOOL)active error:(NSError **)outError;
- (BOOL)ngd_setActive:(BOOL)active withOptions:(AVAudioSessionSetActiveOptions)options error:(NSError **)outError;
- (BOOL)ngd_setCategory:(AVAudioSessionCategory)category
               mode:(AVAudioSessionMode)mode
            options:(AVAudioSessionCategoryOptions)options
              error:(NSError **)outError;
- (BOOL)ngd_setCategory:(AVAudioSessionCategory)category error:(NSError **)outError;
@end
@implementation AVAudioSession (NaiwaGuoAudioObservation)
- (BOOL)ngd_setActive:(BOOL)active error:(NSError **)outError {
    BOOL result = [self ngd_setActive:active error:outError];
    NGDRecordSessionChange(@"AVAudioSession.setActive", self, result, outError, @{ @"active": @(active) });
    return result;
}
- (BOOL)ngd_setActive:(BOOL)active withOptions:(AVAudioSessionSetActiveOptions)options error:(NSError **)outError {
    BOOL result = [self ngd_setActive:active withOptions:options error:outError];
    NGDRecordSessionChange(@"AVAudioSession.setActiveWithOptions", self, result, outError,
                          @{ @"active": @(active), @"options": @(options) });
    return result;
}
- (BOOL)ngd_setCategory:(AVAudioSessionCategory)category
               mode:(AVAudioSessionMode)mode
            options:(AVAudioSessionCategoryOptions)options
              error:(NSError **)outError {
    BOOL result = [self ngd_setCategory:category mode:mode options:options error:outError];
    NGDRecordSessionChange(@"AVAudioSession.setCategoryModeOptions", self, result, outError,
                          @{ @"category": category ?: @"", @"mode": mode ?: @"", @"options": @(options) });
    return result;
}
- (BOOL)ngd_setCategory:(AVAudioSessionCategory)category error:(NSError **)outError {
    BOOL result = [self ngd_setCategory:category error:outError];
    NGDRecordSessionChange(@"AVAudioSession.setCategory", self, result, outError,
                          @{ @"category": category ?: @"" });
    return result;
}
@end

@interface AVAudioEngine (NaiwaGuoAudioObservation)
- (instancetype)ngd_init __attribute__((objc_method_family(init)));
- (BOOL)ngd_startAndReturnError:(NSError **)outError;
- (void)ngd_stop;
@end
@implementation AVAudioEngine (NaiwaGuoAudioObservation)
- (instancetype)ngd_init __attribute__((objc_method_family(init))) {
    id engine = [self ngd_init];
    if (engine && NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVAudioEngine.init", engine, @{});
    }
    return engine;
}
- (BOOL)ngd_startAndReturnError:(NSError **)outError {
    BOOL result = [self ngd_startAndReturnError:outError];
    if (NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVAudioEngine.start", self, @{
            @"result": @(result), @"error": NGDAudioHookError(outError), @"running": @(self.running),
        });
    }
    return result;
}
- (void)ngd_stop {
    [self ngd_stop];
    if (NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVAudioEngine.stop", self, @{ @"running": @(self.running) });
    }
}
@end

@interface AVPlayer (NaiwaGuoAudioObservation)
- (instancetype)ngd_init __attribute__((objc_method_family(init)));
- (instancetype)ngd_initWithURL:(NSURL *)url __attribute__((objc_method_family(init)));
- (instancetype)ngd_initWithPlayerItem:(AVPlayerItem *)item __attribute__((objc_method_family(init)));
- (void)ngd_play;
- (void)ngd_pause;
- (void)ngd_setVolume:(float)volume;
- (void)ngd_setMuted:(BOOL)muted;
- (void)ngd_setRate:(float)rate;
- (void)ngd_replaceCurrentItemWithPlayerItem:(AVPlayerItem *)item;
@end
@implementation AVPlayer (NaiwaGuoAudioObservation)
- (instancetype)ngd_init __attribute__((objc_method_family(init))) {
    id player = [self ngd_init];
    if (player && NGDAudioProbeIsEnabled()) NGDAudioProbeRecordObjectDetails(@"AVPlayer.init", player, @{});
    return player;
}
- (instancetype)ngd_initWithURL:(NSURL *)url __attribute__((objc_method_family(init))) {
    id player = [self ngd_initWithURL:url];
    if (player && NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVPlayer.initWithURL", player, @{
            @"scheme": url.scheme ?: @"", @"pathExtension": url.pathExtension ?: @"",
        });
    }
    return player;
}
- (instancetype)ngd_initWithPlayerItem:(AVPlayerItem *)item __attribute__((objc_method_family(init))) {
    id player = [self ngd_initWithPlayerItem:item];
    if (player && NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVPlayer.initWithPlayerItem", player,
                                        @{ @"asset": NGDAssetDescription(item) });
    }
    return player;
}
- (void)ngd_play {
    [self ngd_play];
    if (NGDAudioProbeIsEnabled()) NGDAudioProbeRecordObjectDetails(@"AVPlayer.play", self, NGDPlayerState(self));
}
- (void)ngd_pause {
    [self ngd_pause];
    if (NGDAudioProbeIsEnabled()) NGDAudioProbeRecordObjectDetails(@"AVPlayer.pause", self, NGDPlayerState(self));
}
- (void)ngd_setVolume:(float)volume {
    [self ngd_setVolume:volume];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDPlayerState(self) mutableCopy];
        details[@"requestedVolume"] = @(volume);
        NGDAudioProbeRecordObjectDetails(@"AVPlayer.setVolume", self, details);
    }
}
- (void)ngd_setMuted:(BOOL)muted {
    [self ngd_setMuted:muted];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDPlayerState(self) mutableCopy];
        details[@"requestedMuted"] = @(muted);
        NGDAudioProbeRecordObjectDetails(@"AVPlayer.setMuted", self, details);
    }
}
- (void)ngd_setRate:(float)rate {
    [self ngd_setRate:rate];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDPlayerState(self) mutableCopy];
        details[@"requestedRate"] = @(rate);
        details[@"asset"] = NGDAssetDescription(self.currentItem);
        NGDAudioProbeRecordObjectDetails(@"AVPlayer.setRate", self, details);
    }
}
- (void)ngd_replaceCurrentItemWithPlayerItem:(AVPlayerItem *)item {
    [self ngd_replaceCurrentItemWithPlayerItem:item];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDPlayerState(self) mutableCopy];
        details[@"asset"] = NGDAssetDescription(item);
        NGDAudioProbeRecordObjectDetails(@"AVPlayer.replaceCurrentItem", self, details);
    }
}
@end

@interface AVPlayerItem (NaiwaGuoAudioObservation)
- (void)ngd_setAudioMix:(AVAudioMix *)audioMix;
@end
@implementation AVPlayerItem (NaiwaGuoAudioObservation)
- (void)ngd_setAudioMix:(AVAudioMix *)audioMix {
    [self ngd_setAudioMix:audioMix];
    if (NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVPlayerItem.setAudioMix", self, @{
            @"asset": NGDAssetDescription(self),
            @"inputParameterCount": @(audioMix.inputParameters.count),
            @"cleared": @(audioMix == nil),
        });
    }
}
@end

@interface AVQueuePlayer (NaiwaGuoAudioObservation)
- (instancetype)ngd_initWithItems:(NSArray<AVPlayerItem *> *)items __attribute__((objc_method_family(init)));
@end
@implementation AVQueuePlayer (NaiwaGuoAudioObservation)
- (instancetype)ngd_initWithItems:(NSArray<AVPlayerItem *> *)items __attribute__((objc_method_family(init))) {
    id player = [self ngd_initWithItems:items];
    if (player && NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVQueuePlayer.initWithItems", player,
                                        @{ @"itemCount": @(items.count) });
    }
    return player;
}
@end

@interface AVAudioPlayer (NaiwaGuoAudioObservation)
- (instancetype)ngd_initWithContentsOfURL:(NSURL *)url error:(NSError **)outError __attribute__((objc_method_family(init)));
- (instancetype)ngd_initWithData:(NSData *)data error:(NSError **)outError __attribute__((objc_method_family(init)));
- (BOOL)ngd_play;
- (BOOL)ngd_playAtTime:(NSTimeInterval)time;
- (void)ngd_pause;
- (void)ngd_stop;
- (void)ngd_setVolume:(float)volume;
@end
@implementation AVAudioPlayer (NaiwaGuoAudioObservation)
- (instancetype)ngd_initWithContentsOfURL:(NSURL *)url error:(NSError **)outError __attribute__((objc_method_family(init))) {
    id player = [self ngd_initWithContentsOfURL:url error:outError];
    if (player && NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.initWithContentsOfURL", player,
                                        NGDAudioPlayerState(player));
    }
    return player;
}
- (instancetype)ngd_initWithData:(NSData *)data error:(NSError **)outError __attribute__((objc_method_family(init))) {
    id player = [self ngd_initWithData:data error:outError];
    if (player && NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDAudioPlayerState(player) mutableCopy];
        details[@"dataLength"] = @(data.length);
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.initWithData", player, details);
    }
    return player;
}
- (BOOL)ngd_play {
    BOOL result = [self ngd_play];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDAudioPlayerState(self) mutableCopy];
        details[@"result"] = @(result);
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.play", self, details);
    }
    return result;
}
- (BOOL)ngd_playAtTime:(NSTimeInterval)time {
    BOOL result = [self ngd_playAtTime:time];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDAudioPlayerState(self) mutableCopy];
        details[@"result"] = @(result);
        details[@"requestedTime"] = @(time);
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.playAtTime", self, details);
    }
    return result;
}
- (void)ngd_pause {
    [self ngd_pause];
    if (NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.pause", self, NGDAudioPlayerState(self));
    }
}
- (void)ngd_stop {
    [self ngd_stop];
    if (NGDAudioProbeIsEnabled()) {
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.stop", self, NGDAudioPlayerState(self));
    }
}
- (void)ngd_setVolume:(float)volume {
    [self ngd_setVolume:volume];
    if (NGDAudioProbeIsEnabled()) {
        NSMutableDictionary *details = [NGDAudioPlayerState(self) mutableCopy];
        details[@"requestedVolume"] = @(volume);
        NGDAudioProbeRecordObjectDetails(@"AVAudioPlayer.setVolume", self, details);
    }
}
@end

static void NGDInstallAudioAlias(Class cls, SEL original, SEL alias) {
    Method native=class_getInstanceMethod(cls,original), wrapper=class_getInstanceMethod(cls,alias);
    if (!native || !wrapper || strcmp(method_getTypeEncoding(native),method_getTypeEncoding(wrapper))) return;
    IMP originalIMP=method_getImplementation(native), wrapperIMP=method_getImplementation(wrapper);
    NGDExpectedAudioIMPs[[NSStringFromClass(cls) stringByAppendingFormat:@".%@",NSStringFromSelector(original)]]=[NSValue valueWithPointer:wrapperIMP];
    if (class_addMethod(cls,original,wrapperIMP,method_getTypeEncoding(native))) class_replaceMethod(cls,alias,originalIMP,method_getTypeEncoding(native));
    else method_exchangeImplementations(native,wrapper);
}
void NGDAudioHooksInstall(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NGDExpectedAudioIMPs=[NSMutableDictionary dictionary];
    NGDInstallAudioAlias(AVAudioSession.class, @selector(setActive:error:), @selector(ngd_setActive:error:));
    NGDInstallAudioAlias(AVAudioSession.class, @selector(setActive:withOptions:error:), @selector(ngd_setActive:withOptions:error:));
    NGDInstallAudioAlias(AVAudioSession.class, @selector(setCategory:mode:options:error:), @selector(ngd_setCategory:mode:options:error:));
    NGDInstallAudioAlias(AVAudioSession.class, @selector(setCategory:error:), @selector(ngd_setCategory:error:));
    NGDInstallAudioAlias(AVAudioEngine.class, @selector(init), @selector(ngd_init));
    NGDInstallAudioAlias(AVAudioEngine.class, @selector(startAndReturnError:), @selector(ngd_startAndReturnError:));
    NGDInstallAudioAlias(AVAudioEngine.class, @selector(stop), @selector(ngd_stop));
    NGDInstallAudioAlias(AVPlayer.class, @selector(init), @selector(ngd_init));
    NGDInstallAudioAlias(AVPlayer.class, @selector(initWithURL:), @selector(ngd_initWithURL:));
    NGDInstallAudioAlias(AVPlayer.class, @selector(initWithPlayerItem:), @selector(ngd_initWithPlayerItem:));
    NGDInstallAudioAlias(AVPlayer.class, @selector(play), @selector(ngd_play));
    NGDInstallAudioAlias(AVPlayer.class, @selector(pause), @selector(ngd_pause));
    NGDInstallAudioAlias(AVPlayer.class, @selector(setVolume:), @selector(ngd_setVolume:));
    NGDInstallAudioAlias(AVPlayer.class, @selector(setMuted:), @selector(ngd_setMuted:));
    NGDInstallAudioAlias(AVPlayer.class, @selector(setRate:), @selector(ngd_setRate:));
    NGDInstallAudioAlias(AVPlayer.class, @selector(replaceCurrentItemWithPlayerItem:), @selector(ngd_replaceCurrentItemWithPlayerItem:));
    NGDInstallAudioAlias(AVPlayerItem.class, @selector(setAudioMix:), @selector(ngd_setAudioMix:));
    NGDInstallAudioAlias(AVQueuePlayer.class, @selector(initWithItems:), @selector(ngd_initWithItems:));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(initWithContentsOfURL:error:), @selector(ngd_initWithContentsOfURL:error:));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(initWithData:error:), @selector(ngd_initWithData:error:));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(play), @selector(ngd_play));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(playAtTime:), @selector(ngd_playAtTime:));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(pause), @selector(ngd_pause));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(stop), @selector(ngd_stop));
    NGDInstallAudioAlias(AVAudioPlayer.class, @selector(setVolume:), @selector(ngd_setVolume:));
    });
}
