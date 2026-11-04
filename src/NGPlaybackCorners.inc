// Capture 45BB1284: two noninteractive 13x13 UIImageViews, direct siblings
// of FQVCollectionView, at x=-0.5 / width-12.5 and y=771.5. Only this paired
// lower-edge decoration is hidden; controls, player masks and video stay intact.
static char NGPlaybackCornerStateKey;
static NSHashTable<UIImageView *> *NGPlaybackCorners;
static BOOL NGPlaybackCornerWriting;
@interface NGPlaybackCornerState : NSObject
@property(nonatomic) BOOL sourceHidden;
@property(nonatomic,weak) UIView *owner;
@property(nonatomic,weak) UIView *context;
@end
@implementation NGPlaybackCornerState @end
static BOOL NGPlaybackCornersEnabled(void) { return NGFullscreenEnabled() || NGCollectionGlassEnabled(); }
static void NGRestorePlaybackCorner(UIImageView *view) {
    NGPlaybackCornerState *state=objc_getAssociatedObject(view,&NGPlaybackCornerStateKey); if (!state) return;
    objc_setAssociatedObject(view,&NGPlaybackCornerStateKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    BOOL prior=NGPlaybackCornerWriting; NGPlaybackCornerWriting=YES; view.hidden=state.sourceHidden; NGPlaybackCornerWriting=prior; [NGPlaybackCorners removeObject:view];
}
static void NGDecoratePlaybackCorners(UIView *parent) {
    if (!parent || NGPlaybackCornerWriting || parent.bounds.size.width<200 || parent.bounds.size.height<300) return;
    CGFloat width=parent.bounds.size.width;
    // Collection detail wraps its player/list and decorations in extra containers.
    // Require an actual playback branch, but do not require direct siblings.
    NSMutableArray *nodes=[NSMutableArray arrayWithObject:parent]; BOOL playback=NO;
    for (NSUInteger i=0;i<nodes.count && i<180;i++) {
        UIView *node=nodes[i]; NSString *name=NGClassName(node);
        if ([name isEqualToString:@"FQVCollectionView"] || [name isEqualToString:@"FQVShortVideoPlayerCellView"] || [name isEqualToString:@"FQVTransitionPlayerLimitScaleContainerView"]) { playback=YES; break; }
        if (node!=parent && ([node isKindOfClass:UIControl.class] || [node isKindOfClass:UIImageView.class])) continue;
        [nodes addObjectsFromArray:node.subviews];
    }
    if (!playback) return;
    UIImageView *left=nil,*right=nil;
    NSMutableArray *candidates=[NSMutableArray arrayWithArray:parent.subviews];
    for (NSUInteger i=0;i<candidates.count && i<100;i++) {
        UIView *node=candidates[i];
        if (![node isKindOfClass:UIImageView.class]) {
            if (![node isKindOfClass:UIControl.class] && ![node isKindOfClass:UIScrollView.class] && !node.gestureRecognizers.count && fabs(node.bounds.size.width-width)<3) [candidates addObjectsFromArray:node.subviews];
            continue;
        }
        if (node.userInteractionEnabled || node.gestureRecognizers.count || node.accessibilityLabel.length) continue;
        UIImageView *image=(id)node; CGRect f=[image convertRect:image.bounds toView:parent];
        if (!image.image || f.size.width<8 || f.size.width>24 || fabs(f.size.width-f.size.height)>1 || f.origin.y<parent.bounds.size.height*0.6 || CGRectGetMaxY(f)>parent.bounds.size.height+2) continue;
        if (fabs(CGRectGetMinX(f))<2) left=image;
        if (fabs(CGRectGetMaxX(f)-width)<2) right=image;
    }
    CGRect lf=left ? [left convertRect:left.bounds toView:parent] : CGRectZero,rf=right ? [right convertRect:right.bounds toView:parent] : CGRectZero;
    BOOL pair=left && right && fabs(lf.origin.y-rf.origin.y)<1 && fabs(lf.size.width-rf.size.width)<1;
    for (UIImageView *old in NGPlaybackCorners.allObjects) if (((NGPlaybackCornerState *)objc_getAssociatedObject(old,&NGPlaybackCornerStateKey)).context==parent && (!pair || (old!=left && old!=right) || !NGPlaybackCornersEnabled())) NGRestorePlaybackCorner(old);
    if (!pair || !NGPlaybackCornersEnabled()) return;
    if (!NGPlaybackCorners) NGPlaybackCorners=[NSHashTable weakObjectsHashTable];
    NGPlaybackCornerWriting=YES;
    for (UIImageView *view in @[left,right]) {
        NGPlaybackCornerState *state=objc_getAssociatedObject(view,&NGPlaybackCornerStateKey);
        if (!state) { state=[NGPlaybackCornerState new]; state.sourceHidden=view.hidden; state.owner=view.superview; state.context=parent; objc_setAssociatedObject(view,&NGPlaybackCornerStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        [NGPlaybackCorners addObject:view]; if (!view.hidden) view.hidden=YES;
    }
    NGPlaybackCornerWriting=NO;
}
static void NGDiscoverPlaybackCorner(UIImageView *image) {
    if (NGPlaybackCornerWriting || !NSThread.isMainThread) return;
    if (!image.image || image.bounds.size.width<8 || image.bounds.size.height<8 || image.bounds.size.width>24 || image.bounds.size.height>24) { if (objc_getAssociatedObject(image,&NGPlaybackCornerStateKey)) NGRestorePlaybackCorner(image); return; }
    for (UIView *parent=image.superview;parent && ![parent isKindOfClass:UIWindow.class];parent=parent.superview) {
        if (parent.bounds.size.width>=200 && parent.bounds.size.height>=300) {
            CGRect frame=[image convertRect:image.bounds toView:parent];
            BOOL edge=fabs(CGRectGetMinX(frame))<2 || fabs(CGRectGetMaxX(frame)-parent.bounds.size.width)<2;
            if (edge && frame.origin.y>=parent.bounds.size.height*0.6) { NGDecoratePlaybackCorners(parent); if (objc_getAssociatedObject(image,&NGPlaybackCornerStateKey)) return; }
        }
        if ([parent isKindOfClass:UIControl.class] || [parent isKindOfClass:UITableView.class]) return;
    }
}
static void NGRefreshPlaybackCorners(void) {
    for (UIImageView *view in NGPlaybackCorners.allObjects) {
        NGPlaybackCornerState *state=objc_getAssociatedObject(view,&NGPlaybackCornerStateKey);
        if (!NGPlaybackCornersEnabled() || view.superview!=state.owner) NGRestorePlaybackCorner(view);
        else NGDecoratePlaybackCorners(state.context);
    }
    if (!NGPlaybackCornersEnabled()) return;
    for (UIWindow *window in NGCaptureWindows()) {
        if (window.windowLevel!=UIWindowLevelNormal) continue;
        NSMutableArray *nodes=[NSMutableArray arrayWithObject:window];
        for (NSUInteger i=0;i<nodes.count && i<10000;i++) {
            UIView *node=nodes[i];
            if ([node isKindOfClass:UIImageView.class]) NGDiscoverPlaybackCorner((id)node);
            else if (![node isKindOfClass:UIControl.class]) [nodes addObjectsFromArray:node.subviews];
        }
    }
}
static void NGInstallPlaybackCornerHooks(void) {
    static BOOL installed; if (installed) return; installed=YES;
    SEL hidden=@selector(setHidden:); Method m=class_getInstanceMethod(UIImageView.class,hidden); IMP original=method_getImplementation(m);
    class_replaceMethod(UIImageView.class,hidden,imp_implementationWithBlock(^(UIImageView *view,BOOL value){
        NGPlaybackCornerState *state=objc_getAssociatedObject(view,&NGPlaybackCornerStateKey);
        if (state && !NGPlaybackCornerWriting) { state.sourceHidden=value; if (NGPlaybackCornersEnabled() && view.superview==state.owner) value=YES; }
        ((void(*)(id,SEL,BOOL))original)(view,hidden,value);
    }),method_getTypeEncoding(m));
    SEL moved=@selector(didMoveToSuperview); m=class_getInstanceMethod(UIImageView.class,moved); IMP move=method_getImplementation(m);
    class_replaceMethod(UIImageView.class,moved,imp_implementationWithBlock(^(UIImageView *view){
        ((void(*)(id,SEL))move)(view,moved);
        NGPlaybackCornerState *state=objc_getAssociatedObject(view,&NGPlaybackCornerStateKey);
        if (state && view.superview!=state.owner) NGRestorePlaybackCorner(view);
        NGDiscoverPlaybackCorner(view);
    }),method_getTypeEncoding(m));
    for (NSString *name in @[@"layoutSubviews",@"didMoveToWindow"]) {
        SEL selector=NSSelectorFromString(name); Method method=class_getInstanceMethod(UIImageView.class,selector); IMP before=method_getImplementation(method);
        class_replaceMethod(UIImageView.class,selector,imp_implementationWithBlock(^(UIImageView *view){ ((void(*)(id,SEL))before)(view,selector); NGDiscoverPlaybackCorner(view); }),method_getTypeEncoding(method));
    }
    SEL imageSetter=@selector(setImage:); Method imageMethod=class_getInstanceMethod(UIImageView.class,imageSetter); IMP imageOriginal=method_getImplementation(imageMethod);
    class_replaceMethod(UIImageView.class,imageSetter,imp_implementationWithBlock(^(UIImageView *view,UIImage *image){ ((void(*)(id,SEL,id))imageOriginal)(view,imageSetter,image); NGDiscoverPlaybackCorner(view); }),method_getTypeEncoding(imageMethod));

}
