#import "Debug/NGDDebugInspector.h"
#import "Debug/NGDDebugCapture.h"
#import "Debug/NGDAudioProbe.h"
#import "Debug/NGDAudioHooks.h"
#import "Debug/NGDAudioRuntime.h"
#import "Debug/NGDTabBarProbe.h"
#import <dlfcn.h>
void NGDRegisterFLEXKingProvider(void);
#import <CoreText/CoreText.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <UIKit/UIKit.h>
#import <PhotosUI/PhotosUI.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <zlib.h>
#import <sys/utsname.h>
#import <sys/resource.h>
#import <math.h>

static NSString *const NGPluginName = @"NaiwaGuo";
static NSString *const NGVersion = @"0.0.7";
static NSString *const NGHostBundleIdentifier = @"com.phoenix.video";
static NSString *const NGHostExecutable = @"Eggplant";
static NSString *const NGPrefix = @"com.naiwaguo.";
static NSString *const NGPreferenceHUD = @"com.naiwaguo.debug.hud";
static NSString *const NGPreferencePerformance = @"com.naiwaguo.debug.performance";
static NSString *const NGPreferencePlaybackGlass = @"com.naiwaguo.playback.rightGlass";
static NSString *const NGPreferenceCommentGlass = @"com.naiwaguo.playback.commentGlass";
static NSString *const NGPreferenceCommentGlassOpacity = @"com.naiwaguo.playback.commentGlassOpacity";
static NSString *const NGPreferenceShareGlass = @"com.naiwaguo.playback.shareGlass";
static NSString *const NGPreferencePlaybackFullscreen = @"com.naiwaguo.playback.fullscreen";
static NSString *const NGPreferencePlaybackHeaderGlass = @"com.naiwaguo.playback.headerGlass";
static NSString *const NGPreferenceChannelRestoreMigration = @"com.naiwaguo.migration.restoreChannels.070";
static NSString *const NGPreferenceRuntimeGlass = @"com.naiwaguo.appearance.runtimeGlass";
static NSString *const NGIdentifierPrefix = @"NaiwaGuo.";

#include "NGAccessState.inc"
#include "NGAuthorAccess.inc"

static BOOL NGRuntimeGlassRequested;
static BOOL NGRuntimeGlassHooksInstalled;
static NSDictionary *NGOriginalHostInfo;

static void NGPrepareRuntimeGlass(void) {
    if (!NGOriginalHostInfo) NGOriginalHostInfo = [NSBundle.mainBundle.infoDictionary copy];
    // Compatibility is automatic only on systems with native Liquid Glass.
    // Individual plugin features still default to off and keep their own switches.
    if (@available(iOS 26.0, *)) {} else return;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSBundle *host = NSBundle.mainBundle;
        NSDictionary *originalInfo = host.infoDictionary;
        if (![host.bundleIdentifier isEqualToString:NGHostBundleIdentifier] &&
            ![originalInfo[@"CFBundleExecutable"] isEqualToString:NGHostExecutable]) return;
        NGOriginalHostInfo = [originalInfo copy];
        NGRuntimeGlassRequested = YES;

        // Override only main-bundle lookups. UIKit may already have cached the original value.
        NSMutableDictionary *replacement = [originalInfo mutableCopy];
        replacement[@"UIDesignRequiresCompatibility"] = @NO;
        NSDictionary *overriddenInfo = [replacement copy];
        Method dictionaryMethod = class_getInstanceMethod(NSBundle.class, @selector(infoDictionary));
        Method keyMethod = class_getInstanceMethod(NSBundle.class, @selector(objectForInfoDictionaryKey:));
        IMP dictionaryOriginal = method_getImplementation(dictionaryMethod);
        IMP keyOriginal = method_getImplementation(keyMethod);
        method_setImplementation(dictionaryMethod, imp_implementationWithBlock(^NSDictionary *(NSBundle *bundle) {
            if (bundle == host) return overriddenInfo;
            return ((NSDictionary *(*)(id, SEL))dictionaryOriginal)(bundle, @selector(infoDictionary));
        }));
        method_setImplementation(keyMethod, imp_implementationWithBlock(^id(NSBundle *bundle, NSString *key) {
            if (bundle == host && [key isEqualToString:@"UIDesignRequiresCompatibility"]) return @NO;
            return ((id (*)(id, SEL, id))keyOriginal)(bundle, @selector(objectForInfoDictionaryKey:), key);
        }));
        NGRuntimeGlassHooksInstalled = YES;
    });
}

static NSDictionary *NGRuntimeGlassStatus(void) {
    return @{
        @"requestedAtLaunch": @(NGRuntimeGlassRequested),
        @"bundleHooksInstalled": @(NGRuntimeGlassHooksInstalled),
        @"originalCompatibility": NGOriginalHostInfo[@"UIDesignRequiresCompatibility"] ?: NSNull.null,
        @"bundleLookupCompatibility": [NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] ?: NSNull.null,
        @"hostInfoPlistModified": @NO,
        @"systemGlassActivationVerified": @NO,
        @"note": @"Bundle lookup override only; does not prove UIKit refreshed any previously cached compatibility state."
    };
}

static void NGUpdatePlaybackHeaderGlass(void);
static void NGUpdatePlaybackGlass(void);
static void NGUpdateDock(void);
static void NGUpdateCommentGlass(void);

static NSString *NGClassName(id object) {
    return object ? NSStringFromClass([object class]) : @"nil";
}

static id NGSafeJSONObject(id object, NSString *path, NSMutableArray *warnings) {
    if ([object isKindOfClass:NSNumber.class]) {
        double value = [object doubleValue];
        if (!isfinite(value)) {
            NSString *replacement = isnan(value) ? @"NaN" : (value > 0 ? @"+Infinity" : @"-Infinity");
            [warnings addObject:@{@"path": path ?: @"$", @"value": replacement}];
            return replacement;
        }
        return object;
    }
    if ([object isKindOfClass:NSDictionary.class]) {
        NSMutableDictionary *result = [NSMutableDictionary dictionary];
        [object enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL *stop) {
            NSString *childPath = [NSString stringWithFormat:@"%@.%@", path ?: @"$", key];
            result[[key description]] = NGSafeJSONObject(value, childPath, warnings);
        }];
        return result;
    }
    if ([object isKindOfClass:NSArray.class]) {
        NSMutableArray *result = [NSMutableArray arrayWithCapacity:[object count]];
        NSUInteger index = 0;
        for (id value in object) {
            [result addObject:NGSafeJSONObject(value, [NSString stringWithFormat:@"%@[%lu]", path ?: @"$", (unsigned long)index++], warnings)];
        }
        return result;
    }
    if (!object || object == NSNull.null) return NSNull.null;
    if ([object isKindOfClass:NSString.class]) return object;
    if ([object isKindOfClass:NSDate.class]) return @([(NSDate *)object timeIntervalSince1970]);
    if ([object isKindOfClass:NSURL.class]) return [(NSURL *)object absoluteString] ?: @"";
    NSString *description = [object description];
    return [NSString stringWithFormat:@"<%@> %@", NGClassName(object), description ?: @""];
}

static id NGSafeJSON(id object) {
    NSMutableArray *warnings = [NSMutableArray array];
    id result = NGSafeJSONObject(object, @"$", warnings);
    if (warnings.count && [result isKindOfClass:NSMutableDictionary.class]) {
        result[@"nonFiniteValues"] = warnings;
    }
    return result;
}

static NSData *NGJSONData(id object) {
    return [NSJSONSerialization dataWithJSONObject:NGSafeJSON(object)
                                           options:(NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys)
                                             error:nil];
}

static NSString *NGTrimmedString(id value, NSUInteger limit) {
    if (![value isKindOfClass:NSString.class]) return nil;
    NSString *text = [(NSString *)value copy];
    if (text.length > limit) text = [[text substringToIndex:limit] stringByAppendingString:@"..."];
    return text;
}

@interface NGEventLog : NSObject
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *events;
@end

@implementation NGEventLog
+ (instancetype)shared {
    static NGEventLog *shared;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ shared = [NGEventLog new]; });
    return shared;
}
- (instancetype)init {
    if ((self = [super init])) _events = [NSMutableArray array];
    return self;
}
- (void)add:(NSString *)category event:(NSString *)event details:(NSDictionary *)details {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ [self add:category event:event details:details]; });
        return;
    }
    NSDictionary *entry = @{
        @"time": @([NSDate.date timeIntervalSince1970]),
        @"uptime": @(NSProcessInfo.processInfo.systemUptime),
        @"category": category ?: @"runtime",
        @"event": event ?: @"",
        @"details": NGSafeJSON(details ?: @{})
    };
    [self.events addObject:entry];
    if (self.events.count > 500) [self.events removeObjectsInRange:NSMakeRange(0, self.events.count - 500)];
}
- (NSArray<NSDictionary *> *)snapshot { return [self.events copy]; }
- (void)clear {
    [self.events removeAllObjects];
    [self add:@"debug" event:@"events-cleared" details:nil];
}
@end

static void NGLog(NSString *category, NSString *event, NSDictionary *details) {
    [[NGEventLog shared] add:category event:event details:details];
}

static UIWindow *NGBaseWindow(void) {
    UIWindow *fallback = nil;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) continue;
        UIWindowScene *windowScene = (UIWindowScene *)scene;
        for (UIWindow *window in windowScene.windows) {
            if ([window.accessibilityIdentifier hasPrefix:NGIdentifierPrefix]) continue;
            if ((scene.activationState == UISceneActivationStateForegroundActive || scene.activationState == UISceneActivationStateForegroundInactive) && window.isKeyWindow) return window;
            if (!fallback && !window.hidden) fallback = window;
        }
    }
    return fallback;
}

static UIViewController *NGDeepestController(UIViewController *controller) {
    UIViewController *current = controller;
    if (!current) return nil;
    while (current.presentedViewController && !current.presentedViewController.isBeingDismissed) current = current.presentedViewController;
    if ([current isKindOfClass:UINavigationController.class]) {
        UIViewController *visible = ((UINavigationController *)current).visibleViewController;
        if (visible && visible != current) return NGDeepestController(visible);
    } else if ([current isKindOfClass:UITabBarController.class]) {
        UIViewController *selected = ((UITabBarController *)current).selectedViewController;
        if (selected && selected != current) return NGDeepestController(selected);
    } else if ([current isKindOfClass:UISplitViewController.class]) {
        UIViewController *last = ((UISplitViewController *)current).viewControllers.lastObject;
        if (last && last != current) return NGDeepestController(last);
    }
    return current;
}

static UIViewController *NGTopController(void) {
    UIWindow *window = NGBaseWindow();
    UIViewController *controller = NGDeepestController(window.rootViewController);
    if (controller.isViewLoaded && controller.view.window) return controller;
    return controller ?: window.rootViewController;
}

static NSString *NGClassName(id object);

static NSString *NGViewPath(UIView *view) {
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    for (UIView *current = view; current; current = current.superview) {
        NSUInteger index = current.superview ? [current.superview.subviews indexOfObject:current] : 0;
        [parts insertObject:[NSString stringWithFormat:@"%@[%lu]", NGClassName(current), (unsigned long)index] atIndex:0];
    }
    return [parts componentsJoinedByString:@"/"];
}

static NSDictionary *NGColorDictionary(CGColorRef color) {
    if (!color) return @{};
    CGFloat red = 0, green = 0, blue = 0, alpha = 0;
    if (![[UIColor colorWithCGColor:color] getRed:&red green:&green blue:&blue alpha:&alpha]) {
        return @{@"description": @"non-RGB"};
    }
    return @{@"red": @(red), @"green": @(green), @"blue": @(blue), @"alpha": @(alpha)};
}

static BOOL NGViewIsVisible(UIView *view) {
    if (!view.window || view.hidden || view.alpha < 0.01) return NO;
    for (UIView *current = view; current; current = current.superview) {
        if (current.hidden || current.alpha < 0.01) return NO;
    }
    return !CGRectIsEmpty([view convertRect:view.bounds toView:view.window]);
}

static NSDictionary *NGViewInfo(UIView *view) {
    if (!view) return @{};
    UIWindow *window = view.window;
    CGRect frame = window ? [view convertRect:view.bounds toView:window] : view.frame;
    CGRect visible = window ? CGRectIntersection(frame, window.bounds) : CGRectZero;
    BOOL ancestorHidden = NO;
    CGFloat effectiveAlpha = 1;
    for (UIView *current = view; current; current = current.superview) {
        ancestorHidden = ancestorHidden || current.hidden;
        effectiveAlpha *= current.alpha;
        if (current.clipsToBounds && window) visible = CGRectIntersection(visible, [current convertRect:current.bounds toView:window]);
    }
    if (CGRectIsNull(visible) || CGRectIsInfinite(visible)) visible = CGRectZero;

    NSMutableArray *constraints = [NSMutableArray array];
    for (NSLayoutConstraint *constraint in view.constraints) {
        if (constraints.count >= 80) break;
        [constraints addObject:@{
            @"firstClass": NGClassName(constraint.firstItem),
            @"firstAttribute": @(constraint.firstAttribute),
            @"relation": @(constraint.relation),
            @"secondClass": NGClassName(constraint.secondItem),
            @"secondAttribute": @(constraint.secondAttribute),
            @"multiplier": @(constraint.multiplier),
            @"constant": @(constraint.constant),
            @"priority": @(constraint.priority),
            @"active": @(constraint.active)
        }];
    }

    NSMutableArray *gestures = [NSMutableArray array];
    for (UIGestureRecognizer *gesture in view.gestureRecognizers) {
        [gestures addObject:@{
            @"class": NGClassName(gesture),
            @"enabled": @(gesture.enabled),
            @"state": @(gesture.state),
            @"cancelsTouches": @(gesture.cancelsTouchesInView),
            @"delaysTouchesBegan": @(gesture.delaysTouchesBegan)
        }];
    }

    NSMutableDictionary *info = [@{
        @"class": NGClassName(view),
        @"superclass": NGClassName(view.superview),
        @"address": [NSString stringWithFormat:@"%p", view],
        @"path": NGViewPath(view),
        @"tag": @(view.tag),
        @"hostIdentifier": NGTrimmedString(view.accessibilityIdentifier, 240) ?: @"",
        @"accessibilityLabel": NGTrimmedString(view.accessibilityLabel, 300) ?: @"",
        @"accessibilityValue": NGTrimmedString(view.accessibilityValue, 300) ?: @"",
        @"frame": NSStringFromCGRect(view.frame),
        @"bounds": NSStringFromCGRect(view.bounds),
        @"windowFrame": NSStringFromCGRect(frame),
        @"visibleFrame": NSStringFromCGRect(visible),
        @"center": NSStringFromCGPoint(view.center),
        @"safeAreaInsets": NSStringFromUIEdgeInsets(view.safeAreaInsets),
        @"layoutMargins": NSStringFromUIEdgeInsets(view.layoutMargins),
        @"transform": NSStringFromCGAffineTransform(view.transform),
        @"hidden": @(view.hidden),
        @"ancestorHidden": @(ancestorHidden),
        @"alpha": @(view.alpha),
        @"effectiveAlpha": @(effectiveAlpha),
        @"visible": @(NGViewIsVisible(view)),
        @"opaque": @(view.opaque),
        @"clipsToBounds": @(view.clipsToBounds),
        @"interaction": @(view.userInteractionEnabled),
        @"accessibilityHidden": @(view.accessibilityElementsHidden),
        @"autoresizingMask": @(view.autoresizingMask),
        @"translatesAutoresizingMask": @(view.translatesAutoresizingMaskIntoConstraints),
        @"ambiguousLayout": @(view.hasAmbiguousLayout),
        @"subviewCount": @(view.subviews.count),
        @"constraintCount": @(view.constraints.count),
        @"constraints": constraints,
        @"gestures": gestures,
        @"viewBackground": NGColorDictionary(view.backgroundColor.CGColor),
        @"layer": @{
            @"class": NGClassName(view.layer),
            @"frame": NSStringFromCGRect(view.layer.frame),
            @"opacity": @(view.layer.opacity),
            @"cornerRadius": @(view.layer.cornerRadius),
            @"zPosition": @(view.layer.zPosition),
            @"masksToBounds": @(view.layer.masksToBounds),
            @"maskClass": NGClassName(view.layer.mask),
            @"background": NGColorDictionary(view.layer.backgroundColor),
            @"borderWidth": @(view.layer.borderWidth),
            @"shadowOpacity": @(view.layer.shadowOpacity)
        }
    } mutableCopy];

    NSString *text = nil;
    if ([view isKindOfClass:UILabel.class]) text = ((UILabel *)view).text;
    else if ([view isKindOfClass:UIButton.class]) text = [(UIButton *)view titleForState:UIControlStateNormal];
    else if ([view isKindOfClass:UITextField.class]) {
        UITextField *field = (UITextField *)view;
        text = field.secureTextEntry ? @"<secure>" : field.text;
    } else if ([view isKindOfClass:UITextView.class]) text = ((UITextView *)view).text;
    if (text) info[@"text"] = NGTrimmedString(text, 500) ?: @"";

    if ([view isKindOfClass:UIImageView.class]) {
        UIImage *image = ((UIImageView *)view).image;
        info[@"image"] = @{@"present": @(image != nil), @"size": NSStringFromCGSize(image.size), @"scale": @(image.scale)};
    }
    if ([view isKindOfClass:UIControl.class]) {
        UIControl *control = (UIControl *)view;
        NSMutableArray *targets = [NSMutableArray array];
        for (id target in control.allTargets) {
            NSArray *actions = [control actionsForTarget:target forControlEvent:UIControlEventTouchUpInside];
            if (actions.count) [targets addObject:@{@"targetClass": NGClassName(target), @"selectors": actions}];
        }
        info[@"control"] = @{@"enabled": @(control.enabled), @"selected": @(control.selected), @"highlighted": @(control.highlighted), @"touchUpInsideTargets": targets};
    }
    if ([view isKindOfClass:UIScrollView.class]) {
        UIScrollView *scroll = (UIScrollView *)view;
        info[@"scroll"] = @{
            @"contentSize": NSStringFromCGSize(scroll.contentSize),
            @"offset": NSStringFromCGPoint(scroll.contentOffset),
            @"inset": NSStringFromUIEdgeInsets(scroll.contentInset),
            @"adjustedInset": NSStringFromUIEdgeInsets(scroll.adjustedContentInset),
            @"enabled": @(scroll.scrollEnabled),
            @"dragging": @(scroll.dragging),
            @"decelerating": @(scroll.decelerating)
        };
    }
    if ([view isKindOfClass:UIVisualEffectView.class]) info[@"effectClass"] = NGClassName(((UIVisualEffectView *)view).effect);
    if (view.layer.animationKeys.count) info[@"layerAnimationKeys"] = view.layer.animationKeys;
    return info;
}

