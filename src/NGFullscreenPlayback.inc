// Keep the feed's native page stride, offsets and layout attributes intact.
// Project that viewport onto the full-height canvas. Counter-scale native HUD
// roots, and uniformly aspect-fill only the captured TTPlayerView render branch.
// The same projection applies to every page during a swipe; there is no active-
// cell selection, screenshot backdrop, second player, timer or scroll polling.
typedef NS_ENUM(NSUInteger, NGFullscreenRole) {
    NGFullscreenViewport, NGFullscreenRender, NGFullscreenHUD
};
@interface NGFullscreenState : NSObject
@property(nonatomic) NGFullscreenRole role;
@property(nonatomic, weak) UICollectionView *collection;
@property(nonatomic, weak) UIView *player;
@property(nonatomic) CGAffineTransform source;
@property(nonatomic) CGAffineTransform applied;
@end
@implementation NGFullscreenState @end
static char NGFullscreenStateKey;
static NSHashTable<UIView *> *NGFullscreenManaged;
static BOOL NGFullscreenApplying;
static BOOL NGFullscreenDiscovering;

static BOOL NGFullscreenEnabled(void) {
    return [NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackFullscreen];
}
static BOOL NGFullscreenPlayer(UIView *view) {
    return [NGClassName(view) isEqualToString:@"FQVShortVideoPlayerCellView"];
}
static UICollectionView *NGFullscreenFeed(UIView *view) {
    for (NSUInteger depth = 0; view && depth < 12; depth++, view = view.superview)
        if ([NGClassName(view) isEqualToString:@"FQVCollectionView"] && [view isKindOfClass:UICollectionView.class]) return (id)view;
    return nil;
}
static CGRect NGFullscreenNativeFrame(UIView *view) {
    return CGRectMake(view.center.x - view.bounds.size.width * view.layer.anchorPoint.x,
                      view.center.y - view.bounds.size.height * view.layer.anchorPoint.y,
                      view.bounds.size.width, view.bounds.size.height);
}
static CGFloat NGFullscreenFeedScale(UICollectionView *feed) {
    UIWindow *window = feed.window;
    if (!NGFullscreenEnabled() || !feed.superview || !window || window.windowLevel != UIWindowLevelNormal ||
        window.bounds.size.width >= window.bounds.size.height) return 0;
    NGFullscreenState *state = objc_getAssociatedObject(feed, &NGFullscreenStateKey);
    if (!CGAffineTransformIsIdentity(state ? state.source : feed.transform)) return 0;
    CGRect native = NGFullscreenNativeFrame(feed);
    CGRect parent = [feed.superview convertRect:feed.superview.bounds toView:window];
    CGFloat height = MIN(CGRectGetMaxY(parent), CGRectGetMaxY(window.bounds)) - CGRectGetMinY(parent) - native.origin.y;
    CGFloat original = feed.bounds.size.height;
    if (!isfinite(height) || original < window.bounds.size.height * 0.6 ||
        feed.bounds.size.width < window.bounds.size.width * 0.88 ||
        fabs(CGRectGetWidth(parent) - feed.bounds.size.width) > 3 ||
        fabs(CGRectGetMinX(parent)) > 3 || native.origin.y < -1 || native.origin.y > 120 ||
        height < original - 0.5 || height > original + 180) return 0;
    return MAX(1, height / original);
}
static char NGFullscreenGradientKey;
static BOOL NGFullscreenGradientWriting;
static NSHashTable<UIView *> *NGFullscreenGradients;
static void NGFullscreenBottomGradient(UIView *view) {
    if (![NGClassName(view) isEqualToString:@"FQVShortVideoGradientLayerView"]) return;
    UICollectionView *feed = NGFullscreenFeed(view);
    BOOL bottom = view.superview && view.frame.origin.y >= view.superview.bounds.size.height * 0.5;
    BOOL enabled = bottom && feed && NGFullscreenFeedScale(feed) >= 1;
    NSNumber *saved = objc_getAssociatedObject(view, &NGFullscreenGradientKey);
    if (enabled && !saved) {
        saved = @(view.alpha); objc_setAssociatedObject(view, &NGFullscreenGradientKey, saved, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!NGFullscreenGradients) NGFullscreenGradients = [NSHashTable weakObjectsHashTable]; [NGFullscreenGradients addObject:view];
    }
    if (!saved) return;
    NGFullscreenGradientWriting = YES;
    view.alpha = enabled ? 0 : saved.doubleValue;
    NGFullscreenGradientWriting = NO;
    if (!enabled) { objc_setAssociatedObject(view, &NGFullscreenGradientKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); [NGFullscreenGradients removeObject:view]; }
}
#include "NGPlaybackCorners.inc"

