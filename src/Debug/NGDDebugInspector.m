// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDDebugInspector.m
//  NaiwaGuo
//
//  全局扳手入口：独立高层级 window 承载调试按钮，点选页面元素后复用
//  NGDDebugCapture / NGDDebugExport 生成导出包。
//

#import "NGDDebugInspector.h"
#import "NGDKeys.h"
#import "NGDUtils.h"
#import "NGDDebugCapture.h"
#import "NGDDebugExport.h"
#import "NGDAudioProbe.h"
#import "NGDAudioRuntime.h"
#import "NGDTabBarProbe.h"

@interface NGDDebugOverlayView : UIView
@end

@implementation NGDDebugOverlayView

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return (hit == self) ? nil : hit;
}

@end

@interface NGDDebugOverlayWindow : UIWindow
@end

@interface NGDDebugOverlayViewController : UIViewController
@property (nonatomic, strong) UIButton *wrenchButton;
@property (nonatomic, assign) BOOL didPlaceButton;
- (BOOL)capturesOverlayTouches;
- (void)exportWholePage;
- (void)showAudioStateMenu;
- (void)startAudioExportWithDeclaredState:(NSString *)declaredState;
@end

@implementation NGDDebugOverlayWindow

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    NGDDebugOverlayViewController *controller = [self.rootViewController isKindOfClass:NGDDebugOverlayViewController.class]
                                               ? (NGDDebugOverlayViewController *)self.rootViewController
                                               : nil;
    if ([controller capturesOverlayTouches]) return [super pointInside:point withEvent:event];
    CGPoint p = [controller.wrenchButton convertPoint:point fromView:self];
    return [controller.wrenchButton pointInside:p withEvent:event];
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    NGDDebugOverlayViewController *controller = [self.rootViewController isKindOfClass:NGDDebugOverlayViewController.class]
                                               ? (NGDDebugOverlayViewController *)self.rootViewController
                                               : nil;
    if ([controller capturesOverlayTouches]) return hit ?: controller.view;
    if (!hit || hit == controller.view || hit == self) return nil;
    return hit;
}

@end

static NGDDebugOverlayWindow *NGDDebugWindow;
static NGDDebugOverlayViewController *NGDDebugController;
static BOOL NGDDebugInstalled;
static BOOL NGDDebugExportBusy;

#pragma mark - UI 工具

static BOOL NGDIsDebugOverlayWindow(UIWindow *window) {
    return [NSStringFromClass(window.class) hasPrefix:@"NGDDebug"];
}

static void NGDPresentError(UIViewController *presenter, NSString *message) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"导出失败"
                                                                       message:message ?: @"未知错误"
                                                                preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"确定" style:UIAlertActionStyleCancel handler:nil]];
        [presenter presentViewController:alert animated:YES completion:nil];
    });
}

// 把 underlying 与保留下来的工作目录一并展开：只显示顶层描述的话，
// ZIP 的 -3/-4/-5 三种失败在界面上长得一模一样，等于没有诊断。
static NSString *NGDExportErrorMessage(NSError *error, NSString *fallback) {
    if (!error) return fallback;
    NSMutableString *message = [NSMutableString stringWithString:error.localizedDescription ?: fallback];
    NSError *underlying = error.userInfo[NSUnderlyingErrorKey];
    if (underlying) {
        [message appendFormat:@"\n\n[%@ %ld] %@", underlying.domain ?: @"",
         (long)underlying.code, underlying.localizedDescription ?: @""];
    }
    NSString *workingDirectory = error.userInfo[NGDDebugExportWorkingDirectoryKey];
    if (workingDirectory.length) {
        [message appendFormat:@"\n\n工作目录已保留:\n%@", workingDirectory];
    }
    return message;
}

static void NGDShareExport(NGDDebugExportResult *result, UIViewController *presenter, UIView *sourceView) {
    if (!result.zipURL) {
        NGDDebugCleanupExport(result);
        NGDPresentError(presenter, @"ZIP 文件不存在");
        return;
    }
    UIActivityViewController *activity = [[UIActivityViewController alloc] initWithActivityItems:@[result.zipURL]
                                                                          applicationActivities:nil];
    if (activity.popoverPresentationController) {
        activity.popoverPresentationController.sourceView = sourceView ?: presenter.view;
        activity.popoverPresentationController.sourceRect = (sourceView ?: presenter.view).bounds;
    }
    activity.completionWithItemsHandler = ^(__unused UIActivityType activityType,
                                            __unused BOOL completed,
                                            __unused NSArray *returnedItems,
                                            __unused NSError *activityError) {
        NGDDebugCleanupExport(result);
    };
    [presenter presentViewController:activity animated:YES completion:nil];
}

