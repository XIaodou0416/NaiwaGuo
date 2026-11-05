// Extend captured mall/mine content into the 84 pt area behind the floating dock.
// Never resize cells, images or video pages; only their full-width containers.
static char NGDockUnderlayStateKey;
static BOOL NGDockUnderlayApplying;
static NSHashTable<UIView *> *NGDockUnderlayViews;
@interface NGDockUnderlayState : NSObject
@property(nonatomic, weak) UIView *root;
@property(nonatomic) CGRect source, applied;
@property(nonatomic) UIEdgeInsets sourceInset, appliedInset, sourceIndicator, appliedIndicator;
@property(nonatomic) BOOL managesInset;
@property(nonatomic) CGFloat sourceAutomaticBottom;
@end
@implementation NGDockUnderlayState @end
static void NGDockUnderlayRestore(UIView *view) {
    NGDockUnderlayState *state = objc_getAssociatedObject(view, &NGDockUnderlayStateKey); if (!state) return;
    BOOL wasApplying = NGDockUnderlayApplying; NGDockUnderlayApplying = YES;
    if (CGRectEqualToRect(view.frame, state.applied)) view.frame = state.source;
    if (state.managesInset && [view isKindOfClass:UIScrollView.class]) {
        UIScrollView *scroll = (id)view;
        if (UIEdgeInsetsEqualToEdgeInsets(scroll.contentInset, state.appliedInset)) scroll.contentInset = state.sourceInset;
        if (UIEdgeInsetsEqualToEdgeInsets(scroll.verticalScrollIndicatorInsets, state.appliedIndicator)) scroll.verticalScrollIndicatorInsets = state.sourceIndicator;
    }
    objc_setAssociatedObject(view, &NGDockUnderlayStateKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [NGDockUnderlayViews removeObject:view]; NGDockUnderlayApplying = wasApplying;
}
static void NGDockUnderlayApply(UIView *view) {
    NGDockUnderlayState *state = objc_getAssociatedObject(view, &NGDockUnderlayStateKey);
    UIView *root = state.root;
    if (!state || NGDockUnderlayApplying) return;
    if (!root || !root.window || root.window.windowLevel != UIWindowLevelNormal ||
        !NGFeatureBool(NGPreferenceDock) ||
        ![view isDescendantOfView:root] || !CGAffineTransformIsIdentity(view.transform)) { NGDockUnderlayRestore(view); return; }
    if (!CGRectEqualToRect(view.frame, state.applied)) state.source = view.frame;
    CGRect originalInRoot = [view.superview convertRect:state.source toView:root];
    CGFloat targetBottom = MIN(CGRectGetMaxY(root.bounds), [root convertPoint:CGPointMake(0, CGRectGetMaxY(root.window.bounds)) fromView:root.window].y);
    CGFloat gap = targetBottom - CGRectGetMaxY(originalInRoot);
    if (!isfinite(gap) || gap < -1 || gap > 130) { NGDockUnderlayRestore(view); return; }
    CGRect frame = state.source; frame.size.height += MAX(0, gap);
    NGDockUnderlayApplying = YES;
    state.applied = frame; if (!CGRectEqualToRect(view.frame, frame)) view.frame = frame;
    if (state.managesInset) {
        UIScrollView *scroll = (id)view;
        if (!UIEdgeInsetsEqualToEdgeInsets(scroll.contentInset, state.appliedInset)) state.sourceInset = scroll.contentInset;
        if (!UIEdgeInsetsEqualToEdgeInsets(scroll.verticalScrollIndicatorInsets, state.appliedIndicator)) state.sourceIndicator = scroll.verticalScrollIndicatorInsets;
        // Keep total clearance stable when UIKit adds a new safe-area inset.
        CGFloat automaticBottom = scroll.adjustedContentInset.bottom - scroll.contentInset.bottom;
        UIEdgeInsets inset = state.sourceInset; inset.bottom += MAX(0, gap + state.sourceAutomaticBottom - automaticBottom); state.appliedInset = inset;
        if (!UIEdgeInsetsEqualToEdgeInsets(scroll.contentInset, inset)) scroll.contentInset = inset;
        inset = state.sourceIndicator; inset.bottom += MAX(0, gap); state.appliedIndicator = inset;
        if (!UIEdgeInsetsEqualToEdgeInsets(scroll.verticalScrollIndicatorInsets, inset)) scroll.verticalScrollIndicatorInsets = inset;
    }
    NGDockUnderlayApplying = NO;
}
static BOOL NGDockUnderlayRoot(UIView *view) {
    if ([NGClassName(view) isEqualToString:@"SSMine737KMPRootView"]) return YES;
    UIViewController *owner = [view.nextResponder isKindOfClass:UIViewController.class] ? (id)view.nextResponder : nil;
    return [NGClassName(owner) isEqualToString:@"FQMSEcomXTabMallViewController"] && owner.viewIfLoaded == view;
}
static void NGDockUnderlayRefresh(UIView *root) {
    if (NGDockUnderlayApplying || !root.window || !NGDockUnderlayRoot(root) || !NSThread.isMainThread) return;
    if (!NGFeatureBool(NGPreferenceDock)) {
        for (UIView *view in NGDockUnderlayViews.allObjects) if (((NGDockUnderlayState *)objc_getAssociatedObject(view, &NGDockUnderlayStateKey)).root == root) NGDockUnderlayRestore(view);
        return;
    }
    NSMutableArray *nodes = [NSMutableArray arrayWithArray:root.subviews];
    for (NSUInteger i = 0; i < nodes.count && i < 220; i++) {
        UIView *view = nodes[i]; NSString *name = NGClassName(view);
        if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo."] || [view isKindOfClass:UICollectionViewCell.class] || [view isKindOfClass:UITableViewCell.class] ||
            [view isKindOfClass:UIImageView.class] || [view isKindOfClass:UIControl.class] || [name containsString:@"NavigationBar"] || [name containsString:@"Comment"]) continue;
        NGDockUnderlayState *state = objc_getAssociatedObject(view, &NGDockUnderlayStateKey);
        CGRect frame = state ? state.source : view.frame;
        CGRect native = [view.superview convertRect:frame toView:root];
        CGFloat gap = CGRectGetMaxY(root.bounds) - CGRectGetMaxY(native);
        BOOL candidate = frame.size.width >= root.bounds.size.width * 0.9 && frame.size.height >= root.bounds.size.height * 0.45 &&
            gap >= -1 && gap <= 130 && CGAffineTransformIsIdentity(view.transform);
        if (candidate && (gap > 0.5 || state)) {
            if (!state) {
                state = [NGDockUnderlayState new]; state.source = frame; state.applied = frame; state.root = root;
                state.managesInset = [view isKindOfClass:UICollectionView.class] || [view isKindOfClass:UITableView.class];
                if (state.managesInset) { UIScrollView *scroll = (id)view; state.sourceInset = state.appliedInset = scroll.contentInset; state.sourceIndicator = state.appliedIndicator = scroll.verticalScrollIndicatorInsets; state.sourceAutomaticBottom = scroll.adjustedContentInset.bottom - scroll.contentInset.bottom; }
                objc_setAssociatedObject(view, &NGDockUnderlayStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if (!NGDockUnderlayViews) NGDockUnderlayViews = [NSHashTable weakObjectsHashTable]; [NGDockUnderlayViews addObject:view];
            }
            NGDockUnderlayApply(view);
        }
        if (![view isKindOfClass:UICollectionView.class] && ![view isKindOfClass:UITableView.class]) [nodes addObjectsFromArray:view.subviews];
    }
}
static void NGUpdateDockUnderlay(void) {
    for (UIView *view in NGDockUnderlayViews.allObjects) NGDockUnderlayApply(view);
    for (UIWindow *window in NGCaptureWindows()) {
        if (window.windowLevel != UIWindowLevelNormal) continue;
        NSMutableArray *nodes = [NSMutableArray arrayWithObject:window];
        for (NSUInteger i = 0; i < nodes.count && i < 1200; i++) {
            UIView *view = nodes[i];
            if (NGDockUnderlayRoot(view)) NGDockUnderlayRefresh(view);
            else if (![view isKindOfClass:UICollectionView.class] && ![view isKindOfClass:UITableView.class] && ![view.accessibilityIdentifier hasPrefix:@"NaiwaGuo."]) [nodes addObjectsFromArray:view.subviews];
        }
    }
}
static void NGInstallDockUnderlayHooks(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        SEL sel = @selector(setFrame:); Method method = class_getInstanceMethod(UIView.class, sel); IMP original = method_getImplementation(method);
        method_setImplementation(method, imp_implementationWithBlock(^(UIView *view, CGRect frame) {
            NGDockUnderlayState *state = NGDockUnderlayApplying ? nil : objc_getAssociatedObject(view, &NGDockUnderlayStateKey);
            if (state && CGRectEqualToRect(frame, state.applied)) return;
            if (state) state.source = frame;
            ((void (*)(id, SEL, CGRect))original)(view, sel, frame);
            if (state) NGDockUnderlayApply(view);
        }));
    });
    static NSMutableSet *installed; if (!installed) installed = [NSMutableSet set];
    for (NSString *name in @[@"FQMSEcomXTabMallViewController", @"SSMine737KMPViewController"]) {
        Class cls = NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        SEL sel = @selector(viewDidLayoutSubviews); Method method = class_getInstanceMethod(cls, sel); IMP original = method_getImplementation(method);
        class_replaceMethod(cls, sel, imp_implementationWithBlock(^(UIViewController *controller) {
            ((void (*)(id, SEL))original)(controller, sel); NGDockUnderlayRefresh(controller.viewIfLoaded);
        }), method_getTypeEncoding(method)); [installed addObject:name];
    }
    for (NSString *name in @[@"IESECNativeMallView", @"IESECMallHomeView", @"SSMine737KMPRootView", @"SSMyUser637TabsView", @"SSMyUser637NestedContainerScrollView"]) {
        Class cls = NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        SEL sel = @selector(layoutSubviews); Method method = class_getInstanceMethod(cls, sel); IMP original = method_getImplementation(method);
        class_replaceMethod(cls, sel, imp_implementationWithBlock(^(UIView *view) {
            ((void (*)(id, SEL))original)(view, sel);
            for (UIView *root = view; root && ![root isKindOfClass:UIWindow.class]; root = root.superview)
                if (NGDockUnderlayRoot(root)) { NGDockUnderlayRefresh(root); break; }
        }), method_getTypeEncoding(method)); [installed addObject:name];
    }
}
