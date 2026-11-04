static char NGEpisodeGlassAssociation, NGEpisodeLabelToolbarKey;
@interface NGEpisodeLabelState : NSObject
@property(nonatomic, weak) UIView *toolbar;
@end
@implementation NGEpisodeLabelState @end
static void NGDecorateEpisodeToolbar(UIView *toolbar) {
    NSMutableDictionary<NSString *, UIVisualEffectView *> *surfaces = objc_getAssociatedObject(toolbar, &NGEpisodeGlassAssociation);
    if (!surfaces) { surfaces = [NSMutableDictionary dictionary]; objc_setAssociatedObject(toolbar, &NGEpisodeGlassAssociation, surfaces, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
    for (UIView *glass in surfaces.allValues) glass.hidden = YES;
    if (!NGPlaybackHeaderGlassEnabled() || toolbar.hidden) return;
    CGRect back = CGRectNull;
    NSMutableDictionary<NSString *, NSValue *> *frames = [NSMutableDictionary dictionary];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:toolbar.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 150; i++) {
        UIView *view = queue[i];
        if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo."] || view.hidden || view.alpha < 0.01) continue;
        CGRect frame = [view convertRect:view.bounds toView:toolbar];
        if ([view isKindOfClass:UIControl.class] && CGRectGetMidX(frame) < CGRectGetWidth(toolbar.bounds) * 0.25 && frame.size.width <= 80) back = CGRectUnion(back, frame);
        if ([view isKindOfClass:UILabel.class]) {
            NGEpisodeLabelState *labelState = objc_getAssociatedObject(view, &NGEpisodeLabelToolbarKey);
            if (!labelState) { labelState = [NGEpisodeLabelState new]; objc_setAssociatedObject(view, &NGEpisodeLabelToolbarKey, labelState, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
            labelState.toolbar = toolbar;
            NSString *text = ((UILabel *)view).text;
            if ([text hasPrefix:@"第"] && [text containsString:@"集"] && text.length <= 18 && frame.origin.x < toolbar.bounds.size.width * 0.45) {
                UILabel *label = (id)view;
                CGRect ink = [label textRectForBounds:label.bounds limitedToNumberOfLines:1];
                CGSize fit = [label sizeThatFits:CGSizeMake(toolbar.bounds.size.width * 0.4, 40)];
                ink.size.width = MIN(label.bounds.size.width, MIN(fit.width, toolbar.bounds.size.width * 0.4));
                if (label.textAlignment == NSTextAlignmentRight) ink.origin.x = CGRectGetWidth(label.bounds) - ink.size.width;
                else if (label.textAlignment == NSTextAlignmentCenter) ink.origin.x = (CGRectGetWidth(label.bounds) - ink.size.width) / 2;
                back = CGRectUnion(back, [label convertRect:ink toView:toolbar]);
            }
            if (([text hasSuffix:@"x"] || [text containsString:@"倍速"]) && CGRectGetMidX(frame) > CGRectGetWidth(toolbar.bounds) / 2) {
                UIView *control = view.superview;
                frames[@"Speed"] = [NSValue valueWithCGRect:[control convertRect:control.bounds toView:toolbar]];
            }
        }
        if ([view isKindOfClass:UIButton.class] && CGRectGetMidX(frame) > CGRectGetWidth(toolbar.bounds) * 0.88) frames[@"More"] = [NSValue valueWithCGRect:frame];
        [queue addObjectsFromArray:view.subviews];
    }
    if (!CGRectIsNull(back)) frames[@"BackEpisodes"] = [NSValue valueWithCGRect:back];
    for (NSString *kind in frames) {
        CGRect frame = frames[kind].CGRectValue;
        if (CGRectIsEmpty(frame)) continue;
        frame = CGRectInset(frame, -6, 0);
        frame.origin.y = MAX(0, (CGRectGetHeight(toolbar.bounds) - 40) / 2);
        frame.size.height = MIN(40, CGRectGetHeight(toolbar.bounds));
        UIVisualEffectView *glass = surfaces[kind];
        if (!glass) {
            glass = [[UIVisualEffectView alloc] initWithEffect:nil];
            glass.userInteractionEnabled = NO; glass.accessibilityElementsHidden = YES;
            glass.accessibilityIdentifier = [@"NaiwaGuo.EpisodeGlass." stringByAppendingString:kind];
            glass.frame = frame;
            if (@available(iOS 26.0, *)) { glass.cornerConfiguration = [UICornerConfiguration capsuleConfiguration]; UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear]; effect.interactive = NO; glass.effect = effect; }
            else glass.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial];
            glass.layer.cornerRadius = 20; glass.clipsToBounds = YES;
            surfaces[kind] = glass;
        }
        glass.frame = frame; glass.hidden = NO;
        if (glass.superview != toolbar) [toolbar insertSubview:glass atIndex:0];
    }
}
static void NGDecorateEpisodeController(UIViewController *controller) {
    if (![NGClassName(controller) isEqualToString:@"FQVShortVideoListViewController"] || !controller.isViewLoaded) return;
    for (UIView *view in controller.view.subviews) {
        if ([NGClassName(view) isEqualToString:@"FQVTouchIgoringView"] && CGRectGetWidth(view.bounds) >= CGRectGetWidth(controller.view.bounds) * 0.8 && CGRectGetHeight(view.bounds) >= 36 && CGRectGetHeight(view.bounds) <= 60 && CGRectGetMinY(view.frame) < 140) NGDecorateEpisodeToolbar(view);
    }
}
static void NGInstallEpisodeGlassHooks(void) {
    static NSMutableSet *installed; if (!installed) installed = [NSMutableSet set];
    if (![installed containsObject:@"labels"]) {
        for (NSString *name in @[@"setText:", @"setAttributedText:"]) {
            SEL sel = NSSelectorFromString(name); Method method = class_getInstanceMethod(UILabel.class, sel); IMP original = method_getImplementation(method);
            method_setImplementation(method, imp_implementationWithBlock(^(UILabel *label, id value) {
                ((void (*)(id, SEL, id))original)(label, sel, value);
                UIView *owner = ((NGEpisodeLabelState *)objc_getAssociatedObject(label, &NGEpisodeLabelToolbarKey)).toolbar;
                if (owner && [label isDescendantOfView:owner]) [owner setNeedsLayout];
            }));
        }
        [installed addObject:@"labels"];
    }
    Class toolbar = NSClassFromString(@"FQVTouchIgoringView");
    if (toolbar && ![installed containsObject:@"toolbar"]) {
        SEL sel = @selector(layoutSubviews); Method method = class_getInstanceMethod(toolbar, sel); IMP original = method_getImplementation(method);
        class_replaceMethod(toolbar, sel, imp_implementationWithBlock(^(UIView *view) {
            ((void (*)(id, SEL))original)(view, sel);
            if (objc_getAssociatedObject(view, &NGEpisodeGlassAssociation)) NGDecorateEpisodeToolbar(view);
        }), method_getTypeEncoding(method));
        [installed addObject:@"toolbar"];
    }
    Class target = NSClassFromString(@"FQVShortVideoListViewController");
    if (!target || [installed containsObject:@"controller"]) return;
    for (NSString *name in @[@"viewWillAppear:", @"viewDidAppear:", @"viewDidLayoutSubviews"]) {
        SEL sel = NSSelectorFromString(name); Method method = class_getInstanceMethod(target, sel); IMP original = method_getImplementation(method);
        IMP replacement = [name hasSuffix:@":"] ? imp_implementationWithBlock(^(UIViewController *controller, BOOL animated) {
            ((void (*)(id, SEL, BOOL))original)(controller, sel, animated); NGDecorateEpisodeController(controller);
            __weak UIViewController *weakController = controller;
            dispatch_async(dispatch_get_main_queue(), ^{ NGDecorateEpisodeController(weakController); });
        }) : imp_implementationWithBlock(^(UIViewController *controller) {
            ((void (*)(id, SEL))original)(controller, sel); NGDecorateEpisodeController(controller);
        });
        class_replaceMethod(target, sel, replacement, method_getTypeEncoding(method));
    }
    [installed addObject:@"controller"];
}