static void NGAppendViewTree(UIView *view, NSMutableArray *nodes, NSUInteger depth) {
    if (!view || nodes.count >= 12000 || depth > 90) return;
    if ([view.accessibilityIdentifier hasPrefix:NGIdentifierPrefix] && ![view.accessibilityIdentifier hasPrefix:@"NaiwaGuo.Playback."]) return;
    [nodes addObject:NGViewInfo(view)];
    for (UIView *child in view.subviews) NGAppendViewTree(child, nodes, depth + 1);
}

static NSDictionary *NGControllerInfo(UIViewController *controller, NSUInteger depth, NSHashTable *visited) {
    if (!controller || depth > 32) return @{};
    if ([visited containsObject:controller]) return @{@"class": NGClassName(controller), @"reference": [NSString stringWithFormat:@"%p", controller]};
    [visited addObject:controller];
    NSMutableArray *children = [NSMutableArray array];
    for (UIViewController *child in controller.childViewControllers) [children addObject:NGControllerInfo(child, depth + 1, visited)];
    NSMutableDictionary *info = [@{
        @"class": NGClassName(controller),
        @"address": [NSString stringWithFormat:@"%p", controller],
        @"title": NGTrimmedString(controller.title, 300) ?: @"",
        @"loaded": @(controller.isViewLoaded),
        @"parent": NGClassName(controller.parentViewController),
        @"presenting": NGClassName(controller.presentingViewController),
        @"isBeingPresented": @(controller.isBeingPresented),
        @"isBeingDismissed": @(controller.isBeingDismissed),
        @"children": children,
        @"additionalSafeAreaInsets": NSStringFromUIEdgeInsets(controller.additionalSafeAreaInsets)
    } mutableCopy];
    if (controller.isViewLoaded) {
        info[@"viewFrame"] = NSStringFromCGRect(controller.view.frame);
        info[@"viewWindowFrame"] = controller.view.window ? NSStringFromCGRect([controller.view convertRect:controller.view.bounds toView:controller.view.window]) : @"none";
    }
    if ([controller isKindOfClass:UINavigationController.class]) info[@"navigationStack"] = [((UINavigationController *)controller).viewControllers valueForKey:@"class"];
    if ([controller isKindOfClass:UITabBarController.class]) info[@"selectedTab"] = @(((UITabBarController *)controller).selectedIndex);
    if (controller.presentedViewController) info[@"presented"] = NGControllerInfo(controller.presentedViewController, depth + 1, visited);
    return info;
}

static NSString *NGDeviceIdentifier(void) {
    struct utsname systemInfo;
    if (uname(&systemInfo) != 0) return @"unknown";
    return @(systemInfo.machine);
}

static NSDictionary *NGEnvironment(void) {
    NSBundle *bundle = NSBundle.mainBundle;
    UIDevice *device = UIDevice.currentDevice;
    UIWindow *window = NGBaseWindow();
    NSMutableDictionary *environment = [@{
        @"plugin": NGPluginName,
        @"pluginVersion": NGVersion,
        @"iOS": device.systemVersion,
        @"deviceModel": device.model ?: @"unknown",
        @"deviceIdentifier": NGDeviceIdentifier(),
        @"bundleIdentifier": bundle.bundleIdentifier ?: @"unknown",
        @"appVersion": bundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"unknown",
        @"appBuild": bundle.infoDictionary[@"CFBundleVersion"] ?: @"unknown",
        @"executable": bundle.infoDictionary[@"CFBundleExecutable"] ?: @"unknown",
        @"designRequiresCompatibility": bundle.infoDictionary[@"UIDesignRequiresCompatibility"] ?: @NO,
        @"hostSDK": bundle.infoDictionary[@"DTSDKName"] ?: @"unknown",
        @"runtimeGlass": NGRuntimeGlassStatus(),
        @"screenPoints": window ? NSStringFromCGSize(window.bounds.size) : @"unknown",
        @"screenPixels": window ? NSStringFromCGSize(CGSizeMake(window.bounds.size.width * window.screen.scale, window.bounds.size.height * window.screen.scale)) : @"unknown",
        @"screenScale": @(window.screen.scale ?: UIScreen.mainScreen.scale),
        @"maximumFramesPerSecond": @(window.screen.maximumFramesPerSecond ?: 60),
        @"nativeScale": @(window.screen.nativeScale ?: UIScreen.mainScreen.nativeScale),
        @"reduceMotion": @(UIAccessibilityIsReduceMotionEnabled()),
        @"reduceTransparency": @(UIAccessibilityIsReduceTransparencyEnabled()),
        @"boldText": @(UIAccessibilityIsBoldTextEnabled()),
        @"darkerColors": @(UIAccessibilityDarkerSystemColorsEnabled()),
        @"contentSizeCategory": window.traitCollection.preferredContentSizeCategory ?: @"unknown",
        @"userInterfaceStyle": @(window.traitCollection.userInterfaceStyle),
        @"lowPowerMode": @(NSProcessInfo.processInfo.lowPowerModeEnabled),
        @"thermalState": @(NSProcessInfo.processInfo.thermalState),
        @"processUptime": @(NSProcessInfo.processInfo.systemUptime)
    } mutableCopy];
    if (window.windowScene) {
        environment[@"interfaceOrientation"] = @(window.windowScene.interfaceOrientation);
        environment[@"sceneActivationState"] = @(window.windowScene.activationState);
    }
    return environment;
}

static NSDictionary *NGViewport(void) {
    UIWindow *window = NGBaseWindow();
    if (!window) return @{};
    UIViewController *controller = NGTopController();
    return @{
        @"keyWindowClass": NGClassName(window),
        @"windowBounds": NSStringFromCGRect(window.bounds),
        @"safeAreaInsets": NSStringFromUIEdgeInsets(window.safeAreaInsets),
        @"rootController": NGClassName(window.rootViewController),
        @"visibleController": NGClassName(controller),
        @"topViewControllerTitle": NGTrimmedString(controller.title, 300) ?: @""
    };
}

static NSDictionary *NGPreferenceSnapshot(void) {
    NSDictionary *all = NSUserDefaults.standardUserDefaults.dictionaryRepresentation;
    NSMutableDictionary *filtered = [NSMutableDictionary dictionary];
    [all enumerateKeysAndObjectsUsingBlock:^(NSString *key, id value, BOOL *stop) {
        if ([key hasPrefix:NGPrefix]) filtered[key] = NGSafeJSON(value);
    }];
    return filtered;
}

@interface NGPerformanceRecorder : NSObject
@property(nonatomic, strong) CADisplayLink *displayLink;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *seconds;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *events;
@property(nonatomic, strong) NSDate *startedAt;
@property(nonatomic, copy) NSString *state;
@property(nonatomic, copy) NSString *lastPage;
@property(nonatomic) CFTimeInterval started;
@property(nonatomic) CFTimeInterval bucketStart;
@property(nonatomic) CFTimeInterval lastFrame;
@property(nonatomic) CFTimeInterval activeSeconds;
@property(nonatomic) NSUInteger frames;
@property(nonatomic) NSUInteger over33;
@property(nonatomic) NSUInteger over50;
@property(nonatomic) NSUInteger over100;
@property(nonatomic) double maximumGap;
@property(nonatomic) double lastCPU;
@property(nonatomic) double lastCPUAt;
- (void)start;
- (void)stopWithReason:(NSString *)reason;
- (void)pause;
- (void)resume;
- (NSDictionary *)snapshot;
@end

@implementation NGPerformanceRecorder
+ (instancetype)shared {
    static NGPerformanceRecorder *shared;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ shared = [NGPerformanceRecorder new]; });
    return shared;
}
- (instancetype)init {
    if ((self = [super init])) {
        _seconds = [NSMutableArray array];
        _events = [NSMutableArray array];
        _state = @"idle";
    }
    return self;
}
- (void)recordEvent:(NSString *)event details:(NSDictionary *)details {
    if (self.events.count >= 1000) [self.events removeObjectAtIndex:0];
    [self.events addObject:@{
        @"elapsedSeconds": @(MAX(0, CACurrentMediaTime() - self.started)),
        @"event": event ?: @"",
        @"details": details ?: @{}
    }];
}
- (double)processCPUTime {
    struct rusage usage = {0};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return -1;
    return usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6 + usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
}
- (void)start {
    if (self.displayLink) [self.displayLink invalidate];
    [self.seconds removeAllObjects];
    [self.events removeAllObjects];
    self.startedAt = NSDate.date;
    self.started = self.bucketStart = CACurrentMediaTime();
    self.lastFrame = 0;
    self.activeSeconds = 0;
    self.frames = self.over33 = self.over50 = self.over100 = 0;
    self.maximumGap = 0;
    self.lastCPU = [self processCPUTime];
    self.lastCPUAt = self.started;
    self.lastPage = NGClassName(NGTopController());
    self.state = @"recording";
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick:)];
    [self.displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    [self recordEvent:@"start" details:@{@"page": self.lastPage ?: @"unknown"}];
    NGLog(@"performance", @"started", nil);
}
- (void)stopWithReason:(NSString *)reason {
    if (self.displayLink) [self flushAt:CACurrentMediaTime()];
    if (!self.displayLink && ![self.state isEqualToString:@"paused"]) return;
    [self.displayLink invalidate];
    self.displayLink = nil;
    self.state = @"stopped";
    [self recordEvent:@"stop" details:@{@"reason": reason ?: @"user"}];
    NGLog(@"performance", @"stopped", @{@"reason": reason ?: @"user"});
}
- (void)pause {
    if (!self.displayLink || ![self.state isEqualToString:@"recording"]) return;
    [self flushAt:CACurrentMediaTime()];
    [self.displayLink invalidate];
    self.displayLink = nil;
    self.state = @"paused";
    [self recordEvent:@"pause" details:@{@"reason": @"background"}];
}
- (void)resume {
    if (![self.state isEqualToString:@"paused"]) return;
    self.bucketStart = self.lastFrame = CACurrentMediaTime();
    self.frames = self.over33 = self.over50 = self.over100 = 0;
    self.maximumGap = 0;
    self.lastCPU = [self processCPUTime];
    self.lastCPUAt = self.bucketStart;
    self.state = @"recording";
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick:)];
    [self.displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    [self recordEvent:@"resume" details:nil];
}
- (void)tick:(CADisplayLink *)link {
    CFTimeInterval now = CACurrentMediaTime();
    double gap = self.lastFrame > 0 ? now - self.lastFrame : 0;
    self.lastFrame = now;
    self.frames++;
    self.maximumGap = MAX(self.maximumGap, gap * 1000);
    self.over33 += gap > 0.033;
    self.over50 += gap > 0.050;
    self.over100 += gap > 0.100;
    NSString *page = NGClassName(NGTopController());
    if (page && ![page isEqualToString:self.lastPage]) {
        self.lastPage = page;
        [self recordEvent:@"page" details:@{@"class": page}];
    }
    if (now - self.bucketStart >= 1.0) [self flushAt:now];
    if (self.activeSeconds >= 600) {
        [self stopWithReason:@"duration_limit"];
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:NGPreferencePerformance];
    }
}
- (void)flushAt:(CFTimeInterval)end {
    double duration = end - self.bucketStart;
    if (duration <= 0.05) return;
    double nowCPU = [self processCPUTime];
    double cpuPercent = (self.lastCPUAt > 0 && self.lastCPU >= 0 && nowCPU >= self.lastCPU) ? 100.0 * (nowCPU - self.lastCPU) / duration : -1;
    self.lastCPU = nowCPU;
    self.lastCPUAt = end;
    [self.seconds addObject:@{
        @"elapsedSeconds": @(MAX(0, self.bucketStart - self.started)),
        @"durationSeconds": @(duration),
        @"callbackCount": @(self.frames),
        @"callbackFPS": @(self.frames / duration),
        @"maximumCallbackGapMs": @(self.maximumGap),
        @"gapsOver33ms": @(self.over33),
        @"gapsOver50ms": @(self.over50),
        @"gapsOver100ms": @(self.over100),
        @"cpuPercent": @(cpuPercent),
        @"pageClass": self.lastPage ?: @"unknown"
    }];
    if (self.seconds.count > 600) [self.seconds removeObjectsInRange:NSMakeRange(0, self.seconds.count - 600)];
    self.activeSeconds += duration;
    self.bucketStart = end;
    self.frames = self.over33 = self.over50 = self.over100 = 0;
    self.maximumGap = 0;
}
- (NSDictionary *)snapshot {
    return @{
        @"schemaVersion": @1,
        @"state": self.state ?: @"idle",
        @"startedAt": self.startedAt ? @([self.startedAt timeIntervalSince1970]) : NSNull.null,
        @"activeSeconds": @(self.activeSeconds),
        @"environment": NGEnvironment(),
        @"page": self.lastPage ?: @"unknown",
        @"seconds": [self.seconds copy],
        @"events": [self.events copy],
        @"metricNotes": @[
            @"callbackFPS is display-link callback rate, not physical panel refresh rate.",
            @"CPU percentage is for the whole host process and may exceed 100 percent on multicore devices.",
            @"Sampling pauses when the app enters the background."
        ]
    };
}

@end

static void NGSetPerformanceEnabled(BOOL enabled) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    [defaults setBool:enabled forKey:NGPreferencePerformance];
    NGPerformanceRecorder *recorder = NGPerformanceRecorder.shared;
    if (enabled) {
        if (!recorder.displayLink) [recorder start];
    } else if (recorder.displayLink || [recorder.state isEqualToString:@"paused"]) {
        [recorder stopWithReason:@"setting_disabled"];
    }
}

#ifdef NG_TEST
// Historical in-memory export fixture, excluded from the shipping plugin.
static void NGZipAppend16(NSMutableData *data, uint16_t value) {
    uint16_t little = CFSwapInt16HostToLittle(value);
    [data appendBytes:&little length:2];
}

static void NGZipAppend32(NSMutableData *data, uint32_t value) {
    uint32_t little = CFSwapInt32HostToLittle(value);
    [data appendBytes:&little length:4];
}

static NSData *NGCreateZip(NSDictionary<NSString *, NSData *> *files) {
    NSMutableData *zip = [NSMutableData data];
    NSMutableData *central = [NSMutableData data];
    NSArray<NSString *> *names = [files.allKeys sortedArrayUsingSelector:@selector(compare:)];
    for (NSString *name in names) {
        NSData *filename = [name dataUsingEncoding:NSUTF8StringEncoding];
        NSData *body = files[name];
        uLong crc = crc32(0, body.bytes, (uInt)body.length);
        uint32_t offset = (uint32_t)zip.length;
        uint32_t size = (uint32_t)body.length;
        NGZipAppend32(zip, 0x04034b50);
        NGZipAppend16(zip, 20);
        NGZipAppend16(zip, 0x0800);
        NGZipAppend16(zip, 0);
        NGZipAppend16(zip, 0);
        NGZipAppend16(zip, 0x21);
        NGZipAppend32(zip, (uint32_t)crc);
        NGZipAppend32(zip, size);
        NGZipAppend32(zip, size);
        NGZipAppend16(zip, (uint16_t)filename.length);
        NGZipAppend16(zip, 0);
        [zip appendData:filename];
        [zip appendData:body];

        NGZipAppend32(central, 0x02014b50);
        NGZipAppend16(central, 20);
        NGZipAppend16(central, 20);
        NGZipAppend16(central, 0x0800);
        NGZipAppend16(central, 0);
        NGZipAppend16(central, 0);
        NGZipAppend16(central, 0x21);
        NGZipAppend32(central, (uint32_t)crc);
        NGZipAppend32(central, size);
        NGZipAppend32(central, size);
        NGZipAppend16(central, (uint16_t)filename.length);
        NGZipAppend16(central, 0);
        NGZipAppend16(central, 0);
        NGZipAppend16(central, 0);
        NGZipAppend16(central, 0);
        NGZipAppend32(central, 0);
        NGZipAppend32(central, offset);
        [central appendData:filename];
    }
    uint32_t centralOffset = (uint32_t)zip.length;
    [zip appendData:central];
    NGZipAppend32(zip, 0x06054b50);
    NGZipAppend16(zip, 0);
    NGZipAppend16(zip, 0);
    NGZipAppend16(zip, (uint16_t)names.count);
    NGZipAppend16(zip, (uint16_t)names.count);
    NGZipAppend32(zip, (uint32_t)central.length);
    NGZipAppend32(zip, centralOffset);
    NGZipAppend16(zip, 0);
    return zip;
}

#endif

static NSArray<UIWindow *> *NGCaptureWindows(void) {
    UIWindow *base = NGBaseWindow();
    if (!base.windowScene) return base ? @[base] : @[];
    NSMutableArray *windows = [NSMutableArray array];
    for (UIWindow *window in base.windowScene.windows) {
        if (window.hidden || window.alpha < 0.01) continue;
        if ([window.accessibilityIdentifier hasPrefix:NGIdentifierPrefix]) continue;
        [windows addObject:window];
    }
    [windows sortUsingComparator:^NSComparisonResult(UIWindow *left, UIWindow *right) {
        if (left.windowLevel < right.windowLevel) return NSOrderedAscending;
        if (left.windowLevel > right.windowLevel) return NSOrderedDescending;
        return NSOrderedSame;
    }];
    return windows;
}

static NSDictionary *NGWindowInfo(UIWindow *window) {
    return @{
        @"class": NGClassName(window),
        @"key": @(window.isKeyWindow),
        @"hidden": @(window.hidden),
        @"alpha": @(window.alpha),
        @"windowLevel": @(window.windowLevel),
        @"bounds": NSStringFromCGRect(window.bounds),
        @"safeAreaInsets": NSStringFromUIEdgeInsets(window.safeAreaInsets),
        @"rootController": NGClassName(window.rootViewController)
    };
}

static NSData *NGCaptureScreenshot(void) {
    UIWindow *base = NGBaseWindow();
    if (!base) return nil;
    NSArray<UIWindow *> *windows = NGCaptureWindows();
    UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat defaultFormat];
    format.opaque = NO;
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:base.bounds.size format:format];
    return [renderer PNGDataWithActions:^(UIGraphicsImageRendererContext *context) {
        for (UIWindow *window in windows) {
            CGContextSaveGState(context.CGContext);
            CGRect rect = [base convertRect:window.bounds fromWindow:window];
            CGContextTranslateCTM(context.CGContext, rect.origin.x, rect.origin.y);
            [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:NO];
            CGContextRestoreGState(context.CGContext);
        }
    }];
}

