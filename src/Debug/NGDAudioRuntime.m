// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDAudioRuntime.m
//  NaiwaGuo
//
//  只读采集当前媒体候选、受控 ivar 图和玻璃底栏几何。
//  不读取正文、URL、账号字段，不调用任意私有 getter。
//

#import "NGDAudioRuntime.h"
#import "NGDAudioProbe.h"
#import "NGDDebugCapture.h"
#import "NGDGlassTabBar.h"

#import <AVFAudio/AVFAudio.h>
#import <objc/message.h>
#import <objc/runtime.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

static NSString *NGDRuntimeCString(const char *value) {
    if (!value) return @"";
    NSString *utf8 = [NSString stringWithUTF8String:value];
    if (utf8) return utf8;
    size_t length = strnlen(value, 4096);
    NSString *latin = [[NSString alloc] initWithBytes:value length:length encoding:NSISOLatin1StringEncoding];
    if (latin) return latin;
    NSMutableString *hex = [NSMutableString stringWithString:@"<bytes:"];
    for (size_t i = 0; i < MIN(length, (size_t)64); i++) [hex appendFormat:@"%02x", (unsigned char)value[i]];
    [hex appendString:@">"];
    return hex;
}

static NSString *NGDRuntimePointer(const void *pointer) {
    return [NSString stringWithFormat:@"%p", pointer];
}

static BOOL NGDRuntimeContainsToken(NSString *value) {
    NSString *lower = value.lowercaseString;
    if (lower.length == 0) return NO;
    static NSArray<NSString *> *tokens;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        tokens = @[ @"audio", @"player", @"playback", @"pcm", @"render", @"sound",
                    @"volume", @"mute", @"music", @"media", @"mixer", @"engine",
                    @"stream", @"video", @"photo", @"note", @"rich", @"awemeplay",
                    @"ttav", @"wrapper", @"decoder" ];
    });
    for (NSString *token in tokens) if ([lower containsString:token]) return YES;
    return NO;
}

static UIViewController *NGDRuntimeViewControllerForView(UIView *view) {
    UIResponder *responder = view;
    for (NSUInteger i = 0; responder && i < 80; i++) {
        responder = responder.nextResponder;
        if ([responder isKindOfClass:UIViewController.class]) return (UIViewController *)responder;
    }
    return nil;
}

static NSArray<NSString *> *NGDRuntimeStateSelectors(void) {
    static NSArray<NSString *> *selectors;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        selectors = @[
            @"isPlaying", @"feed_isPlaying", @"isAutoPlaying", @"isBackgroundPlaying",
            @"isPaused", @"isPausedByUserClick", @"isPlayStarted", @"playerState",
            @"playState", @"rate", @"playbackRate", @"volume", @"mute", @"isMuted",
            @"timeControlStatus", @"status", @"isRunning", @"outputVolume",
            @"inputAvailable", @"otherAudioPlaying",
            @"currentPlaybackTime", @"currPlaybackTime", @"currentPlayerPlaybackTime",
            @"feed_currentPlaybackTime", @"totalPlayTime", @"duration",
            @"hasPreparedPlayer", @"isPlayerInStall",
            // AVAudioPlayer / AVAudioEngine 的身份读数，用于分辨图文 BGM 走的是哪条链路。
            @"numberOfChannels", @"isMeteringEnabled", @"numberOfLoops", @"currentTime"
        ];
    });
    return selectors;
}

static const char *NGDRuntimeSkipTypeQualifiers(const char *type) {
    while (type && strchr("rnNoORV", type[0])) type++;
    return type ?: "";
}

