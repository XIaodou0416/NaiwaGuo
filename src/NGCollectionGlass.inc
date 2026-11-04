// Captured chain: BaseInsertView > BottomDistributionView > DistributionContentView.
// Keep the host row and actions; its dedicated carrier is hidden as a whole.
static NSString *const NGPreferenceCollectionGlass = @"com.naiwaguo.playback.collectionGlass";
static char NGCollectionSurfaceKey, NGCollectionBackgroundKey, NGCollectionHiddenKey;
static BOOL NGCollectionUpdating;
@interface NGCollectionHiddenState : NSObject
@property(nonatomic) BOOL source;
@end
@implementation NGCollectionHiddenState @end
static UIView *NGCollectionRow(UIView *view) {
    for (NSUInteger depth = 0; view && depth < 7; depth++, view = view.superview)
        if ([NGClassName(view) isEqualToString:@"SSShortVideoBottomDistributionView"]) return view;
    return nil;
}
static UIView *NGCollectionCarrier(UIView *row) {
    UIView *parent = row.superview;
    if (![NGClassName(parent) isEqualToString:@"FQVShortVideoBaseInsertView"]) return row;
    for (UIView *child in parent.subviews) if (child != row && ![child.accessibilityIdentifier hasPrefix:@"NaiwaGuo."]) return row;
    return parent;
}
static BOOL NGCollectionGlassEnabled(void) { return [NSUserDefaults.standardUserDefaults boolForKey:NGPreferenceCollectionGlass]; }
static void NGCollectionSetHidden(UIView *view, BOOL hidden) {
    NGCollectionHiddenState *state = objc_getAssociatedObject(view, &NGCollectionHiddenKey);
    if (hidden) {
        if (!state) { state = [NGCollectionHiddenState new]; state.source = view.hidden; objc_setAssociatedObject(view, &NGCollectionHiddenKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        view.hidden = YES;
    } else if (state) {
        objc_setAssociatedObject(view, &NGCollectionHiddenKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        view.hidden = state.source;
    }
}
static void NGDecorateCollectionRow(UIView *source) {
    if (NGCollectionUpdating || !NSThread.isMainThread) return;
    UIView *row = NGCollectionRow(source); if (!row) return;
    NGCollectionUpdating = YES;
    @try {
        BOOL hidden = NGPlaybackElementHidden(@"text.collection");
        NGCollectionSetHidden(NGCollectionCarrier(row), hidden);
        NGCollectionSetHidden(row, hidden);
        UIVisualEffectView *glass = objc_getAssociatedObject(row, &NGCollectionSurfaceKey);
        BOOL enabled = NGCollectionGlassEnabled();
        if (enabled && row.bounds.size.width > 80 && row.bounds.size.height >= 24) {
            if (!glass) {
                glass = [UIVisualEffectView new]; glass.userInteractionEnabled = NO; glass.accessibilityElementsHidden = YES;
                glass.accessibilityIdentifier = @"NaiwaGuo.Collection.Glass";
                glass.frame = CGRectInset(row.bounds, 8, 2);
                if (@available(iOS 26.0, *)) {
                    UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear]; effect.interactive = NO; glass.effect = effect;
                    glass.cornerConfiguration = [UICornerConfiguration capsuleConfiguration];
                } else glass.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial];
                glass.layer.cornerRadius = glass.bounds.size.height / 2; glass.clipsToBounds = YES;
                objc_setAssociatedObject(row, &NGCollectionSurfaceKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            glass.frame = CGRectInset(row.bounds, 8, 2);
            if (glass.superview != row) [row insertSubview:glass atIndex:0]; else [row sendSubviewToBack:glass];
        } else { [glass removeFromSuperview]; objc_setAssociatedObject(row, &NGCollectionSurfaceKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        NSMutableArray *nodes = [NSMutableArray arrayWithObject:row];
        for (NSUInteger i = 0; i < nodes.count && i < 100; i++) {
            UIView *view = nodes[i]; if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo."]) continue;
            id background = objc_getAssociatedObject(view, &NGCollectionBackgroundKey);
            if (enabled && view.bounds.size.width >= row.bounds.size.width * 0.6 && ![view isKindOfClass:UILabel.class] && ![view isKindOfClass:UIImageView.class]) {
                if (!background) objc_setAssociatedObject(view, &NGCollectionBackgroundKey, view.backgroundColor ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                view.backgroundColor = UIColor.clearColor;
            } else if (background) {
                objc_setAssociatedObject(view, &NGCollectionBackgroundKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                view.backgroundColor = background == NSNull.null ? nil : background;
            }
            [nodes addObjectsFromArray:view.subviews];
        }
    } @finally { NGCollectionUpdating = NO; }
}
static void NGInstallCollectionHooks(void) {
    static NSMutableSet *installed; if (!installed) installed = [NSMutableSet set];
    if (![installed containsObject:@"background"]) {
        SEL sel = @selector(setBackgroundColor:); Method method = class_getInstanceMethod(UIView.class, sel); IMP original = method_getImplementation(method);
        method_setImplementation(method, imp_implementationWithBlock(^(UIView *view, UIColor *color) {
            if (!NGCollectionUpdating && objc_getAssociatedObject(view, &NGCollectionBackgroundKey)) {
                objc_setAssociatedObject(view, &NGCollectionBackgroundKey, color ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if (NGCollectionGlassEnabled() && NGCollectionRow(view)) color = UIColor.clearColor;
            }
            ((void (*)(id, SEL, id))original)(view, sel, color);
        }));
        [installed addObject:@"background"];
    }
    for (NSString *name in @[@"SSShortVideoBottomDistributionView", @"SSShortVideoDistributionContentView", @"FQVShortVideoBaseInsertView"]) {
        Class cls = NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        for (NSString *methodName in @[@"layoutSubviews", @"didMoveToSuperview"]) {
            SEL sel = NSSelectorFromString(methodName); Method method = class_getInstanceMethod(cls, sel); IMP original = method_getImplementation(method);
            IMP replacement = imp_implementationWithBlock(^(UIView *view) {
                ((void (*)(id, SEL))original)(view, sel);
                if ([name isEqualToString:@"FQVShortVideoBaseInsertView"]) { for (UIView *child in view.subviews) if (NGCollectionRow(child) == child) NGDecorateCollectionRow(child); }
                else NGDecorateCollectionRow(view);
            });
            class_replaceMethod(cls, sel, replacement, method_getTypeEncoding(method));
        }
        SEL sel = @selector(setHidden:); Method method = class_getInstanceMethod(cls, sel); IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIView *view, BOOL hidden) {
            NGCollectionHiddenState *state = objc_getAssociatedObject(view, &NGCollectionHiddenKey);
            if (!NGCollectionUpdating && state) { state.source = hidden; if (NGPlaybackElementHidden(@"text.collection")) hidden = YES; }
            ((void (*)(id, SEL, BOOL))original)(view, sel, hidden);
        });
        class_replaceMethod(cls, sel, replacement, method_getTypeEncoding(method));
        [installed addObject:name];
    }
}