static NSArray *NGHitTestSamples(NSArray<UIWindow *> *windows) {
    NSMutableArray *samples = [NSMutableArray array];
    for (UIWindow *window in windows) {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
        for (NSUInteger index = 0; index < queue.count && index < 12000; index++) {
            UIView *view = queue[index];
            [queue addObjectsFromArray:view.subviews];
            if (![view isKindOfClass:UIControl.class] || !NGViewIsVisible(view)) continue;
            CGPoint point = [view convertPoint:CGPointMake(CGRectGetMidX(view.bounds), CGRectGetMidY(view.bounds)) toView:window];
            UIView *hit = nil;
            for (UIWindow *candidate in windows.reverseObjectEnumerator) {
                CGPoint candidatePoint = [candidate convertPoint:point fromWindow:window];
                hit = [candidate hitTest:candidatePoint withEvent:nil];
                if (hit) break;
            }
            if (samples.count >= 1000) break;
            [samples addObject:@{
                @"control": NGViewPath(view),
                @"point": NSStringFromCGPoint(point),
                @"hit": hit ? NGViewPath(hit) : @"none",
                @"reachesControl": @(hit == view || [hit isDescendantOfView:view])
            }];
        }
    }
    return samples;
}

static NSDictionary *NGCaptureSnapshot(void) {
    UIWindow *base = NGBaseWindow();
    NSMutableArray *views = [NSMutableArray array];
    for (UIWindow *window in NGCaptureWindows()) NGAppendViewTree(window, views, 0);
    UIViewController *top = NGTopController();
    NSArray<NSDictionary *> *events = [NGEventLog.shared snapshot];
    NSMutableArray *windows = [NSMutableArray array];
    for (UIWindow *window in NGCaptureWindows()) [windows addObject:NGWindowInfo(window)];
    return @{
        @"schemaVersion": @1,
        @"plugin": NGPluginName,
        @"version": NGVersion,
        @"capturedAt": @([NSDate.date timeIntervalSince1970]),
        @"environment": NGEnvironment(),
        @"viewport": NGViewport(),
        @"controllers": NGControllerInfo(base.rootViewController, 0, [NSHashTable weakObjectsHashTable]),
        @"visibleController": NGClassName(top),
        @"windows": windows,
        @"views": views,
        @"viewsCount": @(views.count),
        @"hitTests": NGHitTestSamples(NGCaptureWindows()),
        @"events": events,
        @"preferences": NGPreferenceSnapshot(),
        @"performance": [NGPerformanceRecorder.shared snapshot],
        @"responsiveLayoutConstraints": (@{
            @"note": @"Review constraints and ambiguousLayout in views when diagnosing sidebar or safe-area movement."
        }),
        @"privacy": @"Explicit local capture. The archive can contain visible text and a screenshot. No automatic upload."
    };
}

#ifdef NG_TEST
static NSURL *NGCreateCaptureArchive(void) {
    if (!NSThread.isMainThread) {
        __block NSURL *url = nil;
        dispatch_sync(dispatch_get_main_queue(), ^{ url = NGCreateCaptureArchive(); });
        return url;
    }
    CFTimeInterval start = CACurrentMediaTime();
    NSDictionary *snapshot = NGCaptureSnapshot();
    NSMutableDictionary<NSString *, NSData *> *files = [NSMutableDictionary dictionary];
    NSData *summary = NGJSONData(snapshot);
    if (summary) files[@"NaiwaGuo-diagnostics.json"] = summary;
    NSData *preferences = NGJSONData(@{
        @"preferences": NGPreferenceSnapshot(),
        @"performance": [NGPerformanceRecorder.shared snapshot]
    });
    if (preferences) files[@"debug-settings.json"] = preferences;
    NSData *events = NGJSONData([NGEventLog.shared snapshot]);
    if (events) files[@"events.json"] = events;
    NSMutableString *hierarchy = [NSMutableString string];
    for (NSDictionary *node in snapshot[@"views"] ?: @[]) {
        [hierarchy appendFormat:@"%@ %@ visible=%@ interaction=%@ alpha=%@\n%@\n",
            node[@"class"] ?: @"?", node[@"windowFrame"] ?: @"?", node[@"visible"] ?: @"?",
            node[@"interaction"] ?: @"?", node[@"effectiveAlpha"] ?: @"?", node[@"path"] ?: @"?"];
    }
    files[@"view-hierarchy.txt"] = [hierarchy dataUsingEncoding:NSUTF8StringEncoding];
    NSData *screenshot = NGCaptureScreenshot();
    if (screenshot) files[@"page.png"] = screenshot;
    NSString *readme = [NSString stringWithFormat:
        @"NaiwaGuo local diagnostics\n"
         "version: %@\n"
         "captureMilliseconds: %.1f\n\n"
         "NaiwaGuo-diagnostics.json: environment, controller tree, view tree, constraints, gestures, controls, hit tests, settings and events.\n"
         "view-hierarchy.txt: compact searchable index.\n"
         "page.png: composited visible windows; the plugin overlay is skipped.\n"
         "events.json: recent page and plugin events.\n"
         "debug-settings.json: current NaiwaGuo preferences and performance samples.\n\n"
         "The archive may contain visible account or page text. Review it before sharing. Nothing is uploaded automatically.\n",
        NGVersion, (CACurrentMediaTime() - start) * 1000];
    files[@"README.txt"] = [readme dataUsingEncoding:NSUTF8StringEncoding];
    NSData *zip = NGCreateZip(files);
    NSString *name = [NSString stringWithFormat:@"NaiwaGuo-capture-%@.zip", NSUUID.UUID.UUIDString];
    NSURL *url = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:name]];
    NSError *error = nil;
    if (!zip || ![zip writeToURL:url options:NSDataWritingAtomic error:&error]) {
        NGLog(@"capture", @"write-failed", @{@"error": error.localizedDescription ?: @"unknown"});
        return nil;
    }
    NGLog(@"capture", @"archive-created", @{@"views": snapshot[@"viewsCount"] ?: @0, @"screenshot": @(screenshot != nil)});
    return url;
}

#endif

static char NGIconPickerCoordinatorAssociation;
static UIImage *NGIconImageOnCanvas(UIImage *source, CGFloat canvasSide, CGFloat contentSide) {
    if (!source || source.size.width <= 0 || source.size.height <= 0) return nil;
    CGFloat ratio = MIN(contentSide / source.size.width, contentSide / source.size.height);
    CGSize size = CGSizeMake(source.size.width * ratio, source.size.height * ratio);
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(canvasSide, canvasSide)];
    return [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        [source drawInRect:CGRectMake((canvasSide - size.width) / 2, (canvasSide - size.height) / 2, size.width, size.height)];
    }];
}
@interface NGIconPickerCoordinator : NSObject <PHPickerViewControllerDelegate>
@property(nonatomic, weak) UIViewController *presenter;
@property(nonatomic, copy) NSString *preferenceKey;
@end
@implementation NGIconPickerCoordinator
- (void)picker:(PHPickerViewController *)picker didFinishPicking:(NSArray<PHPickerResult *> *)results {
    NSItemProvider *provider = results.firstObject.itemProvider;
    [picker dismissViewControllerAnimated:YES completion:^{
        if (!provider || ![provider canLoadObjectOfClass:UIImage.class]) return;
        [provider loadObjectOfClass:UIImage.class completionHandler:^(id<NSItemProviderReading> object, NSError *error) {
            UIImage *image = [object isKindOfClass:UIImage.class] ? (UIImage *)object : nil;
            NSData *data = image ? UIImagePNGRepresentation(NGIconImageOnCanvas(image, 128, 128)) : nil;
            dispatch_async(dispatch_get_main_queue(), ^{
                if (!data.length) {
                    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"图片未能读取" message:@"请选择已下载到本机的图片后重试。" preferredStyle:UIAlertControllerStyleAlert];
                    [alert addAction:[UIAlertAction actionWithTitle:@"知道了" style:UIAlertActionStyleCancel handler:nil]];
                    [self.presenter presentViewController:alert animated:YES completion:nil];
                    return;
                }
                [NSUserDefaults.standardUserDefaults setObject:data forKey:self.preferenceKey];
                if ([self.preferenceKey containsString:@"playback"]) NGUpdatePlaybackGlass();
                if ([self.preferenceKey containsString:@"dockIconImage"]) NGUpdateDock();
                [self.presenter.view setNeedsLayout];
                if ([self.presenter isKindOfClass:UITableViewController.class]) [((UITableViewController *)self.presenter).tableView reloadData];
            });
        }];
    }];
}
@end

static void NGPresentIconPhotoPicker(UIViewController *presenter, NSString *preferenceKey) {
    if (!presenter || !preferenceKey.length) return;
    PHPickerConfiguration *configuration = [PHPickerConfiguration new];
    configuration.filter = PHPickerFilter.imagesFilter;
    configuration.selectionLimit = 1;
    configuration.preferredAssetRepresentationMode = PHPickerConfigurationAssetRepresentationModeCurrent;
    PHPickerViewController *picker = [[PHPickerViewController alloc] initWithConfiguration:configuration];
    NGIconPickerCoordinator *coordinator = [NGIconPickerCoordinator new];
    coordinator.presenter = presenter; coordinator.preferenceKey = preferenceKey; picker.delegate = coordinator;
    objc_setAssociatedObject(presenter, &NGIconPickerCoordinatorAssociation, coordinator, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [presenter presentViewController:picker animated:YES completion:nil];
}

static NSString *const NGPreferencePlaybackRailX = @"com.naiwaguo.playback.rail.x";
static NSString *const NGPreferencePlaybackRailY = @"com.naiwaguo.playback.rail.y";
static NSString *const NGPreferencePlaybackRailScale = @"com.naiwaguo.playback.rail.scale";
static BOOL NGPlaybackElementHidden(NSString *identifier) { return [NSUserDefaults.standardUserDefaults boolForKey:[@"com.naiwaguo.playback.hidden." stringByAppendingString:identifier]]; }
static void NGUpdatePlaybackTextLayout(void);
static void NGUpdatePlaybackHeaderGlass(void);
static void NGRefreshPlaybackActionAppearance(UIView *item);

#include "NGSettingsStyle.inc"
#include "NGTypography.inc"
#include "NGPlaybackPresets.inc"

@interface NGPlaybackGlassView : UIVisualEffectView
@property(nonatomic) CGFloat preferredCornerRadius;
@end

@implementation NGPlaybackGlassView
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithEffect:nil])) {
        self.frame = frame;
        self.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified;
        self.accessibilityIdentifier = @"NaiwaGuo.Playback.RightGlass";
        self.accessibilityElementsHidden = YES;
        self.userInteractionEnabled = NO;
        self.clipsToBounds = YES;
        self.layer.cornerCurve = kCACornerCurveContinuous;
    }
    return self;
}
- (void)setFrame:(CGRect)frame {
    [super setFrame:frame];
    if (!self.effect && CGRectGetWidth(frame) >= 8 && CGRectGetHeight(frame) >= 8) {
        if (@available(iOS 26.0, *)) { UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear]; effect.interactive = NO; self.effect = effect; }
        else self.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial];
    }
}
- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat radius = self.preferredCornerRadius > 0 ? self.preferredCornerRadius : MIN(CGRectGetWidth(self.bounds), CGRectGetHeight(self.bounds)) / 2.0;
    self.layer.cornerRadius = radius;
    if (@available(iOS 26.0, *)) self.cornerConfiguration = [UICornerConfiguration capsuleConfiguration];
}
@end