static void NGDSetProgress(UIAlertController *alert, NSString *text) {
    dispatch_async(dispatch_get_main_queue(), ^{
        alert.message = text ?: @"处理中...";
    });
}

static NSDictionary *NGDAudioStampedMediaSnapshot(double plannedSecond) {
    double started = NGDAudioProbeCurrentCaptureSecond();
    NSMutableDictionary *snapshot = [NGDAudioRuntimeMediaSnapshot() mutableCopy];
    snapshot[@"plannedSecond"] = @(plannedSecond);
    snapshot[@"actualStartSecond"] = @(started);
    snapshot[@"actualEndSecond"] = @(NGDAudioProbeCurrentCaptureSecond());
    // 红果的静音若是压系统输出音量，三个时刻的读数放在一起就能看出来。
    snapshot[@"outputVolume"] = NGDAudioProbeSessionSnapshot()[@"outputVolume"] ?: @(-1);
    return snapshot;
}

static void NGDCreateExportInBackground(NGDDebugExportContext *context,
                                       NGDDebugExportMode mode,
                                       UIAlertController *progressAlert) {
    UIViewController *presenter = context.presenter ?: NGDDebugController;
    if (!presenter) {
        NGDDebugExportBusy = NO;
        return;
    }

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSError *exportError = nil;
        NGDDebugExportResult *result = NGDDebugCreateExport(context, mode, ^(NSString *text) {
            NGDSetProgress(progressAlert, text);
        }, &exportError);
        dispatch_async(dispatch_get_main_queue(), ^{
            NGDDebugExportBusy = NO;
            [progressAlert dismissViewControllerAnimated:YES completion:^{
                if (result) {
                    NGDShareExport(result, presenter, context.sourceView ?: presenter.view);
                } else {
                    NGDPresentError(presenter, NGDExportErrorMessage(exportError, @"没有生成 ZIP 文件"));
                }
            }];
        });
    });
}

static void NGDStartExport(NGDDebugExportContext *context, NGDDebugExportMode mode) {
    UIViewController *presenter = context.presenter ?: NGDDebugController;
    if (!presenter) return;
    if (NGDDebugExportBusy) {
        NGDPresentError(presenter, @"已有调试采集或导出正在进行");
        return;
    }
    NGDDebugExportBusy = YES;

    // 探针读 UIKit，必须在主线程生成后塞进上下文；后台任务只做序列化与压缩。
    context.probeText = NGDTabBarProbeReport() ?: @"";

    UIAlertController *progressAlert = [UIAlertController alertControllerWithTitle:@"奶蛙"
                                                                           message:@"准备导出..."
                                                                    preferredStyle:UIAlertControllerStyleAlert];
    [presenter presentViewController:progressAlert animated:YES completion:^{
        NGDCreateExportInBackground(context, mode, progressAlert);
    }];
}

static UIWindowScene *NGDDebugForegroundScene(void) {
    if (@available(iOS 13.0, *)) {
        UIWindow *target = NGDDebugTargetWindow();
        if (target.windowScene) return target.windowScene;
        for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
            if (![scene isKindOfClass:[UIWindowScene class]]) continue;
            if (scene.activationState == UISceneActivationStateForegroundActive) return (UIWindowScene *)scene;
        }
    }
    return nil;
}

static CGRect NGDDebugScreenBounds(void) {
    UIWindow *target = NGDDebugTargetWindow();
    if (target) return target.bounds;
    return UIScreen.mainScreen.bounds;
}

