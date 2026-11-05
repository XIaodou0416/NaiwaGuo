static BOOL NGDanmakuWriting;
static BOOL NGIsDanmakuEntranceName(NSString *name) { return [name isEqualToString:@"SSShortVideoDanmakuLeftAboveView"] || [name isEqualToString:@"FQCShortVideoDanmakuEntranceView"]; }
static void NGDecorateDanmakuEntrance(UIView *view) {
    BOOL previous=NGDanmakuWriting; NGDanmakuWriting=YES;
    NGSetPlaybackHeaderHidden(view,NGPlaybackElementHidden(@"danmakuEntrance"));
    NGDanmakuWriting=previous;
}
static void NGInstallDanmakuHiddenHook(Class target) {
    SEL selector=@selector(setHidden:);Method method=class_getInstanceMethod(target,selector);if (!method) return;IMP original=method_getImplementation(method);
    class_replaceMethod(target,selector,imp_implementationWithBlock(^(UIView *view,BOOL hidden){
        if (!NGDanmakuWriting) {
            NGPlaybackHeaderHiddenState *state=objc_getAssociatedObject(view,&NGPlaybackHeaderHiddenAssociation);
            if (!state) {state=[NGPlaybackHeaderHiddenState new];objc_setAssociatedObject(view,&NGPlaybackHeaderHiddenAssociation,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
            state.sourceHidden=hidden; hidden=hidden || NGPlaybackElementHidden(@"danmakuEntrance");state.appliedHidden=hidden;
        }
        ((void(*)(id,SEL,BOOL))original)(view,selector,hidden);
    }),method_getTypeEncoding(method));
}