static char NGPlaybackRailTransformAssociation;
static char NGPlaybackRailItemAssociation;
@interface NGPlaybackRailTransformState : NSObject
@property(nonatomic) CGAffineTransform source;
@property(nonatomic) CGAffineTransform applied;
@end
@implementation NGPlaybackRailTransformState @end
@interface NGPlaybackRailItemState : NSObject
@property(nonatomic) CGRect sourceFrame;
@property(nonatomic) CGRect appliedFrame;
@property(nonatomic) BOOL sourceHidden;
@property(nonatomic) BOOL appliedHidden;
@property(nonatomic, strong) id sourceBackgroundColor;
@property(nonatomic, strong) id sourceLayerBackgroundColor;
@property(nonatomic, strong) UIColor *appliedBackgroundColor;
@property(nonatomic, strong) UIColor *appliedLayerBackgroundColor;
@property(nonatomic) CGFloat sourceCornerRadius;
@property(nonatomic) CGFloat appliedCornerRadius;
@property(nonatomic) BOOL appearanceManaged;
@end
@implementation NGPlaybackRailItemState @end
static NSString *NGPlaybackRailItemIdentifier(UIView *view) {
    NSString *name = NGClassName(view);
    if ([name containsString:@"InteractionAvatarView"]) return @"avatar";
    if ([name containsString:@"InteractionCollectView"]) return @"collect";
    if ([name containsString:@"InteractionCommentView"]) return @"comment";
    if ([name containsString:@"InteractionLikeView"]) return @"like";
    if ([name containsString:@"InteractionShareView"]) return @"share";
    return nil;
}
static void NGApplyPlaybackRailLayout(UIView *rail) {
    NSMutableArray<UIView *> *items = [NSMutableArray array];
    for (UIView *child in rail.subviews) if (NGPlaybackRailItemIdentifier(child)) [items addObject:child];
    NSMutableArray<UIView *> *visible = [NSMutableArray array];
    CGFloat totalHeight = 0; CGFloat spacing = 8; CGFloat baselineBottom = 0;
    for (UIView *item in items) {
        NGPlaybackRailItemState *state = objc_getAssociatedObject(item, &NGPlaybackRailItemAssociation);
        if (!state) { state = [NGPlaybackRailItemState new]; state.sourceFrame = item.frame; state.sourceHidden = item.hidden; objc_setAssociatedObject(item, &NGPlaybackRailItemAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        else {
            if (!CGRectEqualToRect(item.frame, state.appliedFrame)) state.sourceFrame = item.frame;
            if (item.hidden != state.appliedHidden) state.sourceHidden = item.hidden;
        }
        baselineBottom = MAX(baselineBottom, CGRectGetMaxY(state.sourceFrame));
        BOOL hide = NGPlaybackElementHidden(NGPlaybackRailItemIdentifier(item));
        item.hidden = state.sourceHidden || hide; state.appliedHidden = item.hidden;
        if (item.hidden) continue;
        [visible addObject:item]; totalHeight += CGRectGetHeight(state.sourceFrame);
    }
    if (visible.count > 1) totalHeight += spacing * (visible.count - 1);
    [visible sortUsingComparator:^NSComparisonResult(UIView *a, UIView *b) {
        NGPlaybackRailItemState *left = objc_getAssociatedObject(a, &NGPlaybackRailItemAssociation);
        NGPlaybackRailItemState *right = objc_getAssociatedObject(b, &NGPlaybackRailItemAssociation);
        CGFloat y1 = CGRectGetMinY(left.sourceFrame), y2 = CGRectGetMinY(right.sourceFrame);
        return y1 < y2 ? NSOrderedAscending : (y1 > y2 ? NSOrderedDescending : NSOrderedSame);
    }];
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    CGFloat y = MAX(0, baselineBottom - totalHeight);
    for (UIView *item in visible) {
        NGPlaybackRailItemState *state = objc_getAssociatedObject(item, &NGPlaybackRailItemAssociation);
        CGRect frame = state.sourceFrame; frame.origin.y = y; item.frame = frame; state.appliedFrame = frame;
        y += CGRectGetHeight(frame) + spacing;
    }
    NGPlaybackRailTransformState *transform = objc_getAssociatedObject(rail, &NGPlaybackRailTransformAssociation);
    if (!transform) { transform = [NGPlaybackRailTransformState new]; transform.source = rail.transform; objc_setAssociatedObject(rail, &NGPlaybackRailTransformAssociation, transform, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
    else if (!CGAffineTransformEqualToTransform(rail.transform, transform.applied)) transform.source = rail.transform;
    CGFloat dx = [defaults floatForKey:NGPreferencePlaybackRailX], dy = [defaults floatForKey:NGPreferencePlaybackRailY];
    CGFloat scale = [defaults objectForKey:NGPreferencePlaybackRailScale] ? [defaults floatForKey:NGPreferencePlaybackRailScale] : 1.0;
    CGAffineTransform scaled = CGAffineTransformScale(transform.source, MAX(0.5, MIN(1.6, scale)), MAX(0.5, MIN(1.6, scale)));
    CGAffineTransform configured = scaled; configured.tx += dx; configured.ty += dy;
    rail.transform = configured; transform.applied = configured;
}

static char NGPlaybackGlassAssociationKey;
static NSHashTable<UIView *> *NGPlaybackInteractionViews;

static BOOL NGPlaybackGlassEnabled(void) {
    return [NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackGlass];
}

static UIColor *NGPlaybackLayerColor(CALayer *layer) {
    return layer.backgroundColor ? [UIColor colorWithCGColor:layer.backgroundColor] : nil;
}

static BOOL NGPlaybackColorsEqual(UIColor *left, UIColor *right) {
    if (left == right) return YES;
    if (!left || !right) return NO;
    return CGColorEqualToColor(left.CGColor, right.CGColor);
}

static void NGApplyPlaybackRailAppearance(UIView *item, NGPlaybackRailItemState *state) {
    BOOL clearBackground = NGPlaybackGlassEnabled();
    if (!state.appearanceManaged) {
        state.sourceBackgroundColor = item.backgroundColor ?: NSNull.null;
        UIColor *layerColor = NGPlaybackLayerColor(item.layer);
        state.sourceLayerBackgroundColor = layerColor ?: NSNull.null;
        state.sourceCornerRadius = item.layer.cornerRadius;
        state.appearanceManaged = YES;
    } else {
        id currentBackground = item.backgroundColor ?: NSNull.null;
        if (!state.appliedBackgroundColor || ![currentBackground isEqual:state.appliedBackgroundColor])
            state.sourceBackgroundColor = currentBackground;
        UIColor *currentLayerColor = NGPlaybackLayerColor(item.layer);
        id currentLayer = currentLayerColor ?: NSNull.null;
        if (!state.appliedLayerBackgroundColor || !NGPlaybackColorsEqual(currentLayerColor, state.appliedLayerBackgroundColor))
            state.sourceLayerBackgroundColor = currentLayer;
        if (!state.appliedCornerRadius || fabs(item.layer.cornerRadius - state.appliedCornerRadius) > 0.01)
            state.sourceCornerRadius = item.layer.cornerRadius;
    }
    if (clearBackground) {
        UIColor *clear = UIColor.clearColor;
        item.backgroundColor = clear;
        item.layer.backgroundColor = clear.CGColor;
        state.appliedBackgroundColor = clear;
        state.appliedLayerBackgroundColor = clear;
    } else {
        id currentBackground = item.backgroundColor ?: NSNull.null;
        BOOL restoreBackground = state.appliedBackgroundColor && [currentBackground isEqual:state.appliedBackgroundColor];
        BOOL restoreLayer = state.appliedLayerBackgroundColor && NGPlaybackColorsEqual(NGPlaybackLayerColor(item.layer), state.appliedLayerBackgroundColor);
        if (restoreBackground)
            item.backgroundColor = state.sourceBackgroundColor == NSNull.null ? nil : state.sourceBackgroundColor;
        if (restoreLayer)
            item.layer.backgroundColor = state.sourceLayerBackgroundColor == NSNull.null ? nil : ((UIColor *)state.sourceLayerBackgroundColor).CGColor;
        state.appliedBackgroundColor = nil;
        state.appliedLayerBackgroundColor = nil;
    }
    state.appearanceManaged = clearBackground;
}

static void NGRefreshPlaybackActionAppearance(UIView *item) {
    NGPlaybackRailItemState *state = objc_getAssociatedObject(item, &NGPlaybackRailItemAssociation);
    if (state) NGApplyPlaybackRailAppearance(item, state);
}

static void NGDecoratePlaybackInteraction(UIView *view) {
    if (!view || CGRectIsEmpty(view.bounds)) return;
    NGApplyPlaybackRailLayout(view);
    if (!NGPlaybackInteractionViews) NGPlaybackInteractionViews = [NSHashTable weakObjectsHashTable];
    [NGPlaybackInteractionViews addObject:view];
    NSArray *actionNames = @[@"SSShortVideoRightInteractionCollectView", @"SSShortVideoRightInteractionCommentView",
                             @"SSShortVideoRightInteractionLikeView", @"SSShortVideoRightInteractionShareView"];
    for (UIView *action in [view.subviews copy]) {
        BOOL avatar = [NGPlaybackRailItemIdentifier(action) isEqualToString:@"avatar"];
        if (!avatar && ![actionNames containsObject:NSStringFromClass(action.class)]) continue;
        if (!avatar) NGApplyPlaybackPreset(action, [actionNames indexOfObject:NSStringFromClass(action.class)]);
        NGRefreshPlaybackActionAppearance(action);
        NGPlaybackGlassView *glass = objc_getAssociatedObject(action, &NGPlaybackGlassAssociationKey);
        if (!NGPlaybackGlassEnabled()) {
            [glass removeFromSuperview];
            objc_setAssociatedObject(action, &NGPlaybackGlassAssociationKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            continue;
        }
        if (!glass) {
            glass = [[NGPlaybackGlassView alloc] initWithFrame:CGRectZero];
            objc_setAssociatedObject(action, &NGPlaybackGlassAssociationKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        // Keep all original controls and gestures in their existing hierarchy.
        CGRect frame = [action convertRect:action.bounds toView:view];
        {
            frame = CGRectMake(action.frame.origin.x, action.frame.origin.y,
                               CGRectGetWidth(action.bounds), CGRectGetWidth(action.bounds));
            for (UIView *icon in action.subviews) {
                if ([icon isKindOfClass:UIImageView.class] && !CGRectIsEmpty(icon.bounds)) {
                    frame = [icon convertRect:icon.bounds toView:view];
                    break;
                }
            }
        }
        if (avatar) frame = CGRectInset(frame, -3, -3);
        if (!CGRectEqualToRect(glass.frame, frame)) glass.frame = frame;
        glass.preferredCornerRadius = 0;
        glass.hidden = action.hidden || CGRectIsEmpty(action.bounds);
        glass.alpha = action.alpha;
        if (glass.superview != view || [view.subviews indexOfObject:glass] > [view.subviews indexOfObject:action])
            [view insertSubview:glass belowSubview:action];
        [glass setNeedsLayout];
    }
}

// Keep the channel and panel implementations in focused includes. The panel include
// owns comment input/emoji/editor glass and share-sheet glass as one hierarchy pass.
static id NGObjectProperty(id object, NSString *name) {
    SEL selector = NSSelectorFromString(name);
    Method method = object ? class_getInstanceMethod([object class], selector) : NULL;
    if (!method || method_getNumberOfArguments(method) != 2) return nil;
    char type[16]; method_getReturnType(method, type, sizeof(type));
    if (type[0] != '@') return nil;
    return ((id (*)(id, SEL))[object methodForSelector:selector])(object, selector);
}
#include "NGPanelGlass.inc"

static void NGScanPlaybackInteractions(void) {
    Class target = NSClassFromString(@"SSShortVideoRightInteractionView");
    if (!target) return;
    for (UIWindow *window in NGCaptureWindows()) {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
        for (NSUInteger index = 0; index < queue.count && index < 12000; index++) {
            UIView *view = queue[index];
            if ([view isKindOfClass:target]) NGDecoratePlaybackInteraction(view);
            [queue addObjectsFromArray:view.subviews];
        }
    }
}

static void NGUpdatePlaybackGlass(void) {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ NGUpdatePlaybackGlass(); });
        return;
    }
    NSArray<UIView *> *known = NGPlaybackInteractionViews.allObjects;
    NGRefreshPlaybackPresetIcons();
    for (UIView *view in known) NGDecoratePlaybackInteraction(view);
    NGScanPlaybackInteractions();
}

static char NGPlaybackHeaderGlassAssociation;
static char NGPlaybackHeaderHiddenAssociation;
static NSHashTable<UIView *> *NGPlaybackHeaderViews;

@interface NGPlaybackHeaderHiddenState : NSObject
@property(nonatomic) BOOL sourceHidden;
@property(nonatomic) BOOL appliedHidden;
@end
@implementation NGPlaybackHeaderHiddenState @end
static void NGSetPlaybackHeaderHidden(UIView *view, BOOL hidden) {
    if (!view) return;
    NGPlaybackHeaderHiddenState *state = objc_getAssociatedObject(view, &NGPlaybackHeaderHiddenAssociation);
    if (!state) { state = [NGPlaybackHeaderHiddenState new]; state.sourceHidden = view.hidden; objc_setAssociatedObject(view, &NGPlaybackHeaderHiddenAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
    else if (view.hidden != state.appliedHidden) state.sourceHidden = view.hidden;
    view.hidden = state.sourceHidden || hidden; state.appliedHidden = view.hidden;
}

#include "NGDanmakuEntrance.inc"

@interface NGPlaybackHeaderGlassState : NSObject
@property(nonatomic, strong) UIVisualEffectView *glass;
@property(nonatomic, copy) NSString *kind;
@end
@implementation NGPlaybackHeaderGlassState
@end

static BOOL NGPlaybackHeaderGlassEnabled(void) {
    return [NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackHeaderGlass];
}

static void NGApplyPlaybackHeaderGlassToTarget(UIView *target, NSString *kind, CGFloat inset, CGFloat radius) {
    if (!target || !kind.length) return;
    NGPlaybackHeaderGlassState *state = objc_getAssociatedObject(target, &NGPlaybackHeaderGlassAssociation);
    if (!NGPlaybackHeaderGlassEnabled() || target.hidden || target.alpha < 0.01 || CGRectIsEmpty(target.bounds) || !target.superview) {
        [state.glass removeFromSuperview];
        objc_setAssociatedObject(target, &NGPlaybackHeaderGlassAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    UIView *anchor = target;
    if ([kind isEqualToString:@"Channels"]) {
        while (anchor.superview && anchor.superview != target.window && ![NGClassName(anchor.superview) isEqualToString:@"SSVideoSeriesFeedHeaderView"]) anchor = anchor.superview;
    }
    UIView *parent = anchor.superview;
    UIUserInterfaceStyle scopedStyle = UIUserInterfaceStyleUnspecified;
    if (!state) {
        state = [NGPlaybackHeaderGlassState new];
        state.glass = [[UIVisualEffectView alloc] initWithEffect:nil];
        state.glass.frame = CGRectInset([target convertRect:target.bounds toView:parent], -inset, -inset);
        if (@available(iOS 26.0, *)) {
            UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
            effect.interactive = NO;
            state.glass.effect = effect;
        }
        state.glass.accessibilityIdentifier = [@"NaiwaGuo.Playback.HeaderGlass." stringByAppendingString:kind];
        state.glass.accessibilityElementsHidden = YES;
        state.glass.userInteractionEnabled = NO;
        state.glass.clipsToBounds = YES;
        state.glass.layer.cornerCurve = kCACornerCurveContinuous;
        objc_setAssociatedObject(target, &NGPlaybackHeaderGlassAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    state.glass.overrideUserInterfaceStyle = scopedStyle;
    state.kind = kind;
    CGRect frame = [target convertRect:target.bounds toView:parent];
    if ([kind isEqualToString:@"Channels"]) {
        CGFloat height = radius > 0 ? radius * 2 : 40;
        frame = CGRectMake(CGRectGetMinX(frame) - inset, CGRectGetMidY(frame) - height / 2,
                           CGRectGetWidth(frame) + inset * 2, height);
    } else if (inset != 0) frame = CGRectInset(frame, -inset, -inset);
    state.glass.layer.cornerRadius = CGRectGetHeight(frame) / 2.0;
    if (@available(iOS 26.0, *)) state.glass.cornerConfiguration = [UICornerConfiguration capsuleConfiguration];
    state.glass.hidden = NO;
    if (!CGRectEqualToRect(state.glass.frame, frame)) state.glass.frame = frame;
    if (state.glass.superview != parent) [parent insertSubview:state.glass belowSubview:anchor];
    else if ([parent.subviews indexOfObject:state.glass] > [parent.subviews indexOfObject:anchor]) [parent insertSubview:state.glass belowSubview:anchor];
}

static void NGDecoratePlaybackHeader(UIView *header) {
    if (!header || ![NGClassName(header) isEqualToString:@"SSVideoSeriesFeedHeaderView"]) return;
    if (!NGPlaybackHeaderViews) NGPlaybackHeaderViews = [NSHashTable weakObjectsHashTable];
    [NGPlaybackHeaderViews addObject:header];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:header];
    CGFloat capsuleHeight = 40;
    for (NSUInteger index = 0; index < queue.count && index < 1200; index++) {
        UIView *view = queue[index];
        if ([NGClassName(view) isEqualToString:@"FQReaderSaaSBaseImageView"] &&
            view.bounds.size.width >= 20 && view.bounds.size.width <= 30 &&
            view.bounds.size.height >= 20 && view.bounds.size.height <= 30)
            capsuleHeight = CGRectGetHeight(view.bounds) + 16;
        [queue addObjectsFromArray:view.subviews];
    }
    for (NSUInteger index = 0; index < queue.count && index < 1200; index++) {
        UIView *view = queue[index];
        NSString *name = NGClassName(view);
        if ([name isEqualToString:@"FQRJXCategoryCollectionView"] && view.alpha > 0.01) {
            NGApplyPlaybackHeaderGlassToTarget(view, @"Channels", 4, capsuleHeight / 2);
        } else if ([name isEqualToString:@"FQReaderSaaSBaseImageView"] && view.bounds.size.width >= 20 && view.bounds.size.width <= 30 && view.bounds.size.height >= 20 && view.bounds.size.height <= 30 && view.alpha > 0.01) {
            CGRect frame = [view convertRect:view.bounds toView:header];
            if (CGRectGetMidX(frame) < CGRectGetWidth(header.bounds) * 0.25) {
                NGSetPlaybackHeaderHidden(view, NGPlaybackElementHidden(@"header.sidebar"));
                NGApplyPlaybackHeaderGlassToTarget(view, @"Sidebar", 8, 0);
            } else if (CGRectGetMidX(frame) > CGRectGetWidth(header.bounds) * 0.75) {
                NGSetPlaybackHeaderHidden(view, NGPlaybackElementHidden(@"header.search"));
                NGApplyPlaybackHeaderGlassToTarget(view, @"Search", 8, 0);
            }
        }
    }
}

#include "NGEpisodeGlass.inc"
#include "NGCollectionGlass.inc"

static void NGScanPlaybackHeaders(void) {
    Class target = NSClassFromString(@"SSVideoSeriesFeedHeaderView");
    if (!target) return;
    for (UIWindow *window in NGCaptureWindows()) {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
        for (NSUInteger index = 0; index < queue.count && index < 12000; index++) {
            UIView *view = queue[index];
            if ([view isKindOfClass:target]) NGDecoratePlaybackHeader(view);
            UIViewController *owner = [view.nextResponder isKindOfClass:UIViewController.class] ? (UIViewController *)view.nextResponder : nil;
            if (owner && owner.viewIfLoaded == view) NGDecorateEpisodeController(owner);
            [queue addObjectsFromArray:view.subviews];
        }
    }
}

static void NGUpdatePlaybackHeaderGlass(void) {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ NGUpdatePlaybackHeaderGlass(); });
        return;
    }
    for (UIView *header in NGPlaybackHeaderViews.allObjects) NGDecoratePlaybackHeader(header);
    NGScanPlaybackHeaders();
}

static char NGPlaybackTextTransformAssociation;
static void NGApplyPlaybackTextTransform(UIView *view, CGFloat dx, CGFloat dy, CGFloat scale) {
    NGPlaybackRailTransformState *state = objc_getAssociatedObject(view, &NGPlaybackTextTransformAssociation);
    if (!state) { state = [NGPlaybackRailTransformState new]; state.source = view.transform; objc_setAssociatedObject(view, &NGPlaybackTextTransformAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
    else if (!CGAffineTransformEqualToTransform(view.transform, state.applied)) state.source = view.transform;
    CGAffineTransform transform = CGAffineTransformScale(state.source, scale, scale); transform.tx += dx; transform.ty += dy;
    view.transform = transform; state.applied = transform;
}
static NSHashTable<UIView *> *NGPlaybackTextViews;
static UIView *NGPlaybackDramaTitleRow(UIView *container) {
    for (UIView *row in container.subviews) {
        if (![NGClassName(row) isEqualToString:@"UIView"]) continue;
        for (UIView *child in row.subviews) {
            if (![child isKindOfClass:UILabel.class]) continue;
            NSString *text = ((UILabel *)child).text;
            if (text.length && ![text hasPrefix:@"@"] && !CGRectIsEmpty(child.bounds)) return row;
        }
    }
    return nil;
}
static void NGApplyPlaybackTextPreferences(UIView *view, NSString *kind) {
    NSString *prefix = [@"com.naiwaguo.playback.layout." stringByAppendingString:kind];
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    CGFloat scale = [defaults objectForKey:[prefix stringByAppendingString:@".scale"]] ? [defaults floatForKey:[prefix stringByAppendingString:@".scale"]] : 1;
    NGApplyPlaybackTextTransform(view, [defaults floatForKey:[prefix stringByAppendingString:@".x"]],
                                 [defaults floatForKey:[prefix stringByAppendingString:@".y"]], MAX(0.5, MIN(1.6, scale)));
}
static NSArray<NSString *> *NGPlaybackTextClassNames(void) {
    static NSArray<NSString *> *names;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ names = @[@"SSShortVideoOutterSeriesInfoView", @"SSShortVideoDerivateInfoView", @"SSShortVideoDerivateLeftContainerView", @"SSShortVideoOutterSeriesLeftContainerView", @"SSShortVideoAiUsageView", @"SSShortVideoBottomDistributionTextView", @"SSShortVideoBottomDistributionView", @"FQVShortVideoProgressView", @"FQVTagListView", @"SSShortVideoDanmakuLeftAboveView", @"FQCShortVideoDanmakuEntranceView"]; });
    return names;
}
static void NGDecoratePlaybackTextView(UIView *view) {
        NSString *name = NGClassName(view);
        if (![NGPlaybackTextClassNames() containsObject:name]) return;
        if (!NGPlaybackTextViews) NGPlaybackTextViews = [NSHashTable weakObjectsHashTable];
        [NGPlaybackTextViews addObject:view];
        if ([name containsString:@"BottomDistributionTextView"] || [name isEqualToString:@"SSShortVideoBottomDistributionView"]) NGDecorateCollectionRow(view);
        if ([name isEqualToString:@"FQVShortVideoProgressView"]) NGSetPlaybackHeaderHidden(view, NGPlaybackElementHidden(@"progress"));
        if ([name isEqualToString:@"FQVTagListView"]) NGSetPlaybackHeaderHidden(view, NGPlaybackElementHidden(@"tags"));
        if (NGIsDanmakuEntranceName(name)) NGDecorateDanmakuEntrance(view);
        if ([name containsString:@"OutterSeriesInfoView"] || [name containsString:@"DerivateInfoView"]) {
            NSString *prefix = @"com.naiwaguo.playback.layout.author";
            CGFloat dx = [NSUserDefaults.standardUserDefaults floatForKey:[prefix stringByAppendingString:@".x"]], dy = [NSUserDefaults.standardUserDefaults floatForKey:[prefix stringByAppendingString:@".y"]];
            CGFloat scale = [NSUserDefaults.standardUserDefaults objectForKey:[prefix stringByAppendingString:@".scale"]] ? [NSUserDefaults.standardUserDefaults floatForKey:[prefix stringByAppendingString:@".scale"]] : 1;
            NSMutableArray *desc = [NSMutableArray arrayWithArray:view.subviews];
            for (NSUInteger j = 0; j < desc.count && j < 600; j++) { UIView *label = desc[j]; if ([label isKindOfClass:UILabel.class] && [((UILabel *)label).text hasPrefix:@"@"] ) NGSetPlaybackHeaderHidden(label, NGPlaybackElementHidden(@"text.author")); [desc addObjectsFromArray:label.subviews]; }
            UIView *authorRow = nil;
            NSMutableArray *authorCandidates = [NSMutableArray arrayWithArray:view.subviews];
            for (NSUInteger j = 0; j < authorCandidates.count && j < 200; j++) { UIView *candidate = authorCandidates[j]; BOOL hasAuthor = NO; for (UIView *child in candidate.subviews) if ([child isKindOfClass:UILabel.class] && [((UILabel *)child).text hasPrefix:@"@"]) { hasAuthor = YES; break; } if (hasAuthor) { authorRow = candidate; break; } [authorCandidates addObjectsFromArray:candidate.subviews]; }
            if (authorRow && authorRow != view && ![NGClassName(authorRow) containsString:@"LeftContainerView"]) NGApplyPlaybackTextTransform(authorRow, dx, dy, scale);
            else if (authorRow) for (UIView *child in authorRow.subviews) if ([child isKindOfClass:UILabel.class] && [((UILabel *)child).text hasPrefix:@"@"]) NGApplyPlaybackTextTransform(child, dx, dy, scale);
        }
        BOOL description = [name containsString:@"DerivateLeftContainerView"] || [name containsString:@"OutterSeriesLeftContainerView"];
        BOOL notice = [name containsString:@"AiUsageView"];
        UIView *titleRow = description ? NGPlaybackDramaTitleRow(view) : nil;
        if (titleRow) {
            NGApplyPlaybackTextPreferences(titleRow, @"title");
            for (UIView *child in titleRow.subviews) {
                if (objc_getAssociatedObject(child, &NGPlaybackTextTransformAssociation)) NGApplyPlaybackTextTransform(child, 0, 0, 1);
            }
            for (UIView *child in view.subviews)
                if ([NGClassName(child) isEqualToString:@"FQVShortVideoTagView"]) NGApplyPlaybackTextPreferences(child, @"title");
        }
        if (description || notice) {
            NSString *kind = notice ? @"notice" : @"description";
            NSString *prefix = [@"com.naiwaguo.playback.layout." stringByAppendingString:kind];
            CGFloat dx = [NSUserDefaults.standardUserDefaults floatForKey:[prefix stringByAppendingString:@".x"]], dy = [NSUserDefaults.standardUserDefaults floatForKey:[prefix stringByAppendingString:@".y"]];
            CGFloat scale = [NSUserDefaults.standardUserDefaults objectForKey:[prefix stringByAppendingString:@".scale"]] ? [NSUserDefaults.standardUserDefaults floatForKey:[prefix stringByAppendingString:@".scale"]] : 1;
            BOOL hidden = NGPlaybackElementHidden([@"text." stringByAppendingString:kind]);
            if (notice) { NGSetPlaybackHeaderHidden(view, hidden); NGApplyPlaybackTextTransform(view, dx, dy, scale); }
            else {
                NSMutableArray *desc = [NSMutableArray arrayWithArray:view.subviews];
                for (NSUInteger j = 0; j < desc.count && j < 300; j++) {
                    UIView *label = desc[j];
                    if (label == titleRow || [NGClassName(label) isEqualToString:@"FQVTagListView"] || [NGClassName(label) isEqualToString:@"FQVShortVideoTagView"]) continue;
                    BOOL isYYLabel = [NGClassName(label) containsString:@"YYLabel"];
                    NSString *text = [label isKindOfClass:UILabel.class] ? ((UILabel *)label).text : [label accessibilityLabel];
                    if (isYYLabel || ([label isKindOfClass:UILabel.class] && ![text hasPrefix:@"@"] && text.length > 8)) { NGSetPlaybackHeaderHidden(label, hidden); NGApplyPlaybackTextTransform(label, dx, dy, scale); }
                    [desc addObjectsFromArray:label.subviews];
                }
            }
        }
}
static void NGUpdatePlaybackTextLayout(void) {
    NSMutableArray<UIView *> *queue = [NSMutableArray array]; for (UIWindow *window in NGCaptureWindows()) [queue addObject:window];
    for (NSUInteger i = 0; i < queue.count && i < 12000; i++) {
        UIView *view = queue[i]; NGDecoratePlaybackTextView(view); [queue addObjectsFromArray:view.subviews];
    }
}
static void NGInstallPlaybackTextHooks(void) {
    NGInstallCollectionHooks();
    static NSMutableSet *installed; if (!installed) installed = [NSMutableSet set];
    for (NSString *name in NGPlaybackTextClassNames()) {
        Class target = NSClassFromString(name); if (!target || [installed containsObject:name]) continue;
        SEL selector = @selector(layoutSubviews); Method method = class_getInstanceMethod(target, selector); if (!method) continue;
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIView *view) { ((void (*)(id, SEL))original)(view, selector); NGDecoratePlaybackTextView(view); });
        if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
        if (NGIsDanmakuEntranceName(name)) NGInstallDanmakuHiddenHook(target);
        [installed addObject:name];
    }
}

static void NGInstallPlaybackHeaderGlassHook(void) {
    NGInstallEpisodeGlassHooks();
    static BOOL installed;
    if (installed) return;
    Class target = NSClassFromString(@"SSVideoSeriesFeedHeaderView");
    if (!target) return;
    SEL selector = @selector(layoutSubviews);
    Method method = class_getInstanceMethod(target, selector);
    if (!method) return;
    IMP original = method_getImplementation(method);
    IMP replacement = imp_implementationWithBlock(^(UIView *header) {
        ((void (*)(id, SEL))original)(header, selector);
        NGDecoratePlaybackHeader(header);
    });
    if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
    installed = YES;
    NGScanPlaybackHeaders();
}

static void NGSetPlaybackGlassEnabled(BOOL enabled) {
    [NSUserDefaults.standardUserDefaults setBool:enabled forKey:NGPreferencePlaybackGlass];
    NGUpdatePlaybackGlass();
    NGLog(@"playback", @"right-glass-changed", @{@"enabled": @(enabled)});
}

static void NGInstallPlaybackGlassHook(void) {
    NGInstallPlaybackPresetHooks();
    static BOOL installed;
    if (installed) return;
    Class target = NSClassFromString(@"SSShortVideoRightInteractionView");
    if (!target) return;
    SEL selector = @selector(layoutSubviews);
    Method method = class_getInstanceMethod(target, selector);
    if (!method) return;
    IMP original = method_getImplementation(method);
    const char *types = method_getTypeEncoding(method);
    IMP replacement = imp_implementationWithBlock(^(UIView *view) {
        ((void (*)(id, SEL))original)(view, selector);
        NGDecoratePlaybackInteraction(view);
    });
    if (!class_addMethod(target, selector, replacement, types)) class_replaceMethod(target, selector, replacement, types);
    installed = YES;
    NGScanPlaybackInteractions();
    NGLog(@"playback", @"right-glass-hook-installed", @{@"class": NSStringFromClass(target)});
}

#include "NGFullscreen.inc"
#include "NGFullscreenPlayback.inc"

static NSArray<NSDictionary *> *NGLiveLayoutControlGroups(void) {
    return @[
        @{@"title": @"右侧整体", @"prefix": @"com.naiwaguo.playback.rail"},
        @{@"title": @"作者名称", @"prefix": @"com.naiwaguo.playback.layout.author"},
        @{@"title": @"剧名", @"prefix": @"com.naiwaguo.playback.layout.title"},
        @{@"title": @"视频简介", @"prefix": @"com.naiwaguo.playback.layout.description"},
        @{@"title": @"作者声明", @"prefix": @"com.naiwaguo.playback.layout.notice"}
    ];
}
static void NGDismissLivePreview(void);

@interface NGLivePreviewController : UIViewController
@property(nonatomic, strong) UIVisualEffectView *panel;
@property(nonatomic, strong) NSArray<NSDictionary *> *groups;
@property(nonatomic, strong) UIView *dragHandle;
@property(nonatomic) CGPoint dragStartOrigin;
@property(nonatomic) BOOL isDragging;
@end

@implementation NGLivePreviewController
- (instancetype)init {
    if ((self = [super init])) self.groups = NGLiveLayoutControlGroups();
    return self;
}
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.clearColor;
    NGUpdatePlaybackGlass();
    NGUpdatePlaybackTextLayout();
    self.panel = [[UIVisualEffectView alloc] initWithEffect:nil];
    self.panel.tag = 9001;
    self.panel.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified;
    self.panel.clipsToBounds = YES;
    self.panel.layer.cornerRadius = 20;
    self.panel.layer.cornerCurve = kCACornerCurveContinuous;
    if (@available(iOS 26.0, *)) {
        UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
        effect.interactive = YES;
        self.panel.effect = effect;
    } else {
        self.panel.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    }
    self.panel.contentView.backgroundColor = [UIColor.secondarySystemBackgroundColor colorWithAlphaComponent:0.25];
    [self.view addSubview:self.panel];

    UIView *header = [UIView new];
    self.dragHandle = header;
    UILabel *title = [UILabel new];
    title.text = @"实时预览调节";
    title.textColor = UIColor.labelColor;
    title.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
    UIButton *close = [UIButton buttonWithType:UIButtonTypeSystem];
    [close setImage:[UIImage systemImageNamed:@"xmark.circle.fill"] forState:UIControlStateNormal];
    close.tintColor = UIColor.labelColor;
    close.accessibilityLabel = @"关闭实时预览";
    [close addTarget:self action:@selector(closePreview) forControlEvents:UIControlEventTouchUpInside];
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panelDragged:)];
    pan.cancelsTouchesInView = NO;
    [header addGestureRecognizer:pan];
    [self.panel.contentView addSubview:header];
    [header addSubview:title];
    [header addSubview:close];
    UIScrollView *scroll = [UIScrollView new];
    UIStackView *stack = [UIStackView new];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 10;
    UIButton *reset = [UIButton buttonWithType:UIButtonTypeSystem];
    [reset setTitle:@"重置默认位置与大小" forState:UIControlStateNormal];
    reset.accessibilityIdentifier = @"NaiwaGuo.LivePreview.ResetLayout";
    [reset addTarget:self action:@selector(resetLayout) forControlEvents:UIControlEventTouchUpInside];
    [stack addArrangedSubview:reset];
    for (NSDictionary *group in self.groups) {
        UILabel *heading = [UILabel new];
        heading.text = group[@"title"];
        heading.textColor = UIColor.secondaryLabelColor;
        heading.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
        [stack addArrangedSubview:heading];
        for (NSString *axis in @[@"x", @"y", @"scale"]) {
            BOOL scale = [axis isEqualToString:@"scale"];
            NSString *key = [group[@"prefix"] stringByAppendingFormat:@".%@", axis];
            UIView *row = [UIView new];
            UILabel *name = [UILabel new];
            name.text = scale ? @"大小" : ([axis isEqualToString:@"x"] ? @"水平位置" : @"垂直位置");
            name.textColor = UIColor.labelColor;
            name.font = [UIFont systemFontOfSize:13 weight:UIFontWeightRegular];
            UILabel *value = [UILabel new];
            value.tag = 2;
            value.textAlignment = NSTextAlignmentRight;
            value.textColor = UIColor.secondaryLabelColor;
            value.font = [UIFont monospacedDigitSystemFontOfSize:11 weight:UIFontWeightRegular];
            UISlider *slider = [UISlider new];
            slider.tag = 1;
            slider.accessibilityIdentifier = key;
            slider.minimumValue = scale ? 0.65 : -100;
            slider.maximumValue = scale ? 1.4 : 100;
            NSNumber *saved = [NSUserDefaults.standardUserDefaults objectForKey:key];
            slider.value = saved ? saved.floatValue : (scale ? 1.0 : 0.0);
            [slider addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            [row addSubview:name]; [row addSubview:value]; [row addSubview:slider];
            name.translatesAutoresizingMaskIntoConstraints = NO;
            value.translatesAutoresizingMaskIntoConstraints = NO;
            slider.translatesAutoresizingMaskIntoConstraints = NO;
            [NSLayoutConstraint activateConstraints:@[
                [name.leadingAnchor constraintEqualToAnchor:row.leadingAnchor],
                [name.topAnchor constraintEqualToAnchor:row.topAnchor],
                [value.trailingAnchor constraintEqualToAnchor:row.trailingAnchor],
                [value.centerYAnchor constraintEqualToAnchor:name.centerYAnchor],
                [value.widthAnchor constraintEqualToConstant:62],
                [slider.leadingAnchor constraintEqualToAnchor:row.leadingAnchor],
                [slider.trailingAnchor constraintEqualToAnchor:row.trailingAnchor],
                [slider.topAnchor constraintEqualToAnchor:name.bottomAnchor constant:2],
                [slider.bottomAnchor constraintEqualToAnchor:row.bottomAnchor],
                [row.heightAnchor constraintEqualToConstant:52]
            ]];
            [self updateValue:value slider:slider];
            [stack addArrangedSubview:row];
        }
    }
    [self.panel.contentView addSubview:scroll];
    [scroll addSubview:stack];
    title.translatesAutoresizingMaskIntoConstraints = NO;
    header.translatesAutoresizingMaskIntoConstraints = NO;
    close.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
        [header.leadingAnchor constraintEqualToAnchor:self.panel.contentView.leadingAnchor constant:12],
        [header.trailingAnchor constraintEqualToAnchor:self.panel.contentView.trailingAnchor constant:-12],
        [header.topAnchor constraintEqualToAnchor:self.panel.contentView.topAnchor constant:4],
        [header.heightAnchor constraintEqualToConstant:48],
        [title.leadingAnchor constraintEqualToAnchor:header.leadingAnchor constant:6],
        [title.centerYAnchor constraintEqualToAnchor:header.centerYAnchor],
        [close.trailingAnchor constraintEqualToAnchor:header.trailingAnchor],
        [close.centerYAnchor constraintEqualToAnchor:title.centerYAnchor],
        [close.widthAnchor constraintEqualToConstant:38], [close.heightAnchor constraintEqualToConstant:38],
        [scroll.leadingAnchor constraintEqualToAnchor:self.panel.contentView.leadingAnchor constant:18],
        [scroll.trailingAnchor constraintEqualToAnchor:self.panel.contentView.trailingAnchor constant:-18],
        [scroll.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:4],
        [scroll.bottomAnchor constraintEqualToAnchor:self.panel.contentView.bottomAnchor constant:-12],
        [stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor],
        [stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor],
        [stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor],
        [stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor],
        [stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor]
    ]];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CGFloat safeBottom = self.view.safeAreaInsets.bottom;
    CGFloat height = MIN(360, CGRectGetHeight(self.view.bounds) * 0.46);
    CGFloat width = MIN(360, CGRectGetWidth(self.view.bounds) - 24);
    CGPoint origin = CGPointMake(12, CGRectGetHeight(self.view.bounds) - safeBottom - height - 10);
    if (!self.isDragging) {
        NSString *saved = [NSUserDefaults.standardUserDefaults stringForKey:@"com.naiwaguo.preview.panelOrigin"];
        if (saved.length) origin = CGPointFromString(saved);
    } else {
        origin = self.panel.frame.origin;
    }
    CGFloat minY = self.view.safeAreaInsets.top + 8;
    CGFloat maxY = MAX(minY, CGRectGetHeight(self.view.bounds) - safeBottom - height - 8);
    origin.x = MIN(MAX(12, origin.x), MAX(12, CGRectGetWidth(self.view.bounds) - width - 12));
    origin.y = MIN(MAX(minY, origin.y), maxY);
    self.panel.frame = CGRectMake(origin.x, origin.y, width, height);
}
- (void)panelDragged:(UIPanGestureRecognizer *)gesture {
    if (gesture.state == UIGestureRecognizerStateBegan) {
        self.isDragging = YES;
        self.dragStartOrigin = self.panel.frame.origin;
    }
    CGPoint translation = [gesture translationInView:self.view];
    CGPoint origin = CGPointMake(self.dragStartOrigin.x + translation.x, self.dragStartOrigin.y + translation.y);
    CGRect frame = self.panel.frame;
    CGFloat minY = self.view.safeAreaInsets.top + 8;
    CGFloat maxY = MAX(minY, CGRectGetHeight(self.view.bounds) - self.view.safeAreaInsets.bottom - CGRectGetHeight(frame) - 8);
    origin.x = MIN(MAX(12, origin.x), MAX(12, CGRectGetWidth(self.view.bounds) - CGRectGetWidth(frame) - 12));
    origin.y = MIN(MAX(minY, origin.y), maxY);
    frame.origin = origin;
    self.panel.frame = frame;
    if (gesture.state == UIGestureRecognizerStateEnded || gesture.state == UIGestureRecognizerStateCancelled) {
        [NSUserDefaults.standardUserDefaults setObject:NSStringFromCGPoint(origin) forKey:@"com.naiwaguo.preview.panelOrigin"];
        self.isDragging = NO;
    }
}
- (void)updateValue:(UILabel *)label slider:(UISlider *)slider {
    label.text = [slider.accessibilityIdentifier hasSuffix:@"scale"]
        ? [NSString stringWithFormat:@"%.2f×", slider.value]
        : [NSString stringWithFormat:@"%+.0f pt", slider.value];
}
- (void)sliderChanged:(UISlider *)slider {
    [NSUserDefaults.standardUserDefaults setFloat:slider.value forKey:slider.accessibilityIdentifier];
    UIView *row = slider.superview;
    [self updateValue:[row viewWithTag:2] slider:slider];
    if ([slider.accessibilityIdentifier hasPrefix:@"com.naiwaguo.playback.rail."]) {
        for (UIView *rail in NGPlaybackInteractionViews.allObjects)
            if (rail.window && !rail.hidden) NGDecoratePlaybackInteraction(rail);
    } else {
        for (UIView *text in NGPlaybackTextViews.allObjects)
            if (text.window && !text.hidden) NGDecoratePlaybackTextView(text);
    }
}
- (void)resetLayout {
    for (NSDictionary *group in self.groups) for (NSString *axis in @[@"x", @"y", @"scale"])
        [NSUserDefaults.standardUserDefaults removeObjectForKey:[group[@"prefix"] stringByAppendingFormat:@".%@", axis]];
    NSMutableArray *queue = [NSMutableArray arrayWithObject:self.panel];
    for (NSUInteger i = 0; i < queue.count && i < 300; i++) {
        UIView *view = queue[i];
        if ([view isKindOfClass:UISlider.class]) {
            UISlider *slider = (id)view; slider.value = [slider.accessibilityIdentifier hasSuffix:@"scale"] ? 1 : 0;
            [self updateValue:[slider.superview viewWithTag:2] slider:slider];
        }
        [queue addObjectsFromArray:view.subviews];
    }
    NGUpdatePlaybackGlass(); NGUpdatePlaybackTextLayout();
}
- (void)closePreview { NGDismissLivePreview(); }
@end

