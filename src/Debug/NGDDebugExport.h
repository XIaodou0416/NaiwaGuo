// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDDebugExport.h
//  NaiwaGuo
//
//  把主线程采集完成的调试上下文序列化并打包。
//

#ifndef NGDDebugExport_h
#define NGDDebugExport_h

#import <Foundation/Foundation.h>
#import "NGDDebugCapture.h"

typedef NS_ENUM(NSUInteger, NGDDebugExportMode) {
    NGDDebugExportModePage,
    NGDDebugExportModeAudio,
};

/// ZIP 生成失败时，工作目录会被保留，路径放在 error.userInfo 的这个键上。
extern NSString *const NGDDebugExportWorkingDirectoryKey;

@interface NGDDebugExportResult : NSObject
@property (nonatomic, strong) NSURL *zipURL;
@property (nonatomic, strong) NSURL *workingDirectoryURL;
@end

#ifdef __cplusplus
extern "C" {
#endif

/// 在后台线程生成导出包。失败时返回 nil，并通过 error 返回真实原因。
NGDDebugExportResult *NGDDebugCreateExport(NGDDebugExportContext *context,
                                         NGDDebugExportMode mode,
                                         void (^progress)(NSString *text),
                                         NSError **error);

/// 分享完成或取消后删除工作目录和 zip。
void NGDDebugCleanupExport(NGDDebugExportResult *result);

#ifdef __cplusplus
}
#endif

#endif /* NGDDebugExport_h */
