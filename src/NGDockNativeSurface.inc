// Keep the host's full-width tab background suppressed between layout passes.
// Channel transitions repaint UIView/CALayer properties without a tab-bar layout.
static char NGDockNativeSurfaceKey;
static BOOL NGDockNativeSurfaceWriting;
@interface NGDockNativeSurfaceState : NSObject
@property(nonatomic,weak) NGSystemDock *dock;
@property(nonatomic) BOOL root;
@property(nonatomic) float opacity;
@property(nonatomic) BOOL interaction;
@end
@implementation NGDockNativeSurfaceState @end
static void NGDockRegisterNativeSurface(UIView *view,NGSystemDock *dock,BOOL root) {
    NGDockNativeSurfaceState *state=objc_getAssociatedObject(view,&NGDockNativeSurfaceKey);
    if (!state) {
        state=[NGDockNativeSurfaceState new]; state.opacity=view.layer.opacity; state.interaction=view.userInteractionEnabled;
        objc_setAssociatedObject(view,&NGDockNativeSurfaceKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view.layer,&NGDockNativeSurfaceKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    state.dock=dock; state.root=root;
}
static void NGInstallDockNativeSurfaceHooks(void) {
    static dispatch_once_t once;
    dispatch_once(&once,^{
        SEL color=@selector(setBackgroundColor:); Method vm=class_getInstanceMethod(UIView.class,color); IMP vo=method_getImplementation(vm);
        method_setImplementation(vm,imp_implementationWithBlock(^(UIView *view,UIColor *value){
            NGDockNativeSurfaceState *state=objc_getAssociatedObject(view,&NGDockNativeSurfaceKey);
            if (!NGDockNativeSurfaceWriting && state.root && state.dock) {
                state.dock.nativeBackground=value; state.dock.nativeLayerBackground=value;
                NGDockNativeSurfaceWriting=YES; ((void(*)(id,SEL,id))vo)(view,color,UIColor.clearColor); NGDockNativeSurfaceWriting=NO;
            } else ((void(*)(id,SEL,id))vo)(view,color,value);
        }));
        Method lm=class_getInstanceMethod(CALayer.class,color); IMP lo=method_getImplementation(lm);
        method_setImplementation(lm,imp_implementationWithBlock(^(CALayer *layer,CGColorRef value){
            NGDockNativeSurfaceState *state=objc_getAssociatedObject(layer,&NGDockNativeSurfaceKey);
            if (!NGDockNativeSurfaceWriting && state.root && state.dock) {
                state.dock.nativeLayerBackground=value ? [UIColor colorWithCGColor:value] : nil; value=UIColor.clearColor.CGColor;
            }
            ((void(*)(id,SEL,CGColorRef))lo)(layer,color,value);
        }));
        SEL opacity=@selector(setOpacity:); Method om=class_getInstanceMethod(CALayer.class,opacity); IMP oo=method_getImplementation(om);
        method_setImplementation(om,imp_implementationWithBlock(^(CALayer *layer,float value){
            NGDockNativeSurfaceState *state=objc_getAssociatedObject(layer,&NGDockNativeSurfaceKey);
            if (!NGDockNativeSurfaceWriting && state && !state.root && state.dock) { state.opacity=value; value=0; }
            ((void(*)(id,SEL,float))oo)(layer,opacity,value);
        }));
        SEL interaction=@selector(setUserInteractionEnabled:); Method im=class_getInstanceMethod(UIView.class,interaction); IMP io=method_getImplementation(im);
        method_setImplementation(im,imp_implementationWithBlock(^(UIView *view,BOOL value){
            NGDockNativeSurfaceState *state=objc_getAssociatedObject(view,&NGDockNativeSurfaceKey);
            if (!NGDockNativeSurfaceWriting && state && !state.root && state.dock) { state.interaction=value; value=NO; }
            ((void(*)(id,SEL,BOOL))io)(view,interaction,value);
        }));
    });
}