static UIWindow *NGLivePreviewWindow;

@interface NGLivePreviewHostWindow : UIWindow
@end
@implementation NGLivePreviewHostWindow
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *panel = [self.rootViewController.view viewWithTag:9001];
    if (!panel || self.hidden) return nil;
    CGPoint local = [panel convertPoint:point fromView:self];
    return [panel pointInside:local withEvent:event] ? [panel hitTest:local withEvent:event] : nil;
}
@end

static void NGDismissLivePreview(void) {
    UIWindow *window = NGLivePreviewWindow;
    if (!window) return;
    NGLivePreviewWindow = nil;
    window.hidden = YES;
    [NGBaseWindow() makeKeyWindow];
}

static void NGPresentLivePreview(void) {
    UIWindow *base = NGBaseWindow();
    if (!base.windowScene) return;
    if (NGLivePreviewWindow) NGDismissLivePreview();
    NGLivePreviewHostWindow *window = [[NGLivePreviewHostWindow alloc] initWithWindowScene:base.windowScene];
    window.frame = base.bounds;
    window.backgroundColor = UIColor.clearColor;
    window.windowLevel = UIWindowLevelAlert - 1;
    window.rootViewController = [NGLivePreviewController new];
    window.hidden = NO;
    [window makeKeyAndVisible];
    NGLivePreviewWindow = window;
    [window.rootViewController.view setNeedsLayout];
}

@interface NGSettingsViewController : NGSettingsTableController
@end


