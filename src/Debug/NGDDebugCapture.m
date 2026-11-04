// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDDebugCapture.m
//  NaiwaGuo
//
//  只读地遍历 UIKit/CoreAnimation 安全字段 + 收集本页类，产出 NGDDebugExportContext。
//  不递归读任意对象的 ivar 值。
//

#import "NGDDebugCapture.h"
#import "NGDKeys.h"
#import "NGDClassDump.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

@implementation NGDDebugExportContext
@end

#pragma mark - 小型格式化工具

static NSString *NGDStringFromPointer(const void *pointer) {
    return [NSString stringWithFormat:@"%p", pointer];
}

static NSString *NGDClassName(id object) {
    return object ? NSStringFromClass([object class]) : @"";
}

static NSString *NGDObjectDesc(id object) {
    if (!object) return @"";
    @try {
        return [object description] ?: @"";
    } @catch (__unused NSException *exception) {
        return @"<description threw>";
    }
}

static NSNumber *NGDNum(double value) {
    return @(isfinite(value) ? value : 0);
}

static NSString *NGDStringFromColor(UIColor *color) {
    if (!color) return @"";
    CGFloat r = 0, g = 0, b = 0, a = 0;
    if ([color getRed:&r green:&g blue:&b alpha:&a]) {
        return [NSString stringWithFormat:@"rgba(%.4f, %.4f, %.4f, %.4f)", r, g, b, a];
    }
    return NGDObjectDesc(color);
}

static NSDictionary *NGDRectDict(CGRect rect) {
    return @{
        @"x": NGDNum(rect.origin.x),
        @"y": NGDNum(rect.origin.y),
        @"width": NGDNum(rect.size.width),
        @"height": NGDNum(rect.size.height),
        @"string": NSStringFromCGRect(rect)
    };
}

static NSDictionary *NGDPointDict(CGPoint point) {
    return @{ @"x": NGDNum(point.x), @"y": NGDNum(point.y), @"string": NSStringFromCGPoint(point) };
}

static NSDictionary *NGDSizeDict(CGSize size) {
    return @{ @"width": NGDNum(size.width), @"height": NGDNum(size.height), @"string": NSStringFromCGSize(size) };
}

static NSDictionary *NGDAffineDict(CGAffineTransform t) {
    return @{
        @"a": NGDNum(t.a), @"b": NGDNum(t.b), @"c": NGDNum(t.c),
        @"d": NGDNum(t.d), @"tx": NGDNum(t.tx), @"ty": NGDNum(t.ty)
    };
}

static NSDictionary *NGDTransform3DDict(CATransform3D t) {
    return @{
        @"m11": NGDNum(t.m11), @"m12": NGDNum(t.m12), @"m13": NGDNum(t.m13), @"m14": NGDNum(t.m14),
        @"m21": NGDNum(t.m21), @"m22": NGDNum(t.m22), @"m23": NGDNum(t.m23), @"m24": NGDNum(t.m24),
        @"m31": NGDNum(t.m31), @"m32": NGDNum(t.m32), @"m33": NGDNum(t.m33), @"m34": NGDNum(t.m34),
        @"m41": NGDNum(t.m41), @"m42": NGDNum(t.m42), @"m43": NGDNum(t.m43), @"m44": NGDNum(t.m44)
    };
}

#pragma mark - 窗口与控制器工具

static BOOL NGDIsDebugWindow(UIWindow *window) {
    NSString *cls = NGDClassName(window);
    return [cls hasPrefix:@"NGDDebug"] || [cls hasPrefix:@"FLEX"];
}

NSArray<UIWindow *> *NGDDebugActiveWindows(void) {
    NSMutableArray<UIWindow *> *windows = [NSMutableArray array];
    if (@available(iOS 13.0, *)) {
        for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
            if (![scene isKindOfClass:[UIWindowScene class]]) continue;
            if (scene.activationState != UISceneActivationStateForegroundActive) continue;
            for (UIWindow *window in ((UIWindowScene *)scene).windows) {
                if (!window || NGDIsDebugWindow(window)) continue;
                if (window.hidden || window.alpha <= 0.01) continue;
                [windows addObject:window];
            }
        }
    }
    if (windows.count == 0) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        for (UIWindow *window in UIApplication.sharedApplication.windows) {
            if (!window || NGDIsDebugWindow(window)) continue;
            if (window.hidden || window.alpha <= 0.01) continue;
            [windows addObject:window];
        }
