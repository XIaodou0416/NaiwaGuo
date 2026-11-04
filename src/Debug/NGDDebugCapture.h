// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDDebugCapture.h
//  NaiwaGuo
//
//  抓取当前页面的运行时快照（窗口/视图树/VC 树/图层/截图/本页类），产出
//  一个只含"已算好数据"的 NGDDebugExportContext，交给 NGDDebugExport 落盘。
//

#ifndef NGDDebugCapture_h
#define NGDDebugCapture_h

#import <UIKit/UIKit.h>

@class NGDAudioProbeCapture;

/// 一次页面快照的产物（纯数据；序列化由 NGDDebugExport 负责）。
@interface NGDDebugExportContext : NSObject
@property (nonatomic, strong) NSDictionary *metadata;
@property (nonatomic, strong) NSArray *windowsJSON;
@property (nonatomic, strong) NSArray *viewTreeJSON;
@property (nonatomic, copy) NSString *viewTreeText;
@property (nonatomic, strong) NSDictionary *selectedViewJSON;
@property (nonatomic, copy) NSString *viewControllersText;
@property (nonatomic, strong) NSArray *layersJSON;
@property (nonatomic, strong) NSData *screenshotPNG;
@property (nonatomic, copy) NSString *summary;
@property (nonatomic, strong) NSArray<NSString *> *pageClassNames;
/// 主线程生成的探针文本（probe/tabbar.txt）；后台导出只落盘，不再读 UIKit。
@property (nonatomic, copy) NSString *probeText;
/// 音频专项模式的五秒采样产物；普通页面/全类导出时为 nil。
@property (nonatomic, strong) NGDAudioProbeCapture *audioCapture;
@property (nonatomic, weak) UIView *sourceView;
@property (nonatomic, weak) UIViewController *presenter;
@end

#ifdef __cplusplus
extern "C" {
#endif

/// 前台全部可见窗口（排除调试自身窗口），按系统返回顺序。
/// 探针必须逐个看：浮层窗口（如 AWEDPlayerPiPWindow，windowLevel 2000）盖在主窗口之上，
/// 只看 key 窗口会把它当成不存在。
NSArray<UIWindow *> *NGDDebugActiveWindows(void);

/// 前台 key 窗口（排除调试自身窗口）。
UIWindow *NGDDebugTargetWindow(void);

/// 某窗口当前最顶层的 VC（穿透 present/nav/tab）。
UIViewController *NGDDebugTopPresenter(UIWindow *window);

/// 抓取一次页面快照。
NGDDebugExportContext *NGDDebugCaptureContext(UIWindow *targetWindow, CGPoint point, UIView *selectedView);

/// 只采元信息与截图，不遍历 view tree；音频专项导出用。
NGDDebugExportContext *NGDDebugCaptureMetadataContext(UIWindow *targetWindow);

#ifdef __cplusplus
}
#endif

#endif /* NGDDebugCapture_h */