static void NGOpenSettings(void) {
    NGDismissLivePreview();
    UIViewController *presenter = NGTopController();
    while (presenter.presentedViewController) presenter = presenter.presentedViewController;
    if (presenter.transitionCoordinator.isAnimated) {
        BOOL accepted = [presenter.transitionCoordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
            dispatch_async(dispatch_get_main_queue(), ^{ NGOpenSettings(); });
        }];
        if (accepted) return;
    }
    if ([NGClassName(presenter) hasPrefix:@"NGSettings"]) return;
    if (presenter.isBeingPresented || presenter.isBeingDismissed) return;
    if ([presenter isKindOfClass:NGSettingsViewController.class] || [presenter isKindOfClass:UINavigationController.class]) {
        if ([presenter isKindOfClass:UINavigationController.class] && [((UINavigationController *)presenter).topViewController isKindOfClass:NGSettingsViewController.class]) return;
    }
    if (!presenter) return;
    UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:[NGSettingsViewController new]];
    nav.modalPresentationStyle = UIModalPresentationFullScreen;
    nav.modalTransitionStyle = UIModalTransitionStyleCoverVertical;
    nav.view.accessibilityIdentifier = @"NaiwaGuo.Settings.Presentation";
    [presenter presentViewController:nav animated:YES completion:nil];
    NGLog(@"settings", @"opened", nil);
}

static UIImage *NGHostIcon(void) {
    NSDictionary *icons = NSBundle.mainBundle.infoDictionary[@"CFBundleIcons"];
    NSArray *files = icons[@"CFBundlePrimaryIcon"][@"CFBundleIconFiles"];
    for (NSString *name in files.reverseObjectEnumerator) {
        UIImage *image = [UIImage imageNamed:name];
        if (image) return image;
        for (NSString *suffix in @[@"@3x.png", @"@2x.png", @".png"]) {
            NSString *path = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:[name stringByAppendingString:suffix]];
            image = [UIImage imageWithContentsOfFile:path];
            if (image) return image;
        }
    }
    return [UIImage imageNamed:@"AppIcon60x60"] ?: [UIImage imageNamed:@"AppIcon"];
}

@interface NGNativeEntry : UIButton
- (void)open;
@end
@implementation NGNativeEntry
- (void)open { NGOpenSettings(); }
@end

static void NGInstallNativeSettings(void) {
    static BOOL installed;
    Class target = NSClassFromString(@"SSSettingViewController");
    if (!target || installed) return;
    SEL selector = @selector(viewDidLoad);
    Method method = class_getInstanceMethod(target, selector);
    IMP original = method_getImplementation(method);
    IMP replacement = imp_implementationWithBlock(^(UIViewController *owner) {
        ((void (*)(id, SEL))original)(owner, selector);
        NSMutableArray *queue = [NSMutableArray arrayWithObject:owner.view];
        UITableView *table = nil;
        for (NSUInteger i = 0; i < queue.count && i < 1000; i++) {
            UIView *view = queue[i];
            if ([view isKindOfClass:UITableView.class]) { table = (UITableView *)view; break; }
            [queue addObjectsFromArray:view.subviews];
        }
        if (!table || [table.tableHeaderView.accessibilityIdentifier isEqual:@"NaiwaGuo.NativeEntry"]) return;
        UIView *oldHeader = table.tableHeaderView;
        CGFloat oldHeight = oldHeader ? CGRectGetHeight(oldHeader.bounds) : 0;
        UIView *header = [[UIView alloc] initWithFrame:CGRectMake(0, 0, table.bounds.size.width, 62 + oldHeight)];
        header.accessibilityIdentifier = @"NaiwaGuo.NativeEntry";
        NGNativeEntry *entry = [NGNativeEntry buttonWithType:UIButtonTypeSystem];
        entry.frame = CGRectMake(0, 0, header.bounds.size.width, 54);
        entry.autoresizingMask = UIViewAutoresizingFlexibleWidth;
        UIButtonConfiguration *config = [UIButtonConfiguration plainButtonConfiguration];
        config.title = @"NaiwaGuo";
        config.image = [UIImage systemImageNamed:@"slider.horizontal.3"];
        config.imagePadding = 12;
        config.baseForegroundColor = UIColor.labelColor;
        config.background.backgroundColor = UIColor.secondarySystemGroupedBackgroundColor;
        config.background.cornerRadius = 8;
        config.contentInsets = NSDirectionalEdgeInsetsMake(12, 16, 12, 16);
        entry.configuration = config;
        entry.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeading;
        entry.accessibilityIdentifier = @"NaiwaGuo.NativeSettingsEntry";
        [entry addTarget:entry action:@selector(open) forControlEvents:UIControlEventTouchUpInside];
        [header addSubview:entry];
        if (oldHeader) { oldHeader.frame = CGRectMake(0, 62, header.bounds.size.width, oldHeight); [header addSubview:oldHeader]; }
        table.tableHeaderView = header;
        table.sectionHeaderTopPadding = 0;
    });
    if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
    installed = YES;
}

#ifdef NG_TEST
static UIWindow *NGCurrentOverlay(void) { return NGDDebugInspectorWindow(); }
#endif
static void NGInstallOverlay(void) { NGDDebugInspectorInstall(); }

static NSDictionary *NGSettingRow(NSString *title, NSString *subtitle, NSString *symbol, NSString *key, NSString *action) {
    NSMutableDictionary *row = [@{@"title": title ?: @"", @"subtitle": subtitle ?: @"", @"symbol": symbol ?: @"circle"} mutableCopy];
    if (key.length) row[@"key"] = key;
    if (action.length) row[@"action"] = action;
    return row;
}

#include "NGDock.inc"

static NSArray<NSArray<NSDictionary *> *> *NGPlaybackVisibilityItems(void) {
    return @[
        @[@{@"key":@"avatar",@"title":@"作者头像"},@{@"key":@"collect",@"title":@"收藏"},@{@"key":@"comment",@"title":@"评论"},@{@"key":@"like",@"title":@"点赞"},@{@"key":@"share",@"title":@"分享"}],
        @[@{@"key":@"header.sidebar",@"title":@"左上角侧边栏"},@{@"key":@"header.search",@"title":@"右上角搜索"}],
        @[@{@"key":@"text.author",@"title":@"作者名称"},@{@"key":@"text.description",@"title":@"视频简介"},@{@"key":@"text.notice",@"title":@"作者声明 / AI 提示"},@{@"key":@"text.collection",@"title":@"剧集 / 合集信息"},@{@"key":@"progress",@"title":@"播放进度条"},@{@"key":@"tags",@"title":@"视频标签"},@{@"key":@"danmakuEntrance",@"title":@"主页发弹幕按钮"}]
    ];
}

@interface NGPlaybackVisibilitySettingsController : NGSettingsTableController
@end
@implementation NGPlaybackVisibilitySettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title = @"播放页元素显示"; self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)]; }
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return NGPlaybackVisibilityItems().count; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return NGPlaybackVisibilityItems()[section].count; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return @[@"右侧控件", @"顶部辅助按钮", @"视频信息"][section]; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *item = NGPlaybackVisibilityItems()[path.section][path.row]; NSString *key = [@"com.naiwaguo.playback.hidden." stringByAppendingString:item[@"key"]];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil]; cell.textLabel.text = item[@"title"];
    UISwitch *toggle = [UISwitch new]; toggle.on = [NSUserDefaults.standardUserDefaults boolForKey:key]; toggle.accessibilityIdentifier = item[@"key"]; [toggle addTarget:self action:@selector(itemChanged:) forControlEvents:UIControlEventValueChanged]; cell.accessoryView = toggle; cell.selectionStyle = UITableViewCellSelectionStyleNone; return cell;
}
- (void)itemChanged:(UISwitch *)sender {
    NSString *key = [@"com.naiwaguo.playback.hidden." stringByAppendingString:sender.accessibilityIdentifier ?: @""];
    [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:key];
    NGPrepareRuntimeGlass();
    NGUpdatePlaybackGlass(); NGUpdatePlaybackHeaderGlass(); NGUpdatePlaybackTextLayout();
}
@end

@interface NGPlaybackLayoutSettingsController : NGSettingsTableController
@property(nonatomic, strong) NSArray<NSArray<NSDictionary *> *> *controls;
@end
@implementation NGPlaybackLayoutSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"位置与大小";
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
    NSMutableArray *sections = [NSMutableArray array];
    NSDictionary *labels = @{@"x":@"水平位置",@"y":@"垂直位置",@"scale":@"整体大小"};
    for (NSDictionary *group in NGLiveLayoutControlGroups()) {
        NSMutableArray *rows = [NSMutableArray array];
        for (NSString *axis in @[@"x",@"y",@"scale"]) {
            BOOL scale = [axis isEqualToString:@"scale"]; [rows addObject:@{@"key":[group[@"prefix"] stringByAppendingFormat:@".%@",axis],@"title":labels[axis],@"min":scale ? @0.65 : @(-100),@"max":scale ? @1.4 : @100,@"default":scale ? @1.0 : @0.0,@"unit":scale ? @"×" : @" pt"}];
        }
        [sections addObject:rows];
    }
    self.controls = sections;
    UIBarButtonItem *preview = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"slider.horizontal.3"] style:UIBarButtonItemStylePlain target:self action:@selector(startPreview)];
    preview.accessibilityLabel = @"实时预览调节";
    UIBarButtonItem *reset = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"arrow.counterclockwise"] style:UIBarButtonItemStylePlain target:self action:@selector(resetLayout)];
    reset.accessibilityLabel = @"恢复默认位置与大小";
    self.navigationItem.rightBarButtonItems = @[reset, preview];
}
- (void)startPreview {
    UINavigationController *navigation = self.navigationController;
    [navigation dismissViewControllerAnimated:YES completion:^{ NGPresentLivePreview(); }];
}
- (void)resetLayout {
    for (NSArray *section in self.controls) for (NSDictionary *item in section) [NSUserDefaults.standardUserDefaults removeObjectForKey:item[@"key"]];
    NGUpdatePlaybackGlass(); NGUpdatePlaybackTextLayout(); [self.tableView reloadData];
}
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return self.controls.count + 1; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section ? self.controls[section - 1].count : 2; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return section ? NGLiveLayoutControlGroups()[section - 1][@"title"] : nil; }
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { return 72; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    if (!path.section && path.row == 1) {
        UITableViewCell *reset = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
        reset.textLabel.text = @"重置默认位置与大小";
        reset.detailTextLabel.text = @"恢复全部五组控件的位置和大小";
        reset.imageView.image = [UIImage systemImageNamed:@"arrow.counterclockwise"];
        reset.accessibilityIdentifier = @"NaiwaGuo.Playback.ResetLayout";
        return reset;
    }
    if (!path.section) {
        UITableViewCell *action = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
        action.textLabel.text = @"实时预览调节";
        action.detailTextLabel.text = @"关闭设置页后直接在播放界面上拖动";
        action.imageView.image = [UIImage systemImageNamed:@"slider.horizontal.3"];
        action.imageView.tintColor = UIColor.systemBlueColor;
        action.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        action.accessibilityIdentifier = @"NaiwaGuo.Playback.LivePreview";
        return action;
    }
    NSDictionary *item = self.controls[path.section - 1][path.row]; UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    CGFloat minimum = [item[@"min"] doubleValue], maximum = [item[@"max"] doubleValue]; NSString *key = item[@"key"];
    UILabel *title = [UILabel new]; title.text = item[@"title"]; title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
    UILabel *value = [UILabel new]; value.textAlignment = NSTextAlignmentRight; value.font = [UIFont monospacedDigitSystemFontOfSize:12 weight:UIFontWeightRegular]; value.tag = 2;
    UISlider *slider = [UISlider new]; slider.minimumValue = minimum; slider.maximumValue = maximum; slider.accessibilityIdentifier = key; slider.tag = 1;
    NSNumber *saved = [NSUserDefaults.standardUserDefaults objectForKey:key]; slider.value = saved ? saved.floatValue : [item[@"default"] floatValue];
    [slider addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged]; [cell.contentView addSubview:title]; [cell.contentView addSubview:value]; [cell.contentView addSubview:slider];
    title.translatesAutoresizingMaskIntoConstraints = NO; value.translatesAutoresizingMaskIntoConstraints = NO; slider.translatesAutoresizingMaskIntoConstraints = NO;
    UILayoutGuide *margins = cell.contentView.layoutMarginsGuide;
    [NSLayoutConstraint activateConstraints:@[[title.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor], [title.topAnchor constraintEqualToAnchor:cell.contentView.topAnchor constant:8], [title.trailingAnchor constraintLessThanOrEqualToAnchor:value.leadingAnchor constant:-8], [value.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor], [value.centerYAnchor constraintEqualToAnchor:title.centerYAnchor], [value.widthAnchor constraintEqualToConstant:66], [slider.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor], [slider.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor], [slider.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:2], [slider.heightAnchor constraintEqualToConstant:30]]];
    [self updateValueLabel:value forSlider:slider unit:item[@"unit"]]; cell.selectionStyle = UITableViewCellSelectionStyleNone; return cell;
}
- (void)updateValueLabel:(UILabel *)label forSlider:(UISlider *)slider unit:(NSString *)unit { label.text = [unit isEqualToString:@"×"] ? [NSString stringWithFormat:@"%.2f×", slider.value] : [NSString stringWithFormat:@"%+.0f pt", slider.value]; }
- (void)sliderChanged:(UISlider *)slider {
    [NSUserDefaults.standardUserDefaults setFloat:slider.value forKey:slider.accessibilityIdentifier]; UITableViewCell *cell = (UITableViewCell *)slider.superview.superview; UILabel *value = [cell.contentView viewWithTag:2]; NSString *unit = [slider.accessibilityIdentifier hasSuffix:@"scale"] ? @"×" : @" pt"; [self updateValueLabel:value forSlider:slider unit:unit];
    NGUpdatePlaybackGlass(); NGUpdatePlaybackTextLayout();
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (!indexPath.section) { if (indexPath.row == 1) [self resetLayout]; else [self startPreview]; }
}
@end