static void NGDEnsureDebugWindow(void) {
    if (NGDDebugWindow && NGDDebugController) {
        if (@available(iOS 13.0, *)) {
            UIWindowScene *scene = NGDDebugForegroundScene();
            if (scene && NGDDebugWindow.windowScene != scene) NGDDebugWindow.windowScene = scene;
        }
        return;
    }

    NGDDebugController = [NGDDebugOverlayViewController new];
    if (@available(iOS 13.0, *)) {
        UIWindowScene *scene = NGDDebugForegroundScene();
        NGDDebugWindow = scene ? [[NGDDebugOverlayWindow alloc] initWithWindowScene:scene]
                              : [[NGDDebugOverlayWindow alloc] initWithFrame:NGDDebugScreenBounds()];
    } else {
        NGDDebugWindow = [[NGDDebugOverlayWindow alloc] initWithFrame:NGDDebugScreenBounds()];
    }
    NGDDebugWindow.backgroundColor = UIColor.clearColor;
    NGDDebugWindow.opaque = NO;
    NGDDebugWindow.windowLevel = UIWindowLevelAlert + 100000.0;
    NGDDebugWindow.rootViewController = NGDDebugController;
    NGDDebugWindow.hidden = YES;
}

#pragma mark - 浮层控制器

@implementation NGDDebugOverlayViewController

- (void)loadView {
    NGDDebugOverlayView *view = [[NGDDebugOverlayView alloc] initWithFrame:NGDDebugScreenBounds()];
    view.backgroundColor = UIColor.clearColor;
    view.opaque = NO;
    self.view = view;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.wrenchButton = [UIButton buttonWithType:UIButtonTypeCustom];
    self.wrenchButton.frame = CGRectMake(0, 0, 48, 48);
    self.wrenchButton.backgroundColor = [UIColor colorWithWhite:0.05 alpha:0.82];
    self.wrenchButton.tintColor = UIColor.whiteColor;
    self.wrenchButton.layer.cornerRadius = 24;
    self.wrenchButton.layer.shadowColor = UIColor.blackColor.CGColor;
    self.wrenchButton.layer.shadowOpacity = 0.24;
    self.wrenchButton.layer.shadowRadius = 8;
    self.wrenchButton.layer.shadowOffset = CGSizeMake(0, 2);
    self.wrenchButton.accessibilityLabel = @"奶蛙 调试工具";
    self.wrenchButton.accessibilityIdentifier = @"NaiwaGuo.Debug.Button";
    [self.wrenchButton addGestureRecognizer:[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(openPluginSettings:)]];

    UIImage *image = nil;
    if ([UIImage respondsToSelector:@selector(systemImageNamed:)]) image = [UIImage systemImageNamed:@"wrench.fill"];
    if (image) {
        [self.wrenchButton setImage:image forState:UIControlStateNormal];
    } else {
        [self.wrenchButton setTitle:@"W" forState:UIControlStateNormal];
        self.wrenchButton.titleLabel.font = [UIFont boldSystemFontOfSize:20];
    }
    [self.wrenchButton addTarget:self action:@selector(showDebugMenu) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.wrenchButton];

    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handleWrenchPan:)];
    [self.wrenchButton addGestureRecognizer:pan];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    if (!self.didPlaceButton) {
        UIEdgeInsets insets = self.view.safeAreaInsets;
        CGFloat x = self.view.bounds.size.width - insets.right - 12.0 - 24.0;
        CGFloat y = insets.top + 12.0 + 24.0;
        self.wrenchButton.center = CGPointMake(x, y);
        self.didPlaceButton = YES;
    }
    [self clampWrenchButton];
}

- (void)clampWrenchButton {
    UIEdgeInsets insets = self.view.safeAreaInsets;
    CGFloat halfW = self.wrenchButton.bounds.size.width / 2.0;
    CGFloat halfH = self.wrenchButton.bounds.size.height / 2.0;
    CGFloat minX = insets.left + halfW + 6.0;
    CGFloat maxX = self.view.bounds.size.width - insets.right - halfW - 6.0;
    CGFloat minY = insets.top + halfH + 6.0;
    CGFloat maxY = self.view.bounds.size.height - insets.bottom - halfH - 6.0;
    CGPoint c = self.wrenchButton.center;
    c.x = MIN(MAX(c.x, minX), maxX);
    c.y = MIN(MAX(c.y, minY), maxY);
    self.wrenchButton.center = c;
}

- (void)openPluginSettings:(UILongPressGestureRecognizer *)gesture { if (gesture.state==UIGestureRecognizerStateBegan) NGDebugOpenSettings(); }

- (void)handleWrenchPan:(UIPanGestureRecognizer *)pan {
    CGPoint delta = [pan translationInView:self.view];
    self.wrenchButton.center = CGPointMake(self.wrenchButton.center.x + delta.x,
                                           self.wrenchButton.center.y + delta.y);
    [pan setTranslation:CGPointZero inView:self.view];
    [self clampWrenchButton];
}