static id NGDRuntimeInvokeScalarGetter(id object, NSString *selectorName) {
    if (!object || selectorName.length == 0) return nil;
    @try {
        SEL selector = NSSelectorFromString(selectorName);
        if (![object respondsToSelector:selector]) return nil;
        NSMethodSignature *signature = [object methodSignatureForSelector:selector];
        if (!signature || signature.numberOfArguments != 2 || signature.methodReturnLength > 16) return nil;
        const char *type = NGDRuntimeSkipTypeQualifiers(signature.methodReturnType);
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
        invocation.target = object;
        invocation.selector = selector;
        [invocation invoke];
        switch (type[0]) {
            case 'B': { BOOL v = NO; [invocation getReturnValue:&v]; return @(v); }
            case 'c': { char v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'C': { unsigned char v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 's': { short v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'S': { unsigned short v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'i': { int v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'I': { unsigned int v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'l': { long v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'L': { unsigned long v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'q': { long long v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'Q': { unsigned long long v = 0; [invocation getReturnValue:&v]; return @(v); }
            case 'f': { float v = 0; [invocation getReturnValue:&v]; return isfinite(v) ? @(v) : @0; }
            case 'd': { double v = 0; [invocation getReturnValue:&v]; return isfinite(v) ? @(v) : @0; }
            default: return nil;
        }
    } @catch (__unused NSException *exception) {
        return nil;
    }
}

static NSDictionary *NGDRuntimeStateValues(id object) {
    NSMutableDictionary *values = [NSMutableDictionary dictionary];
    for (NSString *selector in NGDRuntimeStateSelectors()) {
        id value = NGDRuntimeInvokeScalarGetter(object, selector);
        if (value) values[selector] = value;
    }
    return values;
}

static BOOL NGDRuntimeStateShowsPlaying(NSDictionary *state) {
    for (NSString *key in @[ @"isPlaying", @"feed_isPlaying", @"isAutoPlaying", @"isBackgroundPlaying" ]) {
        if ([state[key] boolValue]) return YES;
    }
    return [state[@"rate"] doubleValue] > 0.0001 || [state[@"playbackRate"] doubleValue] > 0.0001;
}

static BOOL NGDRuntimeStateShowsPaused(NSDictionary *state) {
    if ([state[@"isPaused"] boolValue] || [state[@"isPausedByUserClick"] boolValue]) return YES;
    BOOL hasPlayingFlag = state[@"isPlaying"] || state[@"feed_isPlaying"];
    return hasPlayingFlag && ![state[@"isPlaying"] boolValue] && ![state[@"feed_isPlaying"] boolValue];
}

static BOOL NGDRuntimeViewIsActuallyVisible(UIView *view) {
    UIWindow *window = view.window;
    if (!window || window.hidden || window.alpha <= 0.01 || view.hidden || view.alpha <= 0.01) return NO;
    if (@available(iOS 13.0, *)) {
        UISceneActivationState state = window.windowScene.activationState;
        if (state != UISceneActivationStateForegroundActive &&
            state != UISceneActivationStateForegroundInactive) return NO;
    }
    CGRect visibleRect = [view convertRect:view.bounds toView:window];
    if (CGRectIsEmpty(visibleRect) || !CGRectIntersectsRect(visibleRect, window.bounds)) return NO;
    CGFloat accumulatedAlpha = 1.0;
    for (UIView *cursor = view; cursor; cursor = cursor.superview) {
        if (cursor.hidden) return NO;
        accumulatedAlpha *= cursor.alpha;
        if (accumulatedAlpha <= 0.01) return NO;
        if (cursor.clipsToBounds && cursor.superview) {
            CGRect clip = [cursor convertRect:cursor.bounds toView:window];
            visibleRect = CGRectIntersection(visibleRect, clip);
            if (CGRectIsNull(visibleRect) || CGRectIsEmpty(visibleRect)) return NO;
        }
    }
    return YES;
}

static NSDictionary *NGDRuntimeCandidateForView(UIView *view) {
    UIWindow *window = view.window;
    CGRect frameInWindow = window ? [view convertRect:view.bounds toView:window] : CGRectZero;
    BOOL visible = NGDRuntimeViewIsActuallyVisible(view);
    UIViewController *controller = NGDRuntimeViewControllerForView(view);
    id owner = view.nextResponder;
    return @{
        @"kind": @"view",
        @"class": NSStringFromClass(view.class) ?: @"",
        @"address": NGDRuntimePointer((__bridge const void *)view),
        @"frame": NSStringFromCGRect(view.frame),
        @"frameInWindow": NSStringFromCGRect(frameInWindow),
        @"hidden": @(view.hidden), @"alpha": @(view.alpha), @"visible": @(visible),
        @"windowClass": window ? NSStringFromClass(window.class) : @"",
        @"windowLevel": @(window.windowLevel),
        @"nearestViewController": controller ? NSStringFromClass(controller.class) : @"",
        @"ownerClass": owner ? NSStringFromClass([owner class]) : @"",
        @"viewState": NGDRuntimeStateValues(view),
        @"ownerState": NGDRuntimeStateValues(owner),
        @"controllerState": NGDRuntimeStateValues(controller),
    };
}

static void NGDRuntimeCollectViews(UIView *root,
                                  NSMutableArray<NSDictionary *> *candidates) {
    if (!root) return;
    id owner = root.nextResponder;
    if (NGDRuntimeContainsToken(NSStringFromClass(root.class)) ||
        NGDRuntimeContainsToken(owner ? NSStringFromClass([owner class]) : @"")) {
        [candidates addObject:NGDRuntimeCandidateForView(root)];
    }
    for (UIView *subview in root.subviews) NGDRuntimeCollectViews(subview, candidates);
}

static void NGDRuntimeCollectVCs(UIViewController *controller,
                                NSMutableArray<NSDictionary *> *candidates,
                                NSHashTable *seen) {
    if (!controller || [seen containsObject:controller]) return;
    [seen addObject:controller];
    NSString *className = NSStringFromClass(controller.class) ?: @"";
    BOOL loaded = controller.isViewLoaded;
    UIView *view = loaded ? controller.view : nil;
    NSDictionary *state = NGDRuntimeStateValues(controller);
    BOOL relevantIvar = NO;
    @try {
        for (Class cursor = controller.class; cursor && cursor != NSObject.class && !relevantIvar;
             cursor = class_getSuperclass(cursor)) {
            unsigned int count = 0;
            Ivar *ivars = class_copyIvarList(cursor, &count);
            for (unsigned int i = 0; i < count; i++) {
                if (NGDRuntimeContainsToken(NGDRuntimeCString(ivar_getName(ivars[i]))) ||
                    NGDRuntimeContainsToken(NGDRuntimeCString(ivar_getTypeEncoding(ivars[i])))) {
                    relevantIvar = YES;
                    break;
                }
            }
            if (ivars) free(ivars);
        }
    } @catch (__unused NSException *exception) {}
    if (NGDRuntimeContainsToken(className) || state.count || relevantIvar) {
        [candidates addObject:@{
            @"kind": @"viewController", @"class": className,
            @"address": NGDRuntimePointer((__bridge const void *)controller),
            @"viewLoaded": @(loaded), @"visible": @(loaded && NGDRuntimeViewIsActuallyVisible(view)),
            @"relevantIvar": @(relevantIvar), @"state": state,
        }];
    }
    for (UIViewController *child in controller.childViewControllers) {
        NGDRuntimeCollectVCs(child, candidates, seen);
    }
    if (controller.presentedViewController) {
        NGDRuntimeCollectVCs(controller.presentedViewController, candidates, seen);
    }
}

static NSDictionary *NGDRuntimeCollectCandidates(void) {
    NSMutableArray<NSDictionary *> *candidates = [NSMutableArray array];
    NSHashTable *seenVCs = [NSHashTable hashTableWithOptions:NSHashTableObjectPointerPersonality];
    for (UIWindow *window in NGDDebugActiveWindows()) {
        NGDRuntimeCollectViews(window, candidates);
        NGDRuntimeCollectVCs(window.rootViewController, candidates, seenVCs);
    }

    for (id object in NGDAudioProbeObservedObjects()) {
        if (!object) continue;
        [candidates addObject:@{
            @"kind": @"observedAudioObject",
            @"class": NSStringFromClass([object class]) ?: @"",
            @"address": NGDRuntimePointer((__bridge const void *)object),
            @"visible": @NO,
            @"state": NGDRuntimeStateValues(object),
        }];
    }

    BOOL playing = NO;
    BOOL paused = NO;
    for (NSDictionary *candidate in candidates) {
        BOOL visible = [candidate[@"visible"] boolValue];
        if (!visible) continue;
        for (NSString *key in @[ @"state", @"viewState", @"ownerState", @"controllerState" ]) {
            NSDictionary *state = [candidate[key] isKindOfClass:NSDictionary.class] ? candidate[key] : @{};
            playing |= NGDRuntimeStateShowsPlaying(state);
            paused |= NGDRuntimeStateShowsPaused(state);
        }
    }
    return @{
        @"capturedAt": [NSDate.date description],
        @"derivedState": playing ? @"playing" : (paused ? @"paused" : @"unknown"),
        @"candidateCount": @(candidates.count),
        @"candidates": candidates,
    };
}

NSDictionary *NGDAudioRuntimeMediaSnapshot(void) {
    if (![NSThread isMainThread]) return @{ @"error": @"media snapshot must run on main thread" };
    return NGDRuntimeCollectCandidates();
}

static NSArray *NGDRuntimeClipAncestors(UIView *view) {
    NSMutableArray *ancestors = [NSMutableArray array];
    UIWindow *window = view.window;
    for (UIView *cursor = view; cursor && ancestors.count < 64; cursor = cursor.superview) {
        CGRect converted = window ? [cursor convertRect:cursor.bounds toView:window] : CGRectZero;
        [ancestors addObject:@{
            @"class": NSStringFromClass(cursor.class) ?: @"",
            @"address": NGDRuntimePointer((__bridge const void *)cursor),
            @"boundsInWindow": NSStringFromCGRect(converted),
            @"clipsToBounds": @(cursor.clipsToBounds),
            @"hidden": @(cursor.hidden), @"alpha": @(cursor.alpha),
        }];
    }
    return ancestors;
}

static NSDictionary *NGDRuntimeViewGeometry(UIView *view) {
    if (!view) return @{};
    UIWindow *window = view.window;
    CGRect inWindow = window ? [view convertRect:view.bounds toView:window] : CGRectZero;
    CGRect onScreen = window ? [window convertRect:inWindow toCoordinateSpace:UIScreen.mainScreen.coordinateSpace] : CGRectZero;
    NSMutableDictionary *result = [@{
        @"class": NSStringFromClass(view.class) ?: @"",
        @"address": NGDRuntimePointer((__bridge const void *)view),
        @"frame": NSStringFromCGRect(view.frame), @"bounds": NSStringFromCGRect(view.bounds),
        @"frameInWindow": NSStringFromCGRect(inWindow), @"frameOnScreen": NSStringFromCGRect(onScreen),
        @"hidden": @(view.hidden), @"alpha": @(view.alpha), @"clipsToBounds": @(view.clipsToBounds),
        @"cornerRadius": @(view.layer.cornerRadius), @"masksToBounds": @(view.layer.masksToBounds),
        @"ancestors": NGDRuntimeClipAncestors(view),
    } mutableCopy];
    if (@available(iOS 26.0, *)) {
        result[@"effectiveCornerRadii"] = @{
            @"topLeft": @([view effectiveRadiusForCorner:UIRectCornerTopLeft]),
            @"topRight": @([view effectiveRadiusForCorner:UIRectCornerTopRight]),
            @"bottomLeft": @([view effectiveRadiusForCorner:UIRectCornerBottomLeft]),
            @"bottomRight": @([view effectiveRadiusForCorner:UIRectCornerBottomRight]),
        };
    }
    return result;
}

NSDictionary *NGDAudioRuntimeGlassTarget(void) {
    if (![NSThread isMainThread]) return @{ @"error": @"glass target must run on main thread" };
    UITabBar *bar = NGDGlassTabBarCurrent();
    UIVisualEffectView *plus = NGDGlassPlusKeyCurrent();
    UIView *platter = NGDGlassPlatterCurrent();
    UIWindow *window = bar.window ?: plus.window ?: NGDDebugTargetWindow();
    CGRect unionRect = CGRectNull;
    if (platter.window) unionRect = [platter convertRect:platter.bounds toView:window];
    if (plus.window) {
        CGRect plusRect = [plus convertRect:plus.bounds toView:window];
        unionRect = CGRectIsNull(unionRect) ? plusRect : CGRectUnion(unionRect, plusRect);
    }
    return @{
        @"capturedAt": [NSDate.date description],
        @"bar": NGDRuntimeViewGeometry(bar),
        @"platter": NGDRuntimeViewGeometry(platter),
        @"plusKey": NGDRuntimeViewGeometry(plus),
        @"visualizerEnvelopeInWindow": CGRectIsNull(unionRect) ? @"" : NSStringFromCGRect(unionRect),
        @"windowBounds": window ? NSStringFromCGRect(window.bounds) : @"",
        @"safeAreaInsets": window ? NSStringFromUIEdgeInsets(window.safeAreaInsets) : @"",
        @"screen": @{
            @"bounds": NSStringFromCGRect(UIScreen.mainScreen.bounds),
            @"scale": @(UIScreen.mainScreen.scale),
            @"maximumFramesPerSecond": @(UIScreen.mainScreen.maximumFramesPerSecond),
        },
        @"accessibility": @{
            @"reduceMotion": @(UIAccessibilityIsReduceMotionEnabled()),
            @"reduceTransparency": @(UIAccessibilityIsReduceTransparencyEnabled()),
            @"darkerSystemColors": @(UIAccessibilityDarkerSystemColorsEnabled()),
        },
    };
}