@interface NGCommentGlassOpacityController : NGSettingsTableController
@end
@implementation NGCommentGlassOpacityController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title = @"评论区玻璃透明度"; self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)]; }
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 1; }
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { return 82; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section { return @"提高数值会增加玻璃底色，使评论和输入框文字更容易辨认。"; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    UILabel *value = [UILabel new]; value.tag = 2; UISlider *slider = [UISlider new];
    slider.minimumValue = 0; slider.maximumValue = 0.8; slider.value = [NSUserDefaults.standardUserDefaults objectForKey:NGPreferenceCommentGlassOpacity] ? [NSUserDefaults.standardUserDefaults floatForKey:NGPreferenceCommentGlassOpacity] : 0.32;
    slider.accessibilityIdentifier = NGPreferenceCommentGlassOpacity; slider.tag = 1;
    [slider addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
    [cell.contentView addSubview:value]; [cell.contentView addSubview:slider]; value.translatesAutoresizingMaskIntoConstraints = NO; slider.translatesAutoresizingMaskIntoConstraints = NO;
    UILayoutGuide *margins = cell.contentView.layoutMarginsGuide;
    [NSLayoutConstraint activateConstraints:@[[value.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor], [value.centerYAnchor constraintEqualToAnchor:cell.contentView.centerYAnchor], [value.widthAnchor constraintEqualToConstant:54], [slider.leadingAnchor constraintEqualToAnchor:value.trailingAnchor constant:12], [slider.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor], [slider.centerYAnchor constraintEqualToAnchor:value.centerYAnchor]]];
    [self update:value slider:slider]; return cell;
}
- (void)update:(UILabel *)label slider:(UISlider *)slider { label.text = [NSString stringWithFormat:@"%.0f%%", slider.value * 100]; }
- (void)sliderChanged:(UISlider *)slider { [NSUserDefaults.standardUserDefaults setFloat:slider.value forKey:NGPreferenceCommentGlassOpacity]; UITableViewCell *cell = (UITableViewCell *)slider.superview.superview; [self update:[cell.contentView viewWithTag:2] slider:slider]; NGUpdateCommentGlass(); }
@end

#include "NGSupport.inc"
#include "NGSponsorPrompt.inc"

static NSArray<NSDictionary *> *NGSettingsGroups(void) {
    return @[
        @{@"title": @"播放布局", @"subtitle": @"元素显隐、位置与播放比例", @"symbol": @"rectangle.inset.filled", @"color": UIColor.systemIndigoColor, @"rows": @[
            NGSettingRow(@"显示 / 隐藏播放页元素", @"头像、动作、顶部标签、视频信息", @"eye.slash", nil, @"playbackVisibility"),
            NGSettingRow(@"位置与大小调节", @"右侧整体、作者名、剧名、简介、声明", @"arrow.up.left.and.arrow.down.right", nil, @"playbackLayout"),
            NGSettingRow(@"全屏播放", @"去除底部预留黑边；宽屏保留原始大小", @"arrow.up.left.and.arrow.down.right", NGPreferencePlaybackFullscreen, nil)
        ]},
        @{@"title": @"图标定制", @"subtitle": @"动作预设、底栏图标与相册图片", @"symbol": @"square.grid.2x2", @"color": UIColor.systemOrangeColor, @"rows": @[
            NGSettingRow(@"右侧动作图标", @"收藏、评论、点赞、分享；支持相册上传", @"paintbrush.pointed", nil, @"playbackIconPresets"),
            NGSettingRow(@"底栏图标预设与自定义", @"单项上传图片或输入 SF Symbols", @"rectangle.bottomthird.inset.filled", nil, @"dockIcons"),
            NGSettingRow(@"隐藏底栏项目", @"", @"line.3.horizontal.decrease", nil, @"dockItems")
        ]},
        @{@"title": @"液态外观", @"subtitle": @"玻璃材质、底栏颜色与面板", @"symbol": @"drop.halffull", @"color": UIColor.systemTealColor, @"rows": @[
            NGSettingRow(@"底栏外观与收缩", @"清透、居中收缩、头像、按钮功能与震动反馈", @"rectangle.bottomthird.inset.filled", nil, @"dockAppearance"),
            NGSettingRow(@"底栏颜色", @"未选中与已选中", @"circle.lefthalf.filled", nil, @"dockColors"),
            NGSettingRow(@"顶部液态容器", @"频道、合集返回、倍速与更多", @"capsule", NGPreferencePlaybackHeaderGlass, nil),
            NGSettingRow(@"全集入口悬浮液态", @"观看完整全集入口改为透明悬浮胶囊", @"play.rectangle", NGPreferenceCollectionGlass, nil),
            NGSettingRow(@"右侧液态按钮", @"", @"circle.grid.2x2", NGPreferencePlaybackGlass, nil),
            NGSettingRow(@"评论区液态玻璃", @"评论面板与输入框", @"text.bubble", NGPreferenceCommentGlass, nil),
            NGSettingRow(@"分享、长按与倍速面板", @"清透玻璃、柔雾玻璃", @"square.and.arrow.up", nil, @"shareGlassStyle"),
            NGSettingRow(@"评论区玻璃透明度", @"滑动调节玻璃底色和文字对比度", @"slider.horizontal.3", nil, @"commentGlassOpacity"),
        ]},
        @{@"title": @"工具与诊断", @"subtitle": @"页面结构、音频专项与问题采集", @"symbol": @"waveform.path.ecg", @"color": UIColor.systemRedColor, @"rows": @[
            NGSettingRow(@"调试工具", @"显示扳手入口；音频采集需开启后重启红果", @"wrench.fill", NGPreferenceHUD, nil),
            NGSettingRow(@"调试导出菜单", @"本页 ZIP / 音频专项 ZIP，分享前请检查隐私", @"square.and.arrow.up", nil, @"capture"),
            NGSettingRow(@"清除记录", @"", @"trash", nil, @"clearEvents")
        ]},
        @{@"title": @"字体与字号", @"subtitle": @"原生文字缩放与自定义字体", @"symbol": @"textformat.size", @"color": UIColor.systemPurpleColor, @"rows": @[
            NGSettingRow(@"全局字号与自定义字体", @"比例调节、已安装字体和字体文件导入", @"textformat.size", nil, @"typography")
        ]},

    ];
}

static UIImage *NGSettingsTile(NSString *symbol, UIColor *color) {
    UIImageSymbolConfiguration *configuration = [UIImageSymbolConfiguration configurationWithPointSize:23 weight:UIImageSymbolWeightRegular];
    return [[UIImage systemImageNamed:symbol withConfiguration:configuration] imageWithTintColor:color ?: UIColor.systemBlueColor renderingMode:UIImageRenderingModeAlwaysOriginal];
}

@interface NGBrandHeaderView : UIView
@property(nonatomic,strong) UIImageView *iconView;
@property(nonatomic,strong) UILabel *titleLabel;
@property(nonatomic,strong) UILabel *subtitleLabel;
@property(nonatomic,strong) UILabel *badgeLabel;
@end
@implementation NGBrandHeaderView
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self=[super initWithFrame:frame])) {
        _iconView=[[UIImageView alloc] initWithImage:NGHostIcon()]; _iconView.layer.cornerRadius=10; _iconView.clipsToBounds=YES; [self addSubview:_iconView];
        _titleLabel=[UILabel new]; _titleLabel.text=@"NaiwaGuo"; _titleLabel.font=NGSettingsFont(20,UIFontWeightSemibold); _titleLabel.textColor=NGSettingsInk(); [self addSubview:_titleLabel];
        _subtitleLabel=[UILabel new]; _subtitleLabel.text=@"外观与观看体验"; _subtitleLabel.font=NGSettingsFont(12,UIFontWeightRegular); _subtitleLabel.textColor=NGSettingsMuted(); [self addSubview:_subtitleLabel];
        _badgeLabel=[UILabel new]; _badgeLabel.text=[@"v" stringByAppendingString:NGVersion]; _badgeLabel.font=[UIFont monospacedSystemFontOfSize:11 weight:UIFontWeightRegular]; _badgeLabel.textColor=NGSettingsMuted(); _badgeLabel.textAlignment=NSTextAlignmentRight; [self addSubview:_badgeLabel];
    } return self;
}
- (void)layoutSubviews {
    [super layoutSubviews]; CGFloat w=self.bounds.size.width;
    _iconView.frame=CGRectMake(20,14,42,42); _titleLabel.frame=CGRectMake(74,12,MAX(0,w-164),28);
    _subtitleLabel.frame=CGRectMake(74,41,MAX(0,w-98),20); _badgeLabel.frame=CGRectMake(w-86,18,64,20);
}
@end
@interface NGSettingsCategoryCell : UITableViewCell
@property(nonatomic,strong) UIView *surface;
@property(nonatomic,strong) UIImageView *tile;
@property(nonatomic,strong) UILabel *heading;
@property(nonatomic,strong) UILabel *summary;
@property(nonatomic,strong) UIImageView *chevron;
@end
@implementation NGSettingsCategoryCell
- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)identifier {
    if ((self=[super initWithStyle:style reuseIdentifier:identifier])) {
        self.backgroundConfiguration=[UIBackgroundConfiguration clearConfiguration]; self.selectionStyle=UITableViewCellSelectionStyleNone;
        _surface=[UIView new]; _surface.backgroundColor=NGSettingsSurface(); _surface.layer.cornerRadius=14; _surface.layer.cornerCurve=kCACornerCurveContinuous; _surface.userInteractionEnabled=NO; [self.contentView addSubview:_surface];
        _tile=[UIImageView new]; _tile.contentMode=UIViewContentModeScaleAspectFit; _tile.tintColor=NGSettingsAccent(); [_surface addSubview:_tile];
        _heading=[UILabel new]; _heading.font=NGSettingsFont(15,UIFontWeightMedium); _heading.textColor=NGSettingsInk(); [_surface addSubview:_heading];
        _summary=[UILabel new]; _summary.font=NGSettingsFont(11,UIFontWeightRegular); _summary.textColor=NGSettingsMuted(); _summary.numberOfLines=2; [_surface addSubview:_summary];
        _chevron=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"chevron.right"]]; _chevron.contentMode=UIViewContentModeScaleAspectFit; _chevron.tintColor=UIColor.tertiaryLabelColor; [_surface addSubview:_chevron];
        self.isAccessibilityElement=YES; self.accessibilityTraits=UIAccessibilityTraitButton;
    } return self;
}
- (void)layoutSubviews {
    [super layoutSubviews]; _surface.frame=CGRectInset(self.contentView.bounds,0,1); CGFloat w=_surface.bounds.size.width,h=_surface.bounds.size.height;
    _tile.frame=CGRectMake(16,(h-23)/2,23,23); CGFloat left=53, textWidth=MAX(0,w-left-32);
    CGSize heading=[_heading sizeThatFits:CGSizeMake(textWidth,60)],summary=[_summary sizeThatFits:CGSizeMake(textWidth,80)]; CGFloat total=heading.height+4+summary.height;
    _heading.frame=CGRectMake(left,(h-total)/2,textWidth,heading.height); _summary.frame=CGRectMake(left,CGRectGetMaxY(_heading.frame)+4,textWidth,summary.height);
    _chevron.frame=CGRectMake(w-23,(h-12)/2,7,12);
}
@end

@interface NGSettingsGroupViewController : NGSettingsTableController
@property(nonatomic) NSUInteger groupIndex;
@property(nonatomic, copy) NSString *highlightSetting;
@end

// Keep normal navigation and search results on the same destination map.
static UIViewController *NGSettingsDestinationForAction(NSString *action) {
    if ([action isEqualToString:@"dockAppearance"]) return [NGDockAppearanceSettingsController new];
    if ([action isEqualToString:@"typography"]) return [NGTypographyController new];
    if ([action isEqualToString:@"sponsor"]) return [NGSupportViewController new];
    if ([action isEqualToString:@"community"]) return [NGCommunityViewController new];
    if ([action isEqualToString:@"dockItems"]) return [NGDockItemsSettingsController new];
    if ([action isEqualToString:@"dockColors"]) return [NGDockColorSettingsController new];
    if ([action isEqualToString:@"shareGlassStyle"]) return [NGShareGlassSettingsController new];
    if ([action isEqualToString:@"dockIcons"]) return [NGDockIconSettingsController new];
    if ([action isEqualToString:@"playbackIconPresets"]) return [NGPlaybackPresetSettingsController new];
    if ([action isEqualToString:@"playbackVisibility"]) return [NGPlaybackVisibilitySettingsController new];
    if ([action isEqualToString:@"playbackLayout"]) return [NGPlaybackLayoutSettingsController new];
    if ([action isEqualToString:@"commentGlassOpacity"]) return [NGCommentGlassOpacityController new];
    return nil;
}

@interface NGSettingsViewController () <UISearchResultsUpdating, UISearchControllerDelegate>
@property(nonatomic, strong) NSArray<NSDictionary *> *groups;
@property(nonatomic, strong) NSArray<NSDictionary *> *searchResults;
@property(nonatomic, copy) NSDictionary *pendingSearchMatch;
@property(nonatomic, strong) NGBrandHeaderView *brandHeader;
@property(nonatomic, strong) UIView *supportFooter;
@property(nonatomic,strong) UIButton *accessBanner;
@property(nonatomic) BOOL followPromptShown;
@property(nonatomic) BOOL sponsorPromptConsidered;
@property(nonatomic) BOOL accessCheckedForEntry;
- (void)refreshAccessUI;
- (void)presentSettingsPrompts;
- (void)checkAuthorAccess;
- (void)showSponsorPromptIfNeeded;
@end

@implementation NGSettingsViewController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"设置";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.view.backgroundColor = NGSettingsCanvas();
    self.tableView.backgroundColor = NGSettingsCanvas();
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 70;
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(close)];
    self.navigationItem.leftBarButtonItem.accessibilityLabel = @"返回";
    self.brandHeader = [[NGBrandHeaderView alloc] initWithFrame:CGRectMake(0, 0, 360, 80)];
    self.tableView.tableHeaderView = self.brandHeader;
    UISearchController *search = [[UISearchController alloc] initWithSearchResultsController:nil];
    search.searchResultsUpdater = self;
    search.delegate = self;
    search.obscuresBackgroundDuringPresentation = NO;
    search.searchBar.placeholder = @"搜索功能与设置";
    self.navigationItem.searchController = search;
    if (@available(iOS 16.0, *)) self.navigationItem.preferredSearchBarPlacement = UINavigationItemSearchBarPlacementStacked;
    if (@available(iOS 26.0, *)) self.navigationItem.searchBarPlacementAllowsToolbarIntegration = NO;
    self.navigationItem.hidesSearchBarWhenScrolling = NO;
    self.navigationItem.searchController.searchBar.searchTextField.backgroundColor = NGSettingsSurface();
    self.navigationItem.searchController.searchBar.searchTextField.tintColor = NGSettingsAccent();
    self.definesPresentationContext = YES;
    self.groups = NGSettingsGroups();
    self.searchResults = @[];
    [self installSupportFooter];
    self.accessBanner=[UIButton buttonWithType:UIButtonTypeSystem]; self.accessBanner.configuration=[UIButtonConfiguration tintedButtonConfiguration]; self.accessBanner.titleLabel.numberOfLines=2; self.accessBanner.titleLabel.textAlignment=NSTextAlignmentCenter; self.accessBanner.accessibilityIdentifier=@"NaiwaGuo.Access.Status";
    [self.accessBanner addTarget:self action:@selector(checkAuthorAccess) forControlEvents:UIControlEventTouchUpInside]; [self.brandHeader addSubview:self.accessBanner];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(refreshAccessUI) name:NGAccessChangedNotification object:nil]; [self refreshAccessUI];
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    self.groups = NGSettingsGroups();
    [self.tableView reloadData];
    [self refreshAccessUI];
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if (!self.accessCheckedForEntry) { self.accessCheckedForEntry=YES; NGAccessCheck(); }
    [self presentSettingsPrompts];
}
- (void)dealloc { [NSNotificationCenter.defaultCenter removeObserver:self]; }
- (void)refreshAccessUI {
    if (!self.isViewLoaded) return;
    BOOL allowed=NGAccessAllowed(); self.accessBanner.hidden=allowed;
    [self.accessBanner setTitle:[NGAccessStatusText() stringByAppendingString:@"\n点此重新检测 / 关注作者"] forState:UIControlStateNormal];
    self.accessBanner.enabled=NGAccessStatus!=NGAccessChecking;
    self.navigationItem.searchController.searchBar.userInteractionEnabled=allowed; self.navigationItem.searchController.searchBar.alpha=allowed ? 1 : .4;
    if (!allowed) {
        self.pendingSearchMatch=nil; self.navigationItem.searchController.active=NO;
        if ([self.navigationController.viewControllers containsObject:self] && self.navigationController.topViewController!=self) [self.navigationController popToViewController:self animated:NO];
    }
    [self.tableView reloadData]; [self.view setNeedsLayout];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(.4*NSEC_PER_SEC)),dispatch_get_main_queue(), ^{ [self presentSettingsPrompts]; });
}
- (void)checkAuthorAccess { self.followPromptShown=NO; NGAccessCheck(); }
- (void)showSponsorPromptIfNeeded {
#if NG_TEST
    if (!NGAccessTestPrompts) return;
#endif
    if (self.sponsorPromptConsidered || [NSUserDefaults.standardUserDefaults boolForKey:NGPreferenceSponsorNever] || !self.view.window || self.navigationController.topViewController!=self || self.presentedViewController) return;
    self.sponsorPromptConsidered=YES; [self presentViewController:[NGSponsorPromptController new] animated:YES completion:nil];
}
- (void)presentSettingsPrompts {
#if NG_TEST
    if (!NGAccessTestPrompts) return;
#endif
    if (!self.view.window || self.navigationController.topViewController!=self || self.presentedViewController || self.isBeingDismissed || self.navigationController.isBeingDismissed) return;
    if (NGAccessStatus==NGAccessChecking || NGAccessStatus==NGAccessUnknown) return;
    if (!NGAccessAllowed() && !self.followPromptShown) {
        self.followPromptShown=YES;
        BOOL canFollow=NGAccessStatus==NGAccessNotFollowing;
        NSString *message=canFollow ? [NSString stringWithFormat:@"使用插件前需要关注作者（红果用户 ID：%@）。是否确认关注？确认后会通过红果重新核验，未关注时功能列表保持灰色。",NGAuthorUserID] : [NGAccessStatusText() stringByAppendingString:@"。核验成功前，功能列表保持灰色。"];
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:canFollow ? @"关注作者后使用" : @"关注状态未确认" message:message preferredStyle:UIAlertControllerStyleAlert];
        __weak typeof(self) weakSelf=self;
        [alert addAction:[UIAlertAction actionWithTitle:canFollow ? @"暂不关注" : @"稍后再试" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action){ dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(.4*NSEC_PER_SEC)),dispatch_get_main_queue(),^{[weakSelf showSponsorPromptIfNeeded];}); }]];
        [alert addAction:[UIAlertAction actionWithTitle:canFollow ? @"确认关注" : @"重新检测" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){ if (canFollow) NGAccessFollowAuthor(); else { weakSelf.followPromptShown=NO; NGAccessCheck(); } }]];
        [self presentViewController:alert animated:YES completion:nil]; return;
    }
    [self showSponsorPromptIfNeeded];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    [self layoutSupportFooter];
    if (self.tableView.tableHeaderView == self.brandHeader) {
        CGFloat width = self.tableView.bounds.size.width;
        CGFloat height = 80 + MAX(0,self.brandHeader.titleLabel.font.pointSize-20)*2 + (NGAccessAllowed() ? 0 : 76);
        self.accessBanner.frame=CGRectMake(16,height-72,MAX(0,width-32),64);
        if (self.brandHeader.frame.size.width != width || self.brandHeader.frame.size.height != height) {
            self.brandHeader.frame = CGRectMake(0, 0, width, height);
            self.tableView.tableHeaderView = self.brandHeader;
        }
    }
}
- (void)installSupportFooter {
    UIView *footer = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.tableView.bounds.size.width, 136)];
    footer.backgroundColor = UIColor.clearColor; footer.accessibilityIdentifier = @"NaiwaGuo.Settings.SupportFooter";
    UIStackView *stack = [UIStackView new]; stack.axis = UILayoutConstraintAxisVertical; stack.spacing = 16;
    stack.translatesAutoresizingMaskIntoConstraints = NO; [footer addSubview:stack];
    NSArray *items = @[
        @[@"赞赏", @"heart", @"sponsor"],
        @[@"交流群", @"person.2", @"community"]
    ];
    for (NSArray *item in items) {
        UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
        if (@available(iOS 26.0, *)) button.configuration = [UIButtonConfiguration glassButtonConfiguration];
        else {
            UIButtonConfiguration *fallback = [UIButtonConfiguration tintedButtonConfiguration];
            fallback.background.backgroundColor = [NGSettingsSurface() colorWithAlphaComponent:0.72];
            fallback.background.cornerRadius = 28; button.configuration = fallback;
        }
        UIButtonConfiguration *configuration = button.configuration;
        configuration.title = item[0]; configuration.image = [UIImage systemImageNamed:item[1]];
        configuration.imagePadding = 10; configuration.baseForegroundColor = NGSettingsInk();
        configuration.contentInsets = NSDirectionalEdgeInsetsMake(13, 24, 13, 24);
        button.configuration = configuration; button.accessibilityIdentifier = [@"NaiwaGuo.Settings.Support." stringByAppendingString:item[2]];
        button.accessibilityLabel = item[0]; button.tag = [item[2] isEqualToString:@"sponsor"] ? 1 : 2;
        [button addTarget:self action:@selector(supportFooterTapped:) forControlEvents:UIControlEventTouchUpInside];
        [stack addArrangedSubview:button]; [button.heightAnchor constraintEqualToConstant:48].active = YES;
    }
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:footer.leadingAnchor constant:16],
        [stack.trailingAnchor constraintEqualToAnchor:footer.trailingAnchor constant:-16],
        [stack.topAnchor constraintEqualToAnchor:footer.topAnchor constant:8],
        [stack.bottomAnchor constraintEqualToAnchor:footer.bottomAnchor constant:-16]
    ]];
    self.tableView.tableFooterView = footer; self.supportFooter = footer;
}
- (void)layoutSupportFooter {
    UIView *footer = self.supportFooter; if (!footer) return;
    CGFloat width = self.tableView.bounds.size.width; if (width <= 0) return;
    CGFloat height = 136;
    if (footer.frame.size.width != width || footer.frame.size.height != height) {
        footer.frame = CGRectMake(0, 0, width, height); self.tableView.tableFooterView = footer;
    }
}
- (void)supportFooterTapped:(UIButton *)button {
    if (self.navigationController.topViewController != self || self.navigationController.transitionCoordinator) return;
    NGSettingsFeedback();
    NSString *action = button.tag == 1 ? @"sponsor" : @"community";
    UIViewController *destination = NGSettingsDestinationForAction(action);
    if (destination) [self.navigationController pushViewController:destination animated:YES];
}
- (void)close {
    if (self.navigationController.viewControllers.firstObject != self) [self.navigationController popViewControllerAnimated:YES];
    else [self dismissViewControllerAnimated:YES completion:nil];
}
- (BOOL)isSearching { return self.navigationItem.searchController.isActive && self.navigationItem.searchController.searchBar.text.length > 0; }
- (void)updateSearchResultsForSearchController:(UISearchController *)searchController {
    NSString *query = [searchController.searchBar.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if (!query.length) { self.searchResults = @[]; self.tableView.tableFooterView = self.supportFooter; [self.tableView reloadData]; return; }
    NSMutableArray *matches = [NSMutableArray array];
    [self.groups enumerateObjectsUsingBlock:^(NSDictionary *group, NSUInteger groupIndex, BOOL *stopGroup) {
        for (NSDictionary *row in group[@"rows"]) {
            NSMutableArray *terms = [NSMutableArray arrayWithArray:@[row[@"title"] ?: @"", row[@"subtitle"] ?: @"", group[@"title"] ?: @""]];
            if ([row[@"action"] isEqualToString:@"playbackVisibility"]) for (NSArray *section in NGPlaybackVisibilityItems()) for (NSDictionary *item in section) [terms addObject:item[@"title"]];
            if ([row[@"action"] isEqualToString:@"community"]) [terms addObject:@"QQ 交流群 TG 通知群 TG 交流群 Telegram"];
            NSString *haystack = [terms componentsJoinedByString:@" "];
            if ([haystack localizedCaseInsensitiveContainsString:query]) {
                [matches addObject:@{@"group": @(groupIndex), @"row": row, @"groupTitle": group[@"title"] ?: @""}];
            }
        }
    }];
    NSArray *supportRows = @[
        NGSettingRow(@"赞赏", @"请作者喝杯奶茶吧", @"heart", nil, @"sponsor"),
        NGSettingRow(@"交流群", @"QQ 交流群 · TG 通知群与交流群 · Telegram", @"person.2", nil, @"community")
    ];
    for (NSDictionary *row in supportRows) {
        NSString *terms = [NSString stringWithFormat:@"%@ %@ %@",row[@"title"],row[@"subtitle"],[row[@"action"] isEqualToString:@"community"] ? @"TG 交流群" : @""];
        if ([terms localizedCaseInsensitiveContainsString:query]) [matches addObject:@{@"group":@0,@"row":row,@"groupTitle":@"设置底部"}];
    }
    self.searchResults = matches;
    self.tableView.tableFooterView = [self isSearching] ? nil : self.supportFooter;
    [self.tableView reloadData];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [self isSearching] ? 1 : self.groups.count;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [self isSearching] ? self.searchResults.count : 1;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if ([self isSearching] || section != 0) return nil;
    return nil;
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if ([self isSearching]) return self.searchResults.count ? nil : @"没有找到匹配的设置";
    return nil;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { return [self isSearching] ? UITableViewAutomaticDimension : MAX(64,64+(NGSettingsFont(15,UIFontWeightMedium).pointSize-15)*4); }
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section { return 3; }
- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section { return [self isSearching] && !self.searchResults.count ? UITableViewAutomaticDimension : 3; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.detailTextLabel.numberOfLines = 2;
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    if ([self isSearching]) {
        NSDictionary *match = self.searchResults[indexPath.row];
        NSDictionary *row = match[@"row"];
        cell.textLabel.text = row[@"title"];
        cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ · %@", match[@"groupTitle"], row[@"subtitle"]];
        cell.imageView.image = [UIImage systemImageNamed:row[@"symbol"] ?: @"gear"];
        cell.imageView.tintColor = self.groups[[match[@"group"] unsignedIntegerValue]][@"color"] ?: UIColor.systemBlueColor;
        if (!NGAccessAllowed() && ![@[@"sponsor",@"community"] containsObject:row[@"action"] ?: @""]) { cell.userInteractionEnabled=NO; cell.contentView.alpha=.38; cell.accessibilityTraits|=UIAccessibilityTraitNotEnabled; }
        return cell;
    }
    NSUInteger groupIndex = indexPath.section;
    NSDictionary *group = self.groups[groupIndex];
    UIColor *color = group[@"color"] ?: UIColor.systemBlueColor;
    NGSettingsCategoryCell *category = [[NGSettingsCategoryCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    category.heading.text = group[@"title"];
    category.summary.text = group[@"subtitle"];
    category.tile.image = NGSettingsTile(group[@"symbol"] ?: @"gear", color);
    category.tile.tintColor = color;
    category.accessibilityLabel = [NSString stringWithFormat:@"%@，%@", group[@"title"], group[@"subtitle"]];
    category.accessibilityIdentifier = [NSString stringWithFormat:@"NaiwaGuo.Settings.Group.%lu", (unsigned long)indexPath.section];
    if (!NGAccessAllowed()) { category.userInteractionEnabled=NO; category.contentView.alpha=.38; category.accessibilityTraits|=UIAccessibilityTraitNotEnabled; }
    return category;
}
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)path { return NGAccessAllowed() ? path : nil; }
- (void)openPendingSearchResult {
    NSDictionary *match = self.pendingSearchMatch;
    self.pendingSearchMatch = nil;
    if (!NGAccessAllowed() || !match || self.navigationController.topViewController != self) return;
    NSUInteger groupIndex = [match[@"group"] unsignedIntegerValue];
    if (groupIndex >= self.groups.count) return;
    NSDictionary *row = match[@"row"];
    UIViewController *destination = NGSettingsDestinationForAction(row[@"action"]);
    if (!destination) {
        // Search locates switches and external/destructive actions; it never runs them.
        NGSettingsGroupViewController *group = [NGSettingsGroupViewController new];
        group.groupIndex = groupIndex;
        group.highlightSetting = row[@"key"] ?: row[@"action"];
        destination = group;
    }
    [self.navigationController pushViewController:destination animated:YES];
}
- (void)didDismissSearchController:(UISearchController *)searchController {
    if (searchController != self.navigationItem.searchController) return;
    self.tableView.tableFooterView = self.supportFooter;
    if (!self.pendingSearchMatch) return;
    __weak typeof(self) weakSelf = self;
    // Let UIKit finish its own navigation-bar cleanup before starting the next transition.
    dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf openPendingSearchResult]; });
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (!NGAccessAllowed() || self.pendingSearchMatch || self.navigationController.topViewController != self) return;
    if ([self isSearching]) {
        if (indexPath.row < 0 || (NSUInteger)indexPath.row >= self.searchResults.count) return;
        self.pendingSearchMatch = self.searchResults[indexPath.row];
        // Pushing during search dismissal loses the transition on some iOS versions.
        // UISearchControllerDelegate resumes navigation only after it has finished.
        self.navigationItem.searchController.active = NO;
        return;
    }
    if (indexPath.section < 0 || (NSUInteger)indexPath.section >= self.groups.count) return;
    NGSettingsGroupViewController *detail = [NGSettingsGroupViewController new];
    detail.groupIndex = indexPath.section;
    [self.navigationController pushViewController:detail animated:YES];
}
@end