- (void)showDebugMenu {
    if (!NGDPrefBool(NGDKeyDebugInspectorEnabled)) return;

    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"奶蛙调试工具"
                                                                   message:@"页面包可能包含截图与界面文字；音频包可能包含三秒播放声音。仅本地生成，由你选择分享。"
                                                            preferredStyle:UIAlertControllerStyleActionSheet];
    [alert addAction:[UIAlertAction actionWithTitle:@"导出本页 zip"
                                              style:UIAlertActionStyleDefault
                                            handler:^(__unused UIAlertAction *action) {
        [self exportWholePage];
    }]];
    [alert addAction:[UIAlertAction actionWithTitle:@"导出音频专项 zip"
                                              style:UIAlertActionStyleDefault
                                            handler:^(__unused UIAlertAction *action) {
        [self showAudioStateMenu];
    }]];
    [alert addAction:[UIAlertAction actionWithTitle:@"关闭" style:UIAlertActionStyleCancel handler:nil]];
    if (alert.popoverPresentationController) {
        alert.popoverPresentationController.sourceView = self.wrenchButton;
        alert.popoverPresentationController.sourceRect = self.wrenchButton.bounds;
    }
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)exportWholePage {
    UIWindow *targetWindow = NGDDebugTargetWindow();
    if (!targetWindow) {
        NGDPresentError(self, @"没有找到可导出的 App 窗口");
        return;
    }

    // 调试浮层始终置顶、无需选元素：直接快照整页（selectedView=nil 即整窗）。
    NGDDebugExportContext *context = NGDDebugCaptureContext(targetWindow, CGPointZero, nil);
    context.presenter = self;
    context.sourceView = self.wrenchButton;
    NGDStartExport(context, NGDDebugExportModePage);
}

- (void)showAudioStateMenu {
    if (NGDDebugExportBusy || NGDAudioProbeIsCaptureActive()) {
        NGDPresentError(self, @"已有调试采集或导出正在进行");
        return;
    }
    if (!NGDAudioProbeHasLaunchCoverage()) {
        NGDPresentError(self, @"本进程没有启动期音频覆盖。请保持“调试工具”开启，彻底退出并重新启动红果后再导出。");
        return;
    }

    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"音频专项导出"
                                                                   message:@"选择采样期间保持不变的当前状态"
                                                            preferredStyle:UIAlertControllerStyleActionSheet];
    [alert addAction:[UIAlertAction actionWithTitle:@"播放中"
                                              style:UIAlertActionStyleDefault
                                            handler:^(__unused UIAlertAction *action) {
        [self startAudioExportWithDeclaredState:@"playing"];
    }]];
    [alert addAction:[UIAlertAction actionWithTitle:@"已暂停"
                                              style:UIAlertActionStyleDefault
                                            handler:^(__unused UIAlertAction *action) {
        [self startAudioExportWithDeclaredState:@"paused"];
    }]];
    // 与「播放中」那份对照，才能判定可视化要不要额外的静音闸门。
    [alert addAction:[UIAlertAction actionWithTitle:@"静音播放中"
                                              style:UIAlertActionStyleDefault
                                            handler:^(__unused UIAlertAction *action) {
        [self startAudioExportWithDeclaredState:@"muted-playing"];
    }]];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    if (alert.popoverPresentationController) {
        alert.popoverPresentationController.sourceView = self.wrenchButton;
        alert.popoverPresentationController.sourceRect = self.wrenchButton.bounds;
    }
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)startAudioExportWithDeclaredState:(NSString *)declaredState {
    if (NGDDebugExportBusy || NGDAudioProbeIsCaptureActive()) {
        NGDPresentError(self, @"已有调试采集或导出正在进行");
        return;
    }
    UIWindow *targetWindow = NGDDebugTargetWindow();
    if (!targetWindow) {
        NGDPresentError(self, @"没有找到可导出的 App 窗口");
        return;
    }
    if (!NGDAudioProbeStartCapture(declaredState)) {
        NGDPresentError(self, @"音频探针未就绪，无法开始五秒采样");
        return;
    }

    NGDDebugExportBusy = YES;
    NSMutableArray<NSDictionary *> *snapshots = [NSMutableArray arrayWithCapacity:3];
    [snapshots addObject:NGDAudioStampedMediaSnapshot(0)];

    UIAlertController *progressAlert = [UIAlertController alertControllerWithTitle:@"奶蛙 音频探针"
                                                                           message:@"0/5 秒：稳定期，请保持当前播放状态"
                                                                    preferredStyle:UIAlertControllerStyleAlert];
    [self presentViewController:progressAlert animated:YES completion:nil];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NGDSetProgress(progressAlert, @"1/5 秒：正在记录三秒 PCM 与后端事件");
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [snapshots addObject:NGDAudioStampedMediaSnapshot(2.5)];
        NGDSetProgress(progressAlert, @"2.5/5 秒：PCM 记录中，请保持当前状态");
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(4.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NGDSetProgress(progressAlert, @"4/5 秒：收尾期，请继续保持当前状态");
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NGDAudioProbeStopCapture();
        [snapshots addObject:NGDAudioStampedMediaSnapshot(5)];
        NGDSetProgress(progressAlert, @"采样完成：冻结底栏几何与截图...");

        NSDictionary *glassTarget = NGDAudioRuntimeGlassTarget();
        NGDDebugExportContext *context = NGDDebugCaptureMetadataContext(targetWindow);
        context.presenter = self;
        context.sourceView = self.wrenchButton;
        context.probeText = NGDTabBarProbeReport() ?: @"";

        NSArray *frozenSnapshots = [snapshots copy];
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            NGDSetProgress(progressAlert, @"计算 RMS、频谱并生成 WAV...");
            context.audioCapture = NGDAudioProbeBuildCapture(frozenSnapshots, glassTarget);
            if (!context.audioCapture) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    NGDDebugExportBusy = NO;
                    [progressAlert dismissViewControllerAnimated:YES completion:^{
                        NGDPresentError(self, @"音频采集结果生成失败");
                    }];
                });
                return;
            }

            NSError *exportError = nil;
            NGDDebugExportResult *result = NGDDebugCreateExport(context, NGDDebugExportModeAudio, ^(NSString *text) {
                NGDSetProgress(progressAlert, text);
            }, &exportError);
            dispatch_async(dispatch_get_main_queue(), ^{
                NGDDebugExportBusy = NO;
                [progressAlert dismissViewControllerAnimated:YES completion:^{
                    if (result) NGDShareExport(result, self, self.wrenchButton);
                    else NGDPresentError(self, NGDExportErrorMessage(exportError, @"没有生成音频专项 ZIP 文件"));
                }];
            });
        });
    });
}

