static NSString *const NGPreferenceShareGlassClear = @"com.naiwaguo.playback.shareGlassClear";
static char NGPanelMaterialStateAssociation;
static BOOL NGPanelShareKind(NSString *kind) { return [kind isEqualToString:@"Share"] || [kind isEqualToString:@"More"] || [kind isEqualToString:@"Speed"]; }
static char NGCommentGlassAssociation, NGPanelSurfaceAssociation;
static char NGPanelBackgroundAssociation, NGPanelEffectAssociation, NGPanelTraitAssociation, NGPanelTextAssociation;
static char NGPanelCoverAssociation, NGPanelLayerStateKey;
static BOOL NGPanelViewBackgroundWriting;
@interface NGPanelCoverState : NSObject
@property(nonatomic, weak) UIView *surface;
@property(nonatomic) BOOL share;
@end
@implementation NGPanelCoverState @end
static NSHashTable<UIView *> *NGPanelSurfaces;
static BOOL NGPanelUpdating;
static void NGPanelRepairSurface(UIView *root);
@interface NGPanelGlassView : UIVisualEffectView
@property(nonatomic,weak) UIView *panelOwner;
@end
@implementation NGPanelGlassView
- (void)didMoveToSuperview {
    [super didMoveToSuperview];
    if (self.superview || NGPanelUpdating || !self.panelOwner) return;
    __weak UIView *owner=self.panelOwner; __weak NGPanelGlassView *weakSelf=self;
    dispatch_async(dispatch_get_main_queue(),^{
        UIView *root=owner; NGPanelGlassView *glass=weakSelf;
        if (root.window && glass && !glass.superview && objc_getAssociatedObject(root,&NGCommentGlassAssociation)==glass) NGPanelRepairSurface(root);
    });
}
@end
static BOOL NGCommentGlassEnabled(void) { return NGFeatureBool(NGPreferenceCommentGlass); }
static BOOL NGSharePanelController(id controller) {
    NSString *name = NGClassName(controller);
    return [name containsString:@"SharePanel"] || [name containsString:@"ShareViewController"] ||
           [name containsString:@"UGKmpSharePanelContainer"] || [name isEqualToString:@"FQVShortVideoSettingsViewController"] || [name isEqualToString:@"FQVShortVideoNewSpeedSelectController"];
}
static BOOL NGPanelVisible(UIView *view) {
    if (!view || CGRectIsEmpty(view.bounds)) return NO;
    for (UIView *node = view; node; node = node.superview) if (node.hidden || node.alpha < 0.01) return NO;
    return view.window && !CGRectIsEmpty(CGRectIntersection([view convertRect:view.bounds toView:view.window], view.window.bounds));
}
static BOOL NGCommentContainsClass(UIView *root, NSString *fragment) {
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 3000; i++) { UIView *view = queue[i]; if ([NGClassName(view) containsString:fragment]) return YES; if (![view.accessibilityIdentifier hasPrefix:@"NaiwaGuo."]) [queue addObjectsFromArray:view.subviews]; }
    return NO;
}
static UIView *NGCommentPanelRoot(UIView *source) {
    for (UIView *node = source; node; node = node.superview) {
        UIViewController *controller = [node.nextResponder isKindOfClass:UIViewController.class] ? (UIViewController *)node.nextResponder : nil;
        if ([NGClassName(controller) containsString:@"ShortVideoCommentPanelViewController"]) return controller.view;
        if ([node isKindOfClass:UIWindow.class]) break;
    }
    UIView *fallback = nil;
    for (UIView *node = source; node && ![node isKindOfClass:UIWindow.class]; node = node.superview) {
        if ([node isKindOfClass:UICollectionView.class]) continue;
        if (NGCommentContainsClass(node, @"ShortVideoCommentList") && (NGCommentContainsClass(node, @"ShortVideoCommentPanelTitle") || NGCommentContainsClass(node, @"ShortVideoCommentTitleView"))) {
            if (!fallback) fallback = node;
            if (NGCommentContainsClass(node, @"ShortVideoBottomToolBar")) return node;
        }
    }
    return fallback;
}
@interface NGPanelLayerState : NSObject
@property(nonatomic,weak) UIView *owner;
@property(nonatomic,weak) UIView *surface;
@property(nonatomic,strong) id sourceBackground;
@property(nonatomic,strong) id sourceContents;
@property(nonatomic) BOOL managesContents;
@end
@implementation NGPanelLayerState @end
static BOOL NGPanelLayerActive(NGPanelLayerState *state) {
    return state && NGFeatureBool(NGPreferenceShareGlass) && objc_getAssociatedObject(state.surface,&NGPanelSurfaceAssociation) && (state.owner==state.surface || [state.owner isDescendantOfView:state.surface]);
}
static BOOL NGPanelFlatBackdrop(id contents) {
    if (!contents || CFGetTypeID((__bridge CFTypeRef)contents)!=CGImageGetTypeID()) return NO;
    CGImageRef image=(__bridge CGImageRef)contents; if (CGImageGetWidth(image)<80 || CGImageGetHeight(image)<80) return NO;
    unsigned char pixels[16*16*4]={0}; CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(pixels,16,16,8,64,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big); CGColorSpaceRelease(space); if (!context) return NO;
    CGContextDrawImage(context,CGRectMake(0,0,16,16),image); CGContextRelease(context);
    NSUInteger opaque=0; CGFloat low=1,high=0;
    for (NSUInteger i=0;i<256;i++) { unsigned char *p=pixels+i*4; if (p[3]<220) continue; opaque++; CGFloat r=(CGFloat)p[0]/p[3],g=(CGFloat)p[1]/p[3],b=(CGFloat)p[2]/p[3]; if (MAX(r,MAX(g,b))-MIN(r,MIN(g,b))>0.06) return NO; low=MIN(low,r); high=MAX(high,r); }
    return opaque>=235 && high-low<0.04;
}
static void NGPanelRestoreLayer(UIView *view) {
    NGPanelLayerState *state=objc_getAssociatedObject(view.layer,&NGPanelLayerStateKey); if (!state) return;
    objc_setAssociatedObject(view.layer,&NGPanelLayerStateKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    view.layer.backgroundColor=state.sourceBackground==NSNull.null ? nil : ((UIColor *)state.sourceBackground).CGColor;
    if (state.managesContents) view.layer.contents=state.sourceContents==NSNull.null ? nil : state.sourceContents;
}
static void NGPanelRestore(UIView *root) {
    if (!root) return;
    BOOL priorUpdating=NGPanelUpdating; NGPanelUpdating=YES;
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 5000; i++) {
        UIView *view = queue[i];
        if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo.Panel."]) { [view removeFromSuperview]; continue; }
        id background = objc_getAssociatedObject(view, &NGPanelBackgroundAssociation);
        objc_setAssociatedObject(view, &NGPanelCoverAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (background) { view.backgroundColor = background == NSNull.null ? nil : background; objc_setAssociatedObject(view, &NGPanelBackgroundAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        id effect = objc_getAssociatedObject(view, &NGPanelEffectAssociation);
        if (effect && [view isKindOfClass:UIVisualEffectView.class]) { ((UIVisualEffectView *)view).effect = effect == NSNull.null ? nil : effect; objc_setAssociatedObject(view, &NGPanelEffectAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        NSNumber *trait = objc_getAssociatedObject(view, &NGPanelTraitAssociation);
        if (trait) { view.overrideUserInterfaceStyle = trait.integerValue; objc_setAssociatedObject(view, &NGPanelTraitAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        NGPanelRestoreLayer(view);
        UIColor *text = objc_getAssociatedObject(view, &NGPanelTextAssociation);
        if (text) { if ([view isKindOfClass:UILabel.class]) ((UILabel *)view).textColor = text; else if ([view isKindOfClass:UITextView.class]) ((UITextView *)view).textColor = text; objc_setAssociatedObject(view, &NGPanelTextAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        [queue addObjectsFromArray:view.subviews];
    }
    objc_setAssociatedObject(root, &NGCommentGlassAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(root, &NGPanelSurfaceAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    NGPanelUpdating=priorUpdating;
}
static BOOL NGPanelNeutralColor(UIColor *color, UITraitCollection *traits) {
    if (!color) return NO; CGFloat r, g, b, a;
    if (![[color resolvedColorWithTraitCollection:traits] getRed:&r green:&g blue:&b alpha:&a]) return NO;
    return a > 0.85 && MAX(r, MAX(g, b)) - MIN(r, MIN(g, b)) < 0.12;
}
static UIVisualEffectView *NGPanelMaterial(UIView *root, NSString *kind, CGRect frame) {
    UIVisualEffectView *glass = objc_getAssociatedObject(root, &NGCommentGlassAssociation);
    if (!glass) {
        glass = [NGPanelGlassView new]; ((NGPanelGlassView *)glass).panelOwner=root; glass.userInteractionEnabled = NO; glass.accessibilityElementsHidden = YES;
        glass.accessibilityIdentifier = [@"NaiwaGuo.Panel." stringByAppendingString:kind];
        glass.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified; glass.clipsToBounds = YES;
        glass.layer.cornerCurve = kCACornerCurveContinuous; glass.layer.cornerRadius = [kind isEqualToString:@"InputField"] ? MIN(18, CGRectGetHeight(frame) / 2) : 24;
        glass.layer.maskedCorners = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner;

        objc_setAssociatedObject(root, &NGCommentGlassAssociation, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    glass.frame = frame; glass.hidden=NO; glass.alpha=1;
    BOOL share = NGPanelShareKind(kind);
    BOOL clear = share && NGFeatureBool(NGPreferenceShareGlassClear);
    NSString *material = [NSString stringWithFormat:@"%d|%ld", clear, (long)root.traitCollection.userInterfaceStyle];
    if (CGRectGetWidth(frame) >= 8 && CGRectGetHeight(frame) >= 8 && (!glass.effect || ![objc_getAssociatedObject(glass, &NGPanelMaterialStateAssociation) isEqual:material])) {
        if (@available(iOS 26.0, *)) {
            UIGlassEffect *effect = [UIGlassEffect effectWithStyle:clear ? UIGlassEffectStyleClear : UIGlassEffectStyleRegular];
            effect.interactive = YES;
            if (clear) effect.tintColor = [UIColor colorWithWhite:root.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark ? 0 : 1 alpha:0.12];
            glass.effect = effect;
            glass.cornerConfiguration = [kind isEqualToString:@"InputField"] ? [UICornerConfiguration capsuleConfiguration] : [UICornerConfiguration configurationWithUniformTopRadius:[UICornerRadius fixedRadius:24] bottomLeftRadius:nil bottomRightRadius:nil];
        } else glass.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
        objc_setAssociatedObject(glass, &NGPanelMaterialStateAssociation, material, OBJC_ASSOCIATION_COPY_NONATOMIC);
    }
    CGFloat opacity = NGFeatureObject(NGPreferenceCommentGlassOpacity) ? NGFeatureFloat(NGPreferenceCommentGlassOpacity) : 0.32;
    glass.contentView.backgroundColor = share ? UIColor.clearColor : [UIColor.secondarySystemBackgroundColor colorWithAlphaComponent:MIN(0.8, MAX(0, opacity))];
    if (glass.superview != root) [root insertSubview:glass atIndex:0];
    objc_setAssociatedObject(root, &NGPanelSurfaceAssociation, kind, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!NGPanelSurfaces) NGPanelSurfaces = [NSHashTable weakObjectsHashTable]; [NGPanelSurfaces addObject:root];
    return glass;
}
static void NGPanelRepairSurface(UIView *root) {
    NSString *kind=objc_getAssociatedObject(root,&NGPanelSurfaceAssociation);
    BOOL enabled=NGPanelShareKind(kind) ? NGFeatureBool(NGPreferenceShareGlass) : NGCommentGlassEnabled();
    if (!kind || !enabled || NGPanelUpdating) return;
    NGPanelUpdating=YES; @try { NGPanelMaterial(root,kind,root.bounds); } @finally { NGPanelUpdating=NO; }
}
static void NGPanelClearBranch(UIView *root, UIView *surface, BOOL share) {
    if (!objc_getAssociatedObject(root, &NGPanelTraitAssociation)) objc_setAssociatedObject(root, &NGPanelTraitAssociation, @(root.overrideUserInterfaceStyle), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    root.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified;
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 5000; i++) {
        UIView *view = queue[i];
        if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo.Panel."]) continue;
        BOOL textView = [view isKindOfClass:UITextView.class];
        CGFloat minWidth = CGRectGetWidth(surface.bounds) * 0.55;
        BOOL broad = view == surface || CGRectGetWidth(view.bounds) >= minWidth;
        if (broad || textView) {
            NGPanelCoverState *state = objc_getAssociatedObject(view, &NGPanelCoverAssociation);
            if (!state) { state = [NGPanelCoverState new]; objc_setAssociatedObject(view, &NGPanelCoverAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
            state.surface = surface; state.share = share;
        }
        if (share && broad && ![view isKindOfClass:UILabel.class] && ![view isKindOfClass:UIImageView.class]) {
            NGPanelLayerState *layerState=objc_getAssociatedObject(view.layer,&NGPanelLayerStateKey);
            if (!layerState) { layerState=[NGPanelLayerState new]; layerState.owner=view; layerState.surface=surface; layerState.sourceBackground=view.layer.backgroundColor ? [UIColor colorWithCGColor:view.layer.backgroundColor] : NSNull.null; objc_setAssociatedObject(view.layer,&NGPanelLayerStateKey,layerState,OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
            UIColor *background=view.layer.backgroundColor ? [UIColor colorWithCGColor:view.layer.backgroundColor] : nil;
            if (NGPanelNeutralColor(background,view.traitCollection)) view.layer.backgroundColor=UIColor.clearColor.CGColor;
            // Only full-size DanceUI background drawing surfaces; never text/icon layers.
            BOOL backing=[NGClassName(view) isEqualToString:@"DanceUI._UIGraphicsView"] && view.bounds.size.width>=surface.bounds.size.width*0.9 && view.bounds.size.height>=surface.bounds.size.height*0.85;
            if (backing && !layerState.managesContents) { layerState.managesContents=YES; layerState.sourceContents=view.layer.contents ?: NSNull.null; }
            if (backing && NGPanelFlatBackdrop(view.layer.contents)) view.layer.contents=nil;
        }
        if ((broad || textView) && NGPanelNeutralColor(view.backgroundColor, view.traitCollection)) {
            if (!objc_getAssociatedObject(view, &NGPanelBackgroundAssociation)) objc_setAssociatedObject(view, &NGPanelBackgroundAssociation, view.backgroundColor ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            view.backgroundColor = UIColor.clearColor;
        }
        if ([view isKindOfClass:UIVisualEffectView.class] && broad) {
            UIVisualEffectView *effect = (UIVisualEffectView *)view;
            if (!objc_getAssociatedObject(view, &NGPanelEffectAssociation)) objc_setAssociatedObject(view, &NGPanelEffectAssociation, effect.effect ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            effect.effect = nil;
        }
        // UIKit controls remain in place; never hide drawing views or replace hit targets.
        [queue addObjectsFromArray:view.subviews];
    }
}
static void NGPanelClearCovers(UIView *root, BOOL share) {
    UIView *surface = root;
    if (share) for (UIResponder *r = root.nextResponder; r; r = r.nextResponder)
        if (NGSharePanelController(r)) { surface = ((UIViewController *)r).view; break; }
    NGPanelClearBranch(root, surface, share);
}
static UIView *NGPanelManagedSurface(UIView *view) {
    for (NSUInteger depth = 0; view && depth < 20; depth++, view = view.superview)
        if (objc_getAssociatedObject(view, &NGPanelSurfaceAssociation)) return view;
    return nil;
}
static void NGPanelRefreshInsertedBranch(UIView *view) {
    if (NGPanelUpdating || !NSThread.isMainThread || [view.accessibilityIdentifier hasPrefix:@"NaiwaGuo."]) return;
    UIView *surface = NGPanelManagedSurface(view); if (!surface) return;
    BOOL share = NGPanelShareKind(objc_getAssociatedObject(surface, &NGPanelSurfaceAssociation));
    if (share ? !NGFeatureBool(NGPreferenceShareGlass) : !NGCommentGlassEnabled()) return;
    NGPanelRepairSurface(surface);
    NGPanelUpdating = YES;
    @try { NGPanelClearBranch(view, surface, share); } @finally { NGPanelUpdating = NO; }
}
static void NGDecorateCommentSource(UIView *source) {
    if (NGPanelUpdating || !source) return; NGPanelUpdating = YES;
    @try {
        UIView *root = NGCommentPanelRoot(source); if (!root) return;
        if (!NGCommentGlassEnabled() || !root.superview || root.hidden) { NGPanelRestore(root); return; }
        NGPanelMaterial(root, @"Comment", root.bounds); NGPanelClearCovers(root, NO);
    } @finally { NGPanelUpdating = NO; }
}
static void NGDecorateCommentEditor(UIView *editor) {
    if (!editor || NGPanelUpdating) return; NGPanelUpdating = YES;
    @try {
        UIView *content = NGObjectProperty(editor, @"contentView");
        if (![content isKindOfClass:UIView.class]) { for (UIView *child in editor.subviews) if (CGRectGetWidth(child.bounds) > CGRectGetWidth(editor.bounds) * 0.8 && child.frame.origin.y > 0) content = child; }
        if (!content) return;
        if (!NGCommentGlassEnabled() || editor.hidden || !editor.superview) { NGPanelRestore(content); return; }
        NGPanelMaterial(content, @"Editor", content.bounds); NGPanelClearCovers(content, NO);
        UIView *field = NGObjectProperty(editor, @"textView");
        if ([field isKindOfClass:UIView.class] && CGRectGetHeight(field.bounds) > 20) { NGPanelMaterial(field, @"InputField", field.bounds); NGPanelClearCovers(field, NO); }
        // The expanded editor covers the bottom of the list: join at its top edge.
        for (UIView *panel in NGPanelSurfaces.allObjects) {
            if (![objc_getAssociatedObject(panel, &NGPanelSurfaceAssociation) isEqual:@"Comment"] || panel.window != content.window || !content.window) continue;
            UIVisualEffectView *glass = objc_getAssociatedObject(panel, &NGCommentGlassAssociation);
            CGRect rect = [content convertRect:content.bounds toView:panel]; CGRect frame = panel.bounds;
            if (NGPanelVisible(editor) && CGRectGetMinY(rect) < CGRectGetMaxY(frame)) frame.size.height = MAX(0, CGRectGetMinY(rect) - CGRectGetMinY(frame));
            glass.frame = frame;
        }
    } @finally { NGPanelUpdating = NO; }
}
static void NGInstallPanelDrawingHooks(void) {
    static NSMutableSet *installed; if (!installed) installed=[NSMutableSet set];
    for (NSString *name in @[@"DanceUI._UIGraphicsView",@"DanceUICompose.ComposeRenderingUIViewImpl"]) {
        Class cls=NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        for (NSString *methodName in @[@"layoutSubviews",@"didMoveToWindow"]) {
            SEL sel=NSSelectorFromString(methodName); Method method=class_getInstanceMethod(cls,sel); IMP original=method_getImplementation(method);
            class_replaceMethod(cls,sel,imp_implementationWithBlock(^(UIView *view){ ((void(*)(id,SEL))original)(view,sel); NGPanelRefreshInsertedBranch(view); }),method_getTypeEncoding(method));
        }
        [installed addObject:name];
    }
}
static void NGDecorateShareController(UIViewController *controller) {
    NGInstallPanelDrawingHooks();
    if (!NGSharePanelController(controller) || !controller.isViewLoaded || NGPanelUpdating) return;
    UIView *root = controller.view;
    if (!NGFeatureBool(NGPreferenceShareGlass)) { NGPanelRestore(root); return; }
    NGPanelUpdating = YES;
    @try { NGPanelMaterial(root, [NGClassName(controller) isEqualToString:@"FQVShortVideoSettingsViewController"] ? @"More" : ([NGClassName(controller) isEqualToString:@"FQVShortVideoNewSpeedSelectController"] ? @"Speed" : @"Share"), root.bounds); NGPanelClearCovers(root, YES); }
    @finally { NGPanelUpdating = NO; }
}
static void NGUpdateCommentGlass(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ NGUpdateCommentGlass(); }); return; }
    for (UIView *root in NGPanelSurfaces.allObjects) {
        NSString *kind = objc_getAssociatedObject(root, &NGPanelSurfaceAssociation);
        if ((!NGPanelShareKind(kind) && !NGCommentGlassEnabled()) || (NGPanelShareKind(kind) && !NGFeatureBool(NGPreferenceShareGlass))) NGPanelRestore(root);
    }
    NSMutableArray *queue = [NSMutableArray array]; for (UIWindow *window in NGCaptureWindows()) if (window.windowLevel == UIWindowLevelNormal) [queue addObject:window];
    NSMutableArray *editors = [NSMutableArray array];
    for (NSUInteger i = 0; i < queue.count && i < 12000; i++) {
        UIView *view = queue[i]; if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo.Panel."]) continue;
        NSString *name = NGClassName(view);
        if ([name containsString:@"ShortVideoCommentPanelTitle"] || [name containsString:@"ShortVideoCommentListCollection"]) NGDecorateCommentSource(view);
        if ([name isEqual:@"FQCShortVideoNewEditorView"]) [editors addObject:view];
        UIViewController *controller = [view.nextResponder isKindOfClass:UIViewController.class] ? (UIViewController *)view.nextResponder : nil;
        if (NGSharePanelController(controller)) NGDecorateShareController(controller);
        [queue addObjectsFromArray:view.subviews];
    }
    for (UIView *editor in editors) NGDecorateCommentEditor(editor);
}
static void NGInstallCommentGlassHooks(void) {
    static NSMutableSet *installed; if (!installed) installed = [NSMutableSet set];
    if (![installed containsObject:@"UIView.panelBackground"]) {
        SEL selector = @selector(setBackgroundColor:);
        Method method = class_getInstanceMethod(UIView.class, selector);
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIView *view, UIColor *color) {
            NGPanelCoverState *state = objc_getAssociatedObject(view, &NGPanelCoverAssociation);
            BOOL enabled = state && !NGPanelUpdating && (state.share ? NGFeatureBool(NGPreferenceShareGlass) : NGCommentGlassEnabled());
            if (enabled && objc_getAssociatedObject(state.surface, &NGPanelSurfaceAssociation) &&
                (view == state.surface || [view isDescendantOfView:state.surface])) {
                objc_setAssociatedObject(view, &NGPanelBackgroundAssociation, color ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if (NGPanelNeutralColor(color, view.traitCollection)) color = UIColor.clearColor;
            }
            NGPanelLayerState *layerState=objc_getAssociatedObject(view.layer,&NGPanelLayerStateKey);
            if (!NGPanelUpdating && NGPanelLayerActive(layerState)) layerState.sourceBackground=objc_getAssociatedObject(view,&NGPanelBackgroundAssociation) ?: color ?: NSNull.null;
            BOOL writing=NGPanelViewBackgroundWriting; NGPanelViewBackgroundWriting=YES;
            ((void (*)(id, SEL, id))original)(view, selector, color);
            NGPanelViewBackgroundWriting=writing;
        });
        class_replaceMethod(UIView.class, selector, replacement, method_getTypeEncoding(method));
        [installed addObject:@"UIView.panelBackground"];
    }
    if (![installed containsObject:@"CALayer.panelRepaint"]) {
        SEL sel=@selector(setBackgroundColor:); Method m=class_getInstanceMethod(CALayer.class,sel); IMP original=method_getImplementation(m);
        method_setImplementation(m,imp_implementationWithBlock(^(CALayer *layer,CGColorRef color){
            NGPanelLayerState *state=objc_getAssociatedObject(layer,&NGPanelLayerStateKey);
            if (state && !NGPanelUpdating && !NGPanelViewBackgroundWriting) {
                state.sourceBackground=color ? [UIColor colorWithCGColor:color] : NSNull.null;
                if (NGPanelLayerActive(state) && NGPanelNeutralColor(color ? [UIColor colorWithCGColor:color] : nil,state.owner.traitCollection)) color=UIColor.clearColor.CGColor;
            }
            ((void(*)(id,SEL,CGColorRef))original)(layer,sel,color);
        }));
        SEL contents=@selector(setContents:); m=class_getInstanceMethod(CALayer.class,contents); IMP setContents=method_getImplementation(m);
        method_setImplementation(m,imp_implementationWithBlock(^(CALayer *layer,id image){
            NGPanelLayerState *state=objc_getAssociatedObject(layer,&NGPanelLayerStateKey);
            if (state.managesContents && !NGPanelUpdating) { state.sourceContents=image ?: NSNull.null; if (NGPanelLayerActive(state) && NGPanelFlatBackdrop(image)) image=nil; }
            ((void(*)(id,SEL,id))setContents)(layer,contents,image);
        }));
        SEL effect=@selector(setEffect:); m=class_getInstanceMethod(UIVisualEffectView.class,effect); IMP setEffect=method_getImplementation(m);
        method_setImplementation(m,imp_implementationWithBlock(^(UIVisualEffectView *view,UIVisualEffect *value){
            NGPanelCoverState *state=objc_getAssociatedObject(view,&NGPanelCoverAssociation);
            if (!NGPanelUpdating && state.share && NGFeatureBool(NGPreferenceShareGlass) && [view isDescendantOfView:state.surface] && objc_getAssociatedObject(view,&NGPanelEffectAssociation)) { objc_setAssociatedObject(view,&NGPanelEffectAssociation,value ?: NSNull.null,OBJC_ASSOCIATION_RETAIN_NONATOMIC); value=nil; }
            ((void(*)(id,SEL,id))setEffect)(view,effect,value);
        }));
        [installed addObject:@"CALayer.panelRepaint"];
    }
    if (![installed containsObject:@"UIView.panelInsertion"]) {
        SEL sel = @selector(didAddSubview:); Method method = class_getInstanceMethod(UIView.class, sel); IMP original = method_getImplementation(method);
        method_setImplementation(method, imp_implementationWithBlock(^(UIView *parent, UIView *child) {
            ((void (*)(id, SEL, id))original)(parent, sel, child);
            NGPanelRefreshInsertedBranch(child);
        }));
        [installed addObject:@"UIView.panelInsertion"];
    }
    // The review tab uses a plain UICollectionView, unlike the comments tab.
    // Repair newly displayed and reused review cells before a scroll is needed.
    for (NSString *name in @[@"FQCShortPlaySeriesCommentCell", @"FQCShortVideoCommentCell", @"FQCShortVideoCommentSectionFooterView"]) {
        Class cls = NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        for (NSString *methodName in @[@"layoutSubviews", @"didMoveToSuperview", @"didMoveToWindow", @"prepareForReuse"]) {
            SEL sel = NSSelectorFromString(methodName); Method method = class_getInstanceMethod(cls, sel); if (!method) continue; IMP original = method_getImplementation(method);
            IMP replacement = imp_implementationWithBlock(^(UIView *view) {
                ((void (*)(id, SEL))original)(view, sel);
                if (NGPanelUpdating) return;
                if (!NGPanelManagedSurface(view)) {
                    NGPanelUpdating = YES; NGPanelRestore(view); NGPanelUpdating = NO;
                    if (view.window) NGDecorateCommentSource(view);
                } else NGPanelRefreshInsertedBranch(view);
            });
            class_replaceMethod(cls, sel, replacement, method_getTypeEncoding(method));
        }
        [installed addObject:name];
    }
    for (NSString *name in @[@"FQCShortVideoCommentPanelTitleView", @"FQCShortVideoCommentTitleView", @"FQCShortVideoCommentListCollectionView", @"FQCJXCategoryListContainerView", @"FQCShortVideoBottomToolBar", @"FQCShortVideoNewEditorView", @"SSEmojiView", @"SSEmoticonBottomView", @"SSGifEmoticonListView", @"FQCWTextView"]) {
        Class target = NSClassFromString(name); if (!target || [installed containsObject:name]) continue;
        SEL selector = @selector(layoutSubviews); Method method = class_getInstanceMethod(target, selector); if (!method) continue;
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIView *view) {
            ((void (*)(id, SEL))original)(view, selector);
            UIView *editor = view; while (editor && ![NGClassName(editor) isEqual:@"FQCShortVideoNewEditorView"]) editor = editor.superview;
            if (editor) NGDecorateCommentEditor(editor); else NGDecorateCommentSource(view);
        });
        if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method)); [installed addObject:name];
    }
    for (NSString *className in @[@"SSUGKmpSharePanelContainerViewController", @"SSVideoSharePanelViewController", @"FQVShortVideoSettingsViewController", @"FQVShortVideoNewSpeedSelectController"]) {
        Class target = NSClassFromString(className);
        if (!target || [installed containsObject:className]) continue;
        for (NSString *name in @[@"viewWillAppear:", @"viewDidAppear:", @"viewDidLayoutSubviews", @"viewDidDisappear:"]) {
            SEL selector = NSSelectorFromString(name); Method method = class_getInstanceMethod(target, selector); if (!method) continue;
            IMP original = method_getImplementation(method);
            IMP replacement = [name hasSuffix:@":"] ? imp_implementationWithBlock(^(UIViewController *controller, BOOL animated) {
                ((void (*)(id, SEL, BOOL))original)(controller, selector, animated);
                NGDecorateShareController(controller); NGUpdateDock();
            }) : imp_implementationWithBlock(^(UIViewController *controller) {
                ((void (*)(id, SEL))original)(controller, selector);
                NGDecorateShareController(controller); NGUpdateDock();
            });
            if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
        }
        [installed addObject:className];
    }
    for (NSString *name in @[@"FQVShortVideoSmartSpeedSelectCell", @"FQVShortVideoSettingsCell", @"FQVShortVideoSettingsThumbnailCell", @"FQVShortVideoSettingsArrowCell", @"FQVShortVideoSettingsArrowWithInfoCell", @"FQVShortVideoSettingsSwitchCell", @"FQVShortVideoSettingsOptionCell", @"SSShortVideoMorePanelShareHeaderView"]) {
        Class target = NSClassFromString(name); if (!target || [installed containsObject:name]) continue;
        SEL selector = @selector(layoutSubviews); Method method = class_getInstanceMethod(target, selector); if (!method) continue;
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIView *view) {
            ((void (*)(id, SEL))original)(view, selector);
            if (NGPanelUpdating || !NGFeatureBool(NGPreferenceShareGlass)) return;
            for (UIResponder *responder = view.nextResponder; responder; responder = responder.nextResponder) {
                if (NGSharePanelController(responder)) { NGPanelUpdating = YES; @try { NGPanelClearCovers(view, YES); } @finally { NGPanelUpdating = NO; } break; }
            }
        });
        if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
        [installed addObject:name];
    }
    NGUpdateCommentGlass();
}

@interface NGShareGlassSettingsController : NGSettingsTableController @end
@implementation NGShareGlassSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title = @"分享、长按与倍速面板"; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 3; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text = @[@"关闭", @"柔雾玻璃", @"清透玻璃"][path.row];
    cell.imageView.image = [UIImage systemImageNamed:@[@"circle.slash", @"drop.halffull", @"drop"][path.row]];
    BOOL enabled = NGFeatureBool(NGPreferenceShareGlass);
    BOOL clear = NGFeatureBool(NGPreferenceShareGlassClear);
    cell.accessoryType = path.row == (enabled ? (clear ? 2 : 1) : 0) ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    [NSUserDefaults.standardUserDefaults setBool:path.row != 0 forKey:NGPreferenceShareGlass];
    [NSUserDefaults.standardUserDefaults setBool:path.row == 2 forKey:NGPreferenceShareGlassClear];
    NGPrepareRuntimeGlass();
    NGUpdateCommentGlass(); [self.tableView reloadData];
    [[UISelectionFeedbackGenerator new] selectionChanged];
}
@end