static void NGFullscreenRestore(UIView *view) {
    NGFullscreenState *state = objc_getAssociatedObject(view, &NGFullscreenStateKey);
    if (!state) return;
    if (state.role == NGFullscreenViewport) {
        for (UIView *child in NGFullscreenManaged.allObjects) {
            NGFullscreenState *childState = objc_getAssociatedObject(child, &NGFullscreenStateKey);
            if (child != view && childState.collection == view) NGFullscreenRestore(child);
        }
    }
    BOOL wasApplying = NGFullscreenApplying; NGFullscreenApplying = YES;
    if (CGAffineTransformEqualToTransform(view.transform, state.applied)) view.transform = state.source;
    objc_setAssociatedObject(view, &NGFullscreenStateKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [NGFullscreenManaged removeObject:view];
    NGFullscreenApplying = wasApplying;
}
static void NGFullscreenApplyView(UIView *view) {
    NGFullscreenState *state = objc_getAssociatedObject(view, &NGFullscreenStateKey);
    if (!state || NGFullscreenApplying) return;
    UICollectionView *feed = state.collection;
    if (!NGFullscreenEnabled() || !feed || !view.window || view.window != feed.window ||
        (view != feed && ![view isDescendantOfView:feed]) ||
        !CGAffineTransformIsIdentity(state.source)) { NGFullscreenRestore(view); return; }
    CGFloat scale = NGFullscreenFeedScale(feed);
    if (scale < 1) { NGFullscreenRestore(view); return; }
    CGRect original = NGFullscreenNativeFrame(view);
    CGAffineTransform transform = CGAffineTransformIdentity;
    if (state.role == NGFullscreenViewport) {
        transform.d = scale;
        transform.ty = view.bounds.size.height * view.layer.anchorPoint.y * (scale - 1);
    } else if (state.role == NGFullscreenHUD) {
        // HUD is a direct child of the unresized player; keep its native location
        // and touch path even while the feed's visual page height grows.
        transform.d = 1 / scale;
        transform.ty = view.center.y * (1 / scale - 1);
    } else {
        UIView *player = state.player;
        if (!player || ![view isDescendantOfView:player] || original.size.width < 1 || original.size.height < 1) {
            NGFullscreenRestore(view); return;
        }
        CGRect target = [player convertRect:player.bounds toView:view.superview];
        CGFloat zoom = MAX(target.size.width / original.size.width, target.size.height * scale / original.size.height);
        transform.a = zoom; transform.d = zoom / scale;
        transform.tx = CGRectGetMidX(target) - view.center.x;
        transform.ty = CGRectGetMidY(target) - view.center.y;
    }
    NGFullscreenApplying = YES;
    state.applied = transform;
    if (!CGAffineTransformEqualToTransform(view.transform, transform)) view.transform = transform;
    NGFullscreenApplying = NO;
}
static void NGFullscreenManage(UIView *view, UICollectionView *feed, UIView *player, NGFullscreenRole role) {
    NGFullscreenState *state = objc_getAssociatedObject(view, &NGFullscreenStateKey);
    if (!state) {
        if (!CGAffineTransformIsIdentity(view.transform)) return;
        state = [NGFullscreenState new]; state.source = view.transform; state.applied = view.transform;
        objc_setAssociatedObject(view, &NGFullscreenStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!NGFullscreenManaged) NGFullscreenManaged = [NSHashTable weakObjectsHashTable];
        [NGFullscreenManaged addObject:view];
    }
    state.role = role; state.collection = feed; state.player = player;
    NGFullscreenApplyView(view);
}
static void NGFullscreenRefreshFeed(UICollectionView *feed) {
    NGDecoratePlaybackCorners(feed.superview);
    if (NGFullscreenApplying || NGFullscreenDiscovering || !NSThread.isMainThread || !feed) return;
    if (!NGFullscreenEnabled() || NGFullscreenFeedScale(feed) < 1) {
        for (UIView *view in NGFullscreenManaged.allObjects) {
            NGFullscreenState *state = objc_getAssociatedObject(view, &NGFullscreenStateKey);
            if (state.collection == feed) NGFullscreenRestore(view);
        }
        return;
    }
    NGFullscreenDiscovering = YES;
    NSMutableSet<UIView *> *active = [NSMutableSet set];
    // Restrict discovery to realized feed cells. No full-window work in layout.
    NSMutableArray<UIView *> *nodes = [NSMutableArray arrayWithArray:feed.subviews];
    for (NSUInteger index = 0; index < nodes.count && index < 1200; index++) {
        UIView *player = nodes[index];
        if (!NGFullscreenPlayer(player)) { [nodes addObjectsFromArray:player.subviews]; continue; }
        if (fabs(player.bounds.size.height - feed.bounds.size.height) > 2 ||
            fabs(player.bounds.size.width - feed.bounds.size.width) > 2 ||
            !CGAffineTransformIsIdentity(player.transform)) continue;
        NSMutableArray<UIView *> *mediaNodes = [NSMutableArray arrayWithArray:player.subviews];
        NSMutableSet<UIView *> *branches = [NSMutableSet set];
        NSMutableArray<UIView *> *renders = [NSMutableArray array];
        for (NSUInteger j = 0; j < mediaNodes.count && j < 120; j++) {
            UIView *render = mediaNodes[j];
            NGFullscreenBottomGradient(render);
            if ([NGClassName(render) isEqualToString:@"TTPlayerView"]) {
                UIView *branch = render;
                while (branch.superview && branch.superview != player) branch = branch.superview;
                [branches addObject:branch]; [renders addObject:render];
            } else [mediaNodes addObjectsFromArray:render.subviews];
        }
        if (!renders.count) continue;
        [active addObject:feed];
        NGFullscreenManage(feed, feed, nil, NGFullscreenViewport);
        for (UIView *render in renders) {
            [active addObject:render]; NGFullscreenManage(render, feed, player, NGFullscreenRender);
        }
        for (UIView *hud in player.subviews) if (![branches containsObject:hud]) {
            [active addObject:hud]; NGFullscreenManage(hud, feed, player, NGFullscreenHUD);
        }
    }
    for (UIView *view in NGFullscreenManaged.allObjects) {
        NGFullscreenState *state = objc_getAssociatedObject(view, &NGFullscreenStateKey);
        if (state.collection == feed && ![active containsObject:view]) NGFullscreenRestore(view);
    }
    NGFullscreenDiscovering = NO;
}
static void NGUpdatePlaybackFullscreen(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ NGUpdatePlaybackFullscreen(); }); return; }
    NGRefreshPlaybackCorners();
    for (UIView *gradient in NGFullscreenGradients.allObjects) NGFullscreenBottomGradient(gradient);
    if (!NGFullscreenEnabled()) {
        // Restore child transforms before the viewport projection.
        NSArray *managed = NGFullscreenManaged.allObjects;
        for (UIView *view in managed) if (((NGFullscreenState *)objc_getAssociatedObject(view, &NGFullscreenStateKey)).role != NGFullscreenViewport) NGFullscreenRestore(view);
        for (UIView *view in managed) NGFullscreenRestore(view);
        return;
    }
    for (UIWindow *window in NGCaptureWindows()) {
        if (window.windowLevel != UIWindowLevelNormal) continue;
        NSMutableArray *nodes = [NSMutableArray arrayWithObject:window];
        for (NSUInteger i = 0; i < nodes.count && i < 12000; i++) {
            UIView *view = nodes[i];
            if ([NGClassName(view) isEqualToString:@"FQVCollectionView"] && [view isKindOfClass:UICollectionView.class]) NGFullscreenRefreshFeed((id)view);
            else [nodes addObjectsFromArray:view.subviews];
        }
    }
}
// Remove our projection only inside a host geometry write. UIView then receives
// its original coordinate space, avoiding setFrame shrinking transformed bounds.
static NGFullscreenState *NGFullscreenBeginHostWrite(UIView *view) {
    if (NGFullscreenApplying || !NSThread.isMainThread) return nil;
    NGFullscreenState *state = objc_getAssociatedObject(view, &NGFullscreenStateKey);
    if (!state) return nil;
    NGFullscreenApplying = YES; view.transform = state.source; return state;
}
static void NGFullscreenEndHostWrite(UIView *view, NGFullscreenState *state) {
    if (!state) return;
    state.source = view.transform; NGFullscreenApplying = NO;
    NGFullscreenApplyView(view);
}
static void NGInstallPlaybackFullscreenHooks(void) {
    NGInstallPlaybackCornerHooks();
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        for (NSString *name in @[@"setFrame:", @"setBounds:"]) {
            SEL sel = NSSelectorFromString(name); Method method = class_getInstanceMethod(UIView.class, sel);
            IMP original = method_getImplementation(method);
            method_setImplementation(method, imp_implementationWithBlock(^(UIView *view, CGRect rect) {
                // Reassigning the current presentation frame is a no-op. Feeding
                // it back into the unprojected view would inflate native bounds.
                if (!NGFullscreenApplying && sel == @selector(setFrame:) &&
                    objc_getAssociatedObject(view, &NGFullscreenStateKey) && CGRectEqualToRect(rect, view.frame)) return;
                NGFullscreenState *state = NGFullscreenBeginHostWrite(view);
                ((void (*)(id, SEL, CGRect))original)(view, sel, rect);
                NGFullscreenEndHostWrite(view, state);
            }));
        }
        SEL center = @selector(setCenter:); Method cm = class_getInstanceMethod(UIView.class, center); IMP co = method_getImplementation(cm);
        method_setImplementation(cm, imp_implementationWithBlock(^(UIView *view, CGPoint point) {
            NGFullscreenState *state = NGFullscreenBeginHostWrite(view);
            ((void (*)(id, SEL, CGPoint))co)(view, center, point); NGFullscreenEndHostWrite(view, state);
        }));
        SEL transform = @selector(setTransform:); Method tm = class_getInstanceMethod(UIView.class, transform); IMP to = method_getImplementation(tm);
        method_setImplementation(tm, imp_implementationWithBlock(^(UIView *view, CGAffineTransform value) {
            NGFullscreenState *state = NGFullscreenBeginHostWrite(view);
            ((void (*)(id, SEL, CGAffineTransform))to)(view, transform, value); NGFullscreenEndHostWrite(view, state);
        }));
    });
    static NSMutableSet *installed; if (!installed) installed = [NSMutableSet set];
    Class gradientClass = NSClassFromString(@"FQVShortVideoGradientLayerView");
    if (gradientClass && ![installed containsObject:@"gradient"]) {
        for (NSString *name in @[@"layoutSubviews", @"didMoveToSuperview", @"didMoveToWindow"]) {
            SEL sel = NSSelectorFromString(name); Method method = class_getInstanceMethod(gradientClass, sel); IMP original = method_getImplementation(method);
            class_replaceMethod(gradientClass, sel, imp_implementationWithBlock(^(UIView *view) {
                ((void (*)(id, SEL))original)(view, sel); NGFullscreenBottomGradient(view);
            }), method_getTypeEncoding(method));
        }
        SEL sel = @selector(setAlpha:); Method method = class_getInstanceMethod(gradientClass, sel); IMP original = method_getImplementation(method);
        class_replaceMethod(gradientClass, sel, imp_implementationWithBlock(^(UIView *view, CGFloat alpha) {
            if (!NGFullscreenGradientWriting && objc_getAssociatedObject(view, &NGFullscreenGradientKey)) {
                objc_setAssociatedObject(view, &NGFullscreenGradientKey, @(alpha), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                UICollectionView *feed = NGFullscreenFeed(view); if (feed && NGFullscreenFeedScale(feed) >= 1) alpha = 0;
            }
            ((void (*)(id, SEL, CGFloat))original)(view, sel, alpha);
        }), method_getTypeEncoding(method));
        [installed addObject:@"gradient"];
    }

    for (NSString *name in @[@"FQVCollectionView", @"FQVShortVideoPlayerCellView", @"TTPlayerView"]) {
        Class cls = NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        for (NSString *methodName in @[@"layoutSubviews", @"didMoveToWindow", @"didMoveToSuperview"]) {
            SEL sel = NSSelectorFromString(methodName); Method method = class_getInstanceMethod(cls, sel);
            if (!method) continue;
            IMP original = method_getImplementation(method);
            IMP replacement = imp_implementationWithBlock(^(UIView *view) {
                NGFullscreenState *state = [methodName isEqualToString:@"layoutSubviews"] ? NGFullscreenBeginHostWrite(view) : nil;
                ((void (*)(id, SEL))original)(view, sel);
                NGFullscreenEndHostWrite(view, state);
                if (NGFullscreenApplying || NGFullscreenDiscovering) return;
                UICollectionView *feed = NGFullscreenFeed(view);
                if (feed) NGFullscreenRefreshFeed(feed); else NGFullscreenRestore(view);
            });
            if (!class_addMethod(cls, sel, replacement, method_getTypeEncoding(method))) class_replaceMethod(cls, sel, replacement, method_getTypeEncoding(method));
        }
        [installed addObject:name];
    }
}