- (BOOL)capturesOverlayTouches {
    return self.presentedViewController != nil;
}

@end

#pragma mark - 对外入口

void NGDDebugInspectorRefreshOverlay(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        BOOL enabled = NGDPrefBool(NGDKeyDebugInspectorEnabled);
        if (!enabled) {
            if (NGDDebugWindow) {
                NGDDebugWindow.hidden = YES;
                [NGDDebugTargetWindow() makeKeyWindow];
            }
            return;
        }

        NGDEnsureDebugWindow();
        NGDDebugWindow.frame = NGDDebugScreenBounds();
        NGDDebugWindow.hidden = NO;
        NGDDebugWindow.accessibilityIdentifier = @"NaiwaGuo.Debug.Window";
        UIWindow *targetWindow = NGDDebugTargetWindow();
        if (targetWindow && !targetWindow.isKeyWindow) [targetWindow makeKeyWindow];
    });
}

void NGDDebugInspectorInstall(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (NGDDebugInstalled) {
            NGDDebugInspectorRefreshOverlay();
            return;
        }
        NGDDebugInstalled = YES;

        NSNotificationCenter *nc = NSNotificationCenter.defaultCenter;
        [nc addObserverForName:UIWindowDidBecomeKeyNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            if (NGDIsDebugOverlayWindow((UIWindow *)note.object)) return;
            NGDDebugInspectorRefreshOverlay();
        }];
        [nc addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(__unused NSNotification *note) {
            NGDDebugInspectorRefreshOverlay();
        }];
        NGDDebugInspectorRefreshOverlay();
    });
}

UIWindow *NGDDebugInspectorWindow(void) { return NGDDebugWindow; }
void NGDDebugInspectorPresentMenu(void) {
    if (!NGDPrefBool(NGDKeyDebugInspectorEnabled)) return;
    NGDDebugInspectorRefreshOverlay();
    dispatch_async(dispatch_get_main_queue(), ^{ if (!NGDDebugController.presentedViewController) [NGDDebugController showDebugMenu]; });
}