#pragma clang diagnostic pop
    }
    return windows;
}

UIWindow *NGDDebugTargetWindow(void) {
    NSArray<UIWindow *> *windows = NGDDebugActiveWindows();
    for (UIWindow *window in windows) if (window.isKeyWindow) return window;
    return windows.firstObject;
}

static UIViewController *NGDViewControllerForView(UIView *view) {
    UIResponder *responder = view;
    for (NSUInteger i = 0; responder && i < 80; i++) {
        responder = responder.nextResponder;
        if ([responder isKindOfClass:[UIViewController class]]) return (UIViewController *)responder;
    }
    return nil;
}

static UIViewController *NGDTopViewControllerFrom(UIViewController *vc) {
    for (NSUInteger i = 0; vc && i < 80; i++) {
        if (vc.presentedViewController) {
            vc = vc.presentedViewController;
        } else if ([vc isKindOfClass:[UINavigationController class]]) {
            UIViewController *visible = ((UINavigationController *)vc).visibleViewController;
            if (!visible || visible == vc) break;
            vc = visible;
        } else if ([vc isKindOfClass:[UITabBarController class]]) {
            UIViewController *selected = ((UITabBarController *)vc).selectedViewController;
            if (!selected || selected == vc) break;
            vc = selected;
        } else {
            break;
        }
    }
    return vc;
}

UIViewController *NGDDebugTopPresenter(UIWindow *window) {
    return NGDTopViewControllerFrom(window.rootViewController);
}

#pragma mark - 页面快照

static NSDictionary *NGDLayerInfo(CALayer *layer) {
    if (!layer) return @{};
    return @{
        @"class": NGDClassName(layer),
        @"address": NGDStringFromPointer((__bridge const void *)layer),
        @"bounds": NGDRectDict(layer.bounds),
        @"position": NGDPointDict(layer.position),
        @"anchorPoint": NGDPointDict(layer.anchorPoint),
        @"zPosition": NGDNum(layer.zPosition),
        @"opacity": NGDNum(layer.opacity),
        @"hidden": @(layer.hidden),
        @"masksToBounds": @(layer.masksToBounds),
        @"cornerRadius": NGDNum(layer.cornerRadius),
        @"sublayerCount": @(layer.sublayers.count),
        @"transform": NGDTransform3DDict(layer.transform),
        @"mask": layer.mask ? @{
            @"class": NGDClassName(layer.mask),
            @"address": NGDStringFromPointer((__bridge const void *)layer.mask),
            @"bounds": NGDRectDict(layer.mask.bounds)
        } : @{}
    };
}