@implementation NGSettingsGroupViewController
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (NSDictionary *)group { return NGSettingsGroups()[self.groupIndex]; }
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = self.group[@"title"];
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
    self.navigationItem.leftBarButtonItem.accessibilityLabel = @"返回";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.view.backgroundColor = NGSettingsCanvas();
    self.tableView.backgroundColor = NGSettingsCanvas();
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 64;
    self.tableView.tableFooterView = [UIView new];
    NGSettingsSectionIntro *intro = [[NGSettingsSectionIntro alloc] initWithFrame:CGRectMake(0,0,self.tableView.bounds.size.width,144)];
    intro.heading.text = self.group[@"title"]; intro.caption.text = self.group[@"subtitle"]; self.tableView.tableHeaderView = intro;
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self.tableView reloadData];
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if (!self.highlightSetting.length) return;
    NSArray *rows = self.group[@"rows"];
    [rows enumerateObjectsUsingBlock:^(NSDictionary *row, NSUInteger index, BOOL *stop) {
        NSString *identifier = row[@"key"] ?: row[@"action"];
        if ([identifier isEqualToString:self.highlightSetting]) {
            [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:index inSection:0] atScrollPosition:UITableViewScrollPositionMiddle animated:YES];
            *stop = YES;
        }
    }];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 1; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return [self.group[@"rows"] count]; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section { return self.group[@"footer"]; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSDictionary *row = self.group[@"rows"][indexPath.row];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text = row[@"title"];
    cell.detailTextLabel.text = row[@"subtitle"];
    cell.detailTextLabel.numberOfLines = 0;
    cell.imageView.image = [UIImage systemImageNamed:row[@"symbol"] ?: @"gear"];
    cell.imageView.tintColor = self.group[@"color"] ?: UIColor.systemBlueColor;
    NSString *key = row[@"key"];
    NSString *action = row[@"action"];
    if ([action isEqualToString:@"clearEvents"]) {
        cell.textLabel.textColor = UIColor.systemRedColor;
        cell.imageView.tintColor = UIColor.systemRedColor;
    }
    if (key.length) {
        UISwitch *toggle = [UISwitch new];
        toggle.on = [NSUserDefaults.standardUserDefaults boolForKey:key];
        toggle.accessibilityIdentifier = key;
        toggle.accessibilityLabel = row[@"title"];
        [toggle addTarget:self action:@selector(settingChanged:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = toggle;
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    } else if (action.length) {
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    } else {
        cell.accessoryType = UITableViewCellAccessoryNone;
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.detailTextLabel.textColor = UIColor.secondaryLabelColor;
    }
    return cell;
}
- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)path {
    [super tableView:tableView willDisplayCell:cell forRowAtIndexPath:path];
    NSDictionary *row = self.group[@"rows"][path.row];
    if (![(row[@"key"] ?: row[@"action"]) isEqualToString:self.highlightSetting]) return;
    UIBackgroundConfiguration *background = [cell.backgroundConfiguration copy];
    background.backgroundColor = [NGSettingsAccent() colorWithAlphaComponent:0.12];
    background.strokeColor = [NGSettingsAccent() colorWithAlphaComponent:0.35];
    background.strokeWidth = 1;
    cell.backgroundConfiguration = background;
}
- (void)settingChanged:(UISwitch *)sender {
    NSString *key = sender.accessibilityIdentifier;
    [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:key];
    NGPrepareRuntimeGlass();
    NGUpdateDock();
    if ([key isEqualToString:NGPreferenceHUD]) {
        NGDAudioProbePreferenceDidChange(); NGInstallOverlay();
    } else if ([key isEqualToString:NGPreferencePerformance]) {
        NGSetPerformanceEnabled(sender.on);
    } else if ([key isEqualToString:NGPreferencePlaybackGlass]) {
        NGSetPlaybackGlassEnabled(sender.on);
    } else if ([key isEqualToString:NGPreferencePlaybackHeaderGlass]) {
        NGUpdatePlaybackHeaderGlass();
    } else if ([key isEqualToString:NGPreferenceCollectionGlass]) {
        NGUpdatePlaybackTextLayout(); NGRefreshPlaybackCorners();
    } else if ([key isEqualToString:NGPreferenceCommentGlass]) {
        NGInstallCommentGlassHooks();
        NGUpdateCommentGlass();

    } else if ([key isEqualToString:NGPreferenceShareGlass]) {
        NGInstallCommentGlassHooks();
        NGUpdateCommentGlass();
    } else if ([key isEqualToString:NGPreferencePlaybackFullscreen]) {
        NGInstallPlaybackFullscreenHooks(); NGUpdatePlaybackFullscreen();

    }
    NGSettingsFeedback();
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    NSDictionary *row = self.group[@"rows"][indexPath.row];
    NSString *action = row[@"action"];
    UIViewController *destination = NGSettingsDestinationForAction(action);
    if (destination) {
        NGSettingsFeedback();
        [self.navigationController pushViewController:destination animated:YES];
    } else if (NGCommunityURL(action)) {
        NGOpenCommunity(action, self);
    } else if ([action isEqualToString:@"capture"]) {
        [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceHUD];
        NGDAudioProbePreferenceDidChange(); NGDDebugInspectorPresentMenu(); [self.tableView reloadData];
    } else if ([action isEqualToString:@"clearEvents"]) {
        [NGEventLog.shared clear];
        [self.tableView reloadData];
    } else if ([action isEqualToString:@"applyPlaybackGlass"]) {
        NGSetPlaybackGlassEnabled(YES);
        [self.tableView reloadData];
    }
}
@end

static void NGInstallLifecycleHooks(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        Class controllerClass = UIViewController.class;
        Method method = class_getInstanceMethod(controllerClass, @selector(viewDidAppear:));
        IMP original = method_getImplementation(method);
        method_setImplementation(method, imp_implementationWithBlock(^(UIViewController *controller, BOOL animated) {
            ((void (*)(id, SEL, BOOL))original)(controller, @selector(viewDidAppear:), animated);
            if (![NSStringFromClass(controller.class) hasPrefix:@"NG"] && ![controller.view.accessibilityIdentifier hasPrefix:NGIdentifierPrefix]) {
                NGLog(@"page", @"view-did-appear", @{
                    @"class": NGClassName(controller),
                    @"title": NGTrimmedString(controller.title, 200) ?: @""
                });
            }
            NGInstallOverlay();
            NGInstallNativeSettings();
            
            NGUpdateDock();
            NGInstallPlaybackHeaderGlassHook();
            NGUpdatePlaybackHeaderGlass();
            NGInstallPlaybackGlassHook();
            NGUpdatePlaybackGlass();
            NGInstallCommentGlassHooks();
            NGInstallPlaybackFullscreenHooks();
            NGUpdatePlaybackFullscreen();
        }));
        Method disappearMethod = class_getInstanceMethod(controllerClass, @selector(viewDidDisappear:));
        IMP disappearOriginal = method_getImplementation(disappearMethod);
        method_setImplementation(disappearMethod, imp_implementationWithBlock(^(UIViewController *controller, BOOL animated) {
            ((void (*)(id, SEL, BOOL))disappearOriginal)(controller, @selector(viewDidDisappear:), animated);
            NGUpdateDock();
        }));
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            NGLog(@"lifecycle", @"foreground", nil);
            NGInstallAuthorAccessHooks(); NGAccessCheck();
            [NGPerformanceRecorder.shared resume];
            NGInstallOverlay();
            
            NGInstallPlaybackGlassHook();
            NGUpdatePlaybackGlass();
            NGInstallCommentGlassHooks();
            NGInstallPlaybackHeaderGlassHook();
            NGUpdatePlaybackHeaderGlass();
            NGInstallPlaybackFullscreenHooks();
            NGUpdatePlaybackFullscreen();
            NGUpdateDock();
        }];
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidEnterBackgroundNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            NGLog(@"lifecycle", @"background", nil);
            [NGPerformanceRecorder.shared pause];
        }];
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationWillResignActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {

            NGLog(@"lifecycle", @"resign-active", nil);
        }];
    });
}

static BOOL NGIsHostApp(void) {
    NSBundle *bundle = NSBundle.mainBundle;
    return [bundle.bundleIdentifier isEqualToString:NGHostBundleIdentifier] ||
           [bundle.infoDictionary[@"CFBundleExecutable"] isEqualToString:NGHostExecutable];
}

static void NGRestoreLegacyTopChannels(NSUserDefaults *defaults) {
    if ([defaults boolForKey:NGPreferenceChannelRestoreMigration]) return;
    for (NSString *key in @[@"com.naiwaguo.playback.hidden.header.follow", @"com.naiwaguo.playback.hidden.header.manju", @"com.naiwaguo.playback.hidden.header.live", @"com.naiwaguo.playback.hidden.header.recommended"])
        [defaults removeObjectForKey:key];
    [defaults setBool:YES forKey:NGPreferenceChannelRestoreMigration];
}

static void NGRemoveRetiredPlaybackPreferences(NSUserDefaults *defaults) {
    for (NSString *key in @[@"com.naiwaguo.playback.xStyle", @"com.naiwaguo.playback.commentVideoFreeze", @"com.naiwaguo.playback.fullscreenLandscape", @"com.naiwaguo.playback.hideStatus"])
        [defaults removeObjectForKey:key];
}

static void NGRefreshAfterAccessChange(void) {
    NGLoadTypographyFont(); NGRefreshTypography(); NGUpdateDock(); NGUpdatePlaybackGlass(); NGUpdatePlaybackHeaderGlass(); NGUpdateCommentGlass(); NGUpdatePlaybackFullscreen(); NGUpdatePlaybackTextLayout(); NGRefreshPlaybackCorners(); NGInstallOverlay(); NGDAudioProbePreferenceDidChange();
}

static void NGStart(void) {
    if (!NGIsHostApp()) return;
    NGInstallAuthorAccessHooks();
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NGRemoveRetiredPlaybackPreferences(defaults);
    [defaults registerDefaults:@{
        NGPreferenceHUD: @NO,
        NGPreferenceDock: @NO,
        NGPreferenceDockHaptics: @NO,
        NGPreferenceHiddenDockItems: @[],
        NGPreferencePerformance: @NO,
        NGPreferenceRuntimeGlass: @NO,
        NGPreferencePlaybackGlass: @NO,
        NGPreferenceCommentGlass: @NO,
        NGPreferenceCommentGlassOpacity: @0.32,
        NGPreferenceShareGlass: @NO,
        NGPreferenceShareGlassClear: @NO,
        NGPreferencePlaybackFullscreen: @NO,
        NGPreferencePlaybackHeaderGlass: @NO,
        NGPreferenceCollectionGlass: @NO,
        NGPreferencePlaybackPreset: @"original"
    }];
    NGRestoreLegacyTopChannels(defaults);
    NGPrepareRuntimeGlass();
    NGLoadTypographyFont(); NGInstallTypographyHooks(); NGRefreshTypography();
    NGInstallLifecycleHooks();
    NGInstallNativeSettings();
    
    NGInstallDockHooks();
    NGInstallPlaybackGlassHook();
    NGInstallPlaybackHeaderGlassHook();
    NGInstallCommentGlassHooks();
    NGInstallPlaybackTextHooks();
    NGInstallPlaybackFullscreenHooks();
    NGUpdatePlaybackFullscreen();
    NGUpdatePlaybackTextLayout();
    NGLog(@"lifecycle", @"plugin-loaded", @{@"version": NGVersion, @"bundle": NSBundle.mainBundle.bundleIdentifier ?: @"unknown"});
#ifdef NG_TEST
    if ([defaults boolForKey:NGPreferencePerformance]) NGSetPerformanceEnabled(YES);
#else
    [defaults setBool:NO forKey:NGPreferencePerformance];
#endif
    NGDAudioHooksInstall(); NGDDebugInspectorInstall(); NGDRegisterFLEXKingProvider();
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.7 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NGAccessCheck(); NGInstallOverlay();
        
        NGInstallPlaybackGlassHook();
        NGUpdatePlaybackGlass();
        NGInstallPlaybackHeaderGlassHook();
        NGUpdatePlaybackHeaderGlass();
        NGInstallCommentGlassHooks();
        NGInstallPlaybackTextHooks();
        NGInstallPlaybackFullscreenHooks();
        NGUpdatePlaybackFullscreen();
        NGUpdatePlaybackTextLayout();
    });
}

#include "NGDebugHostBridge.inc"

#ifndef NG_TEST
__attribute__((constructor)) static void NGNaiwaGuoEntry(void) {
    @autoreleasepool {
        if (NGIsHostApp()) NGInstallAccessPreferenceHooks();
        NGPrepareRuntimeGlass();
        if (NGIsHostApp()) { NGDAudioHooksInstall(); NGDAudioProbeInstallIfEnabled(YES); }
        dispatch_async(dispatch_get_main_queue(), ^{ NGStart(); });
    }
}
#endif