static NSMutableDictionary *NGDBaseViewInfo(UIView *view, NSUInteger depth) {
    UIViewController *nearestVC = NGDViewControllerForView(view);
    NSMutableDictionary *info = [@{
        @"class": NGDClassName(view),
        @"address": NGDStringFromPointer((__bridge const void *)view),
        @"depth": @(depth),
        @"frame": NGDRectDict(view.frame),
        @"bounds": NGDRectDict(view.bounds),
        @"center": NGDPointDict(view.center),
        @"alpha": NGDNum(view.alpha),
        @"hidden": @(view.hidden),
        @"opaque": @(view.opaque),
        @"clipsToBounds": @(view.clipsToBounds),
        @"userInteractionEnabled": @(view.userInteractionEnabled),
        @"multipleTouchEnabled": @(view.multipleTouchEnabled),
        @"contentMode": @(view.contentMode),
        @"tag": @(view.tag),
        @"autoresizingMask": @(view.autoresizingMask),
        @"autoresizesSubviews": @(view.autoresizesSubviews),
        @"transform": NGDAffineDict(view.transform),
        @"backgroundColor": NGDStringFromColor(view.backgroundColor),
        @"tintColor": NGDStringFromColor(view.tintColor),
        @"accessibilityIdentifier": view.accessibilityIdentifier ?: @"",
        @"accessibilityLabel": view.accessibilityLabel ?: @"",
        @"nearestViewController": nearestVC ? @{
            @"class": NGDClassName(nearestVC),
            @"address": NGDStringFromPointer((__bridge const void *)nearestVC)
        } : @{},
        @"layer": NGDLayerInfo(view.layer),
        @"subviewCount": @(view.subviews.count)
    } mutableCopy];

    if ([view isKindOfClass:[UILabel class]]) {
        UILabel *label = (UILabel *)view;
        info[@"label"] = @{
            @"text": label.text ?: @"",
            @"attributedText": label.attributedText.string ?: @"",
            @"font": NGDObjectDesc(label.font),
            @"textColor": NGDStringFromColor(label.textColor),
            @"numberOfLines": @(label.numberOfLines),
            @"textAlignment": @(label.textAlignment)
        };
    }
    if ([view isKindOfClass:[UIButton class]]) {
        UIButton *button = (UIButton *)view;
        info[@"button"] = @{
            @"currentTitle": [button titleForState:UIControlStateNormal] ?: (button.currentTitle ?: @""),
            @"currentAttributedTitle": button.currentAttributedTitle.string ?: @"",
            @"enabled": @(button.enabled),
            @"selected": @(button.selected),
            @"highlighted": @(button.highlighted)
        };
    }
    if ([view isKindOfClass:[UIImageView class]]) {
        UIImageView *imageView = (UIImageView *)view;
        info[@"imageView"] = @{
            @"imageSize": imageView.image ? NGDSizeDict(imageView.image.size) : @{},
            @"highlighted": @(imageView.highlighted)
        };
    }
    if ([view isKindOfClass:[UIControl class]]) {
        UIControl *control = (UIControl *)view;
        info[@"control"] = @{
            @"enabled": @(control.enabled),
            @"selected": @(control.selected),
            @"highlighted": @(control.highlighted),
            @"state": @(control.state)
        };
    }
    if ([view isKindOfClass:[UIScrollView class]]) {
        UIScrollView *scroll = (UIScrollView *)view;
        info[@"scrollView"] = @{
            @"contentOffset": NGDPointDict(scroll.contentOffset),
            @"contentSize": NGDSizeDict(scroll.contentSize),
            @"contentInset": NSStringFromUIEdgeInsets(scroll.contentInset),
            @"adjustedContentInset": NSStringFromUIEdgeInsets(scroll.adjustedContentInset),
            @"zoomScale": NGDNum(scroll.zoomScale),
            @"minimumZoomScale": NGDNum(scroll.minimumZoomScale),
            @"maximumZoomScale": NGDNum(scroll.maximumZoomScale),
            @"pagingEnabled": @(scroll.pagingEnabled),
            @"scrollEnabled": @(scroll.scrollEnabled),
            @"dragging": @(scroll.dragging),
            @"tracking": @(scroll.tracking),
            @"decelerating": @(scroll.decelerating)
        };
    }
    if ([view isKindOfClass:[UIVisualEffectView class]]) {
        info[@"visualEffectView"] = @{ @"effect": NGDObjectDesc(((UIVisualEffectView *)view).effect) };
    }
    return info;
}

static NSDictionary *NGDViewTreeJSON(UIView *view, NSUInteger depth, NSMutableArray *layers) {
    NSMutableDictionary *info = NGDBaseViewInfo(view, depth);
    [layers addObject:@{
        @"viewClass": info[@"class"],
        @"viewAddress": info[@"address"],
        @"nearestViewController": info[@"nearestViewController"],
        @"layer": info[@"layer"]
    }];

    NSMutableArray *children = [NSMutableArray arrayWithCapacity:view.subviews.count];
    for (UIView *subview in view.subviews) {
        [children addObject:NGDViewTreeJSON(subview, depth + 1, layers)];
    }
    info[@"children"] = children;
    return info;
}

static void NGDAppendViewTreeText(UIView *view, NSUInteger depth, NSMutableString *out) {
    NSMutableString *indent = [NSMutableString string];
    for (NSUInteger i = 0; i < depth; i++) [indent appendString:@"  "];
    UIViewController *vc = NGDViewControllerForView(view);
    [out appendFormat:@"%@%@ %@ frame=%@ bounds=%@ vc=%@ %@ hidden=%d alpha=%.3f clips=%d user=%d layer=%@\n",
     indent,
     NGDClassName(view),
     NGDStringFromPointer((__bridge const void *)view),
     NSStringFromCGRect(view.frame),
     NSStringFromCGRect(view.bounds),
     vc ? NGDClassName(vc) : @"",
     vc ? NGDStringFromPointer((__bridge const void *)vc) : @"",
     view.hidden,
     view.alpha,
     view.clipsToBounds,
     view.userInteractionEnabled,
     NGDClassName(view.layer)];
    for (UIView *subview in view.subviews) NGDAppendViewTreeText(subview, depth + 1, out);
}

static NSDictionary *NGDWindowJSON(UIWindow *window) {
    return @{
        @"class": NGDClassName(window),
        @"address": NGDStringFromPointer((__bridge const void *)window),
        @"frame": NGDRectDict(window.frame),
        @"bounds": NGDRectDict(window.bounds),
        @"windowLevel": NGDNum(window.windowLevel),
        @"hidden": @(window.hidden),
        @"alpha": NGDNum(window.alpha),
        @"keyWindow": @(window.isKeyWindow),
        @"rootViewController": window.rootViewController ? @{
            @"class": NGDClassName(window.rootViewController),
            @"address": NGDStringFromPointer((__bridge const void *)window.rootViewController)
        } : @{}
    };
}

static NSArray *NGDAncestorChain(UIView *view) {
    NSMutableArray *chain = [NSMutableArray array];
    UIView *v = view;
    for (NSUInteger depth = 0; v && depth < 120; depth++, v = v.superview) {
        [chain addObject:NGDBaseViewInfo(v, depth)];
    }
    return chain;
}

static void NGDAppendVCText(UIViewController *vc, NSUInteger depth, NSMutableString *out, NSHashTable *seen) {
    if (!vc || [seen containsObject:vc]) return;
    [seen addObject:vc];
    NSMutableString *indent = [NSMutableString string];
    for (NSUInteger i = 0; i < depth; i++) [indent appendString:@"  "];
    BOOL loaded = vc.isViewLoaded;
    UIView *view = loaded ? vc.view : nil;
    [out appendFormat:@"%@%@ %@ viewLoaded=%@ view=%@ %@ frame=%@\n",
     indent,
     NGDClassName(vc),
     NGDStringFromPointer((__bridge const void *)vc),
     loaded ? @"YES" : @"NO",
     NGDClassName(view),
     view ? NGDStringFromPointer((__bridge const void *)view) : @"",
     view ? NSStringFromCGRect(view.frame) : @"null"];

    if ([vc isKindOfClass:[UINavigationController class]]) {
        UINavigationController *nav = (UINavigationController *)vc;
        [out appendFormat:@"%@  nav.top=%@ nav.visible=%@\n", indent, NGDClassName(nav.topViewController), NGDClassName(nav.visibleViewController)];
    }
    if ([vc isKindOfClass:[UITabBarController class]]) {
        UITabBarController *tab = (UITabBarController *)vc;
        [out appendFormat:@"%@  tab.selected=%@\n", indent, NGDClassName(tab.selectedViewController)];
    }
    for (UIViewController *child in vc.childViewControllers) NGDAppendVCText(child, depth + 1, out, seen);
    if (vc.presentedViewController) {
        [out appendFormat:@"%@  presented:\n", indent];
        NGDAppendVCText(vc.presentedViewController, depth + 1, out, seen);
    }
}

// 按 windowLevel 从低到高逐个画，截图才等于用户真正看到的画面。
// 只画 key 窗口会漏掉浮层：红果的内嵌画中画 AWEDPlayerPiPWindow 在 windowLevel 2000，
// 它铺满全屏时导出截图却一切正常，排查会整整绕一轮。
static NSData *NGDScreenshotPNG(UIWindow *targetWindow) {
    if (!targetWindow) return NSData.data;

    NSArray<UIWindow *> *windows =
        [NGDDebugActiveWindows() sortedArrayUsingComparator:^NSComparisonResult(UIWindow *a, UIWindow *b) {
            if (a.windowLevel == b.windowLevel) return NSOrderedSame;
            return a.windowLevel < b.windowLevel ? NSOrderedAscending : NSOrderedDescending;
        }];
    if (windows.count == 0) windows = @[ targetWindow ];

    UIGraphicsBeginImageContextWithOptions(targetWindow.bounds.size, NO, 0);
    for (UIWindow *window in windows) {
        BOOL drew = [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:NO];
        if (!drew) [window.layer renderInContext:UIGraphicsGetCurrentContext()];
    }
    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return image ? UIImagePNGRepresentation(image) : NSData.data;
}

#pragma mark - 页面类收集

// 收集一个类及其整条继承链的类名：跳过运行时生成子类噪声，但继续上溯 → 自动收到真实基类。
static void NGDCollectClassAndSupers(Class cls, NSMutableSet<NSString *> *names) {
    for (int i = 0; cls && i < 50; i++) {
        if (!NGDClassIsSafe(cls)) break;
        NSString *name = NSStringFromClass(cls);
        if (name.length && !NGDClassNameIsRuntimeGenerated(name)) [names addObject:name];
        cls = class_getSuperclass(cls);
    }
}

// 递归收集视图子树里每个 view 及其 layer 的类（含继承链）。
static void NGDCollectPageClassesFromView(UIView *view, NSMutableSet<NSString *> *names) {
    if (!view) return;
    NGDCollectClassAndSupers(object_getClass(view), names);
    NGDCollectClassAndSupers(object_getClass(view.layer), names);
    for (UIView *sub in view.subviews) NGDCollectPageClassesFromView(sub, names);
}

// 递归收集 VC 树（child + presented）里每个 VC 及其已加载 view 的类（含继承链）。
static void NGDCollectPageClassesFromVC(UIViewController *vc, NSMutableSet<NSString *> *names, NSHashTable *seen) {
    if (!vc || [seen containsObject:vc]) return;
    [seen addObject:vc];
    NGDCollectClassAndSupers(object_getClass(vc), names);
    if (vc.isViewLoaded) NGDCollectClassAndSupers(object_getClass(vc.view), names);
    for (UIViewController *child in vc.childViewControllers) NGDCollectPageClassesFromVC(child, names, seen);
    if (vc.presentedViewController) NGDCollectPageClassesFromVC(vc.presentedViewController, names, seen);
}

static NSDictionary *NGDDebugMetadata(UIWindow *targetWindow, CGPoint point) {
    return @{
        @"generatedAt": NGDObjectDesc(NSDate.date),
        @"naiwaguoVersion": NGD_VERSION,
        @"bundleIdentifier": NSBundle.mainBundle.bundleIdentifier ?: @"",
        @"bundleName": NSBundle.mainBundle.infoDictionary[@"CFBundleName"] ?: @"",
        @"appVersion": NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"",
        @"buildVersion": NSBundle.mainBundle.infoDictionary[(NSString *)kCFBundleVersionKey] ?: @"",
        @"systemName": UIDevice.currentDevice.systemName ?: @"",
        @"systemVersion": UIDevice.currentDevice.systemVersion ?: @"",
        @"deviceModel": UIDevice.currentDevice.model ?: @"",
        @"screenBounds": NGDRectDict(UIScreen.mainScreen.bounds),
        @"tapPointInTargetWindow": NGDPointDict(point),
        @"targetWindow": targetWindow ? NGDWindowJSON(targetWindow) : @{}
    };
}

// 音频专项导出只用得上元信息和截图。整棵 view tree 有几 MB，
// 采到再丢等于让主线程白跑一趟，所以单开这条轻量路径。
NGDDebugExportContext *NGDDebugCaptureMetadataContext(UIWindow *targetWindow) {
    NGDDebugExportContext *context = [NGDDebugExportContext new];
    context.metadata = NGDDebugMetadata(targetWindow, CGPointZero);
    context.screenshotPNG = NGDScreenshotPNG(targetWindow);
    context.sourceView = targetWindow;
    context.presenter = NGDDebugTopPresenter(targetWindow);
    return context;
}

NGDDebugExportContext *NGDDebugCaptureContext(UIWindow *targetWindow, CGPoint point, UIView *selectedView) {
    NGDDebugExportContext *context = [NGDDebugExportContext new];
    NSArray<UIWindow *> *windows = NGDDebugActiveWindows();
    NSMutableArray *windowsJSON = [NSMutableArray array];
    NSMutableArray *treeJSON = [NSMutableArray array];
    NSMutableArray *layersJSON = [NSMutableArray array];
    NSMutableString *treeText = [NSMutableString string];
    NSMutableString *vcText = [NSMutableString string];
    NSHashTable *seenVCs = [NSHashTable weakObjectsHashTable];
    NSMutableSet<NSString *> *pageClasses = [NSMutableSet set];
    NSHashTable *seenClassVCs = [NSHashTable weakObjectsHashTable];

    for (UIWindow *window in windows) {
        [windowsJSON addObject:NGDWindowJSON(window)];
        [treeJSON addObject:NGDViewTreeJSON(window, 0, layersJSON)];
        NGDAppendViewTreeText(window, 0, treeText);
        NGDCollectPageClassesFromView(window, pageClasses);
        NGDCollectPageClassesFromVC(window.rootViewController, pageClasses, seenClassVCs);
        if (window.rootViewController) {
            [vcText appendFormat:@"Window %@ %@\n", NGDClassName(window), NGDStringFromPointer((__bridge const void *)window)];
            NGDAppendVCText(window.rootViewController, 1, vcText, seenVCs);
        }
    }

    UIViewController *nearestVC = NGDViewControllerForView(selectedView);
    NSDictionary *selected = @{
        @"tapPointInWindow": NGDPointDict(point),
        @"hitView": NGDBaseViewInfo(selectedView ?: targetWindow, 0),
        @"nearestViewController": nearestVC ? @{
            @"class": NGDClassName(nearestVC),
            @"address": NGDStringFromPointer((__bridge const void *)nearestVC)
        } : @{},
        @"ancestorChain": selectedView ? NGDAncestorChain(selectedView) : @[]
    };

    NSString *summary = [NSString stringWithFormat:
                         @"Hit: %@ %@\nFrame: %@\nBounds: %@\nNearest VC: %@ %@\nWindows: %lu\nSubviews: %lu",
                         NGDClassName(selectedView),
                         NGDStringFromPointer((__bridge const void *)selectedView),
                         selectedView ? NSStringFromCGRect(selectedView.frame) : @"",
                         selectedView ? NSStringFromCGRect(selectedView.bounds) : @"",
                         nearestVC ? NGDClassName(nearestVC) : @"",
                         nearestVC ? NGDStringFromPointer((__bridge const void *)nearestVC) : @"",
                         (unsigned long)windows.count,
                         (unsigned long)selectedView.subviews.count];

    context.metadata = NGDDebugMetadata(targetWindow, point);
    context.windowsJSON = windowsJSON;
    context.viewTreeJSON = treeJSON;
    context.viewTreeText = treeText;
    context.selectedViewJSON = selected;
    context.viewControllersText = vcText;
    context.layersJSON = layersJSON;
    context.screenshotPNG = NGDScreenshotPNG(targetWindow);
    context.summary = summary;
    context.pageClassNames = pageClasses.allObjects;
    context.sourceView = targetWindow;
    context.presenter = NGDDebugTopPresenter(targetWindow);
    return context;
}
