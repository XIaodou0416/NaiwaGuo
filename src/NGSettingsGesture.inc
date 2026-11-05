static char NGSettingsGestureKey;
@interface NGSettingsGestureTarget : NSObject <UIGestureRecognizerDelegate>
- (void)held:(UILongPressGestureRecognizer *)gesture;
@end
@implementation NGSettingsGestureTarget
- (void)held:(UILongPressGestureRecognizer *)gesture {
    UIWindow *window=[gesture.view isKindOfClass:UIWindow.class] ? (id)gesture.view : gesture.view.window;
    if (gesture.state==UIGestureRecognizerStateBegan && window) NGOpenSettings();
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other { return YES; }
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    UIWindow *window=[gesture.view isKindOfClass:UIWindow.class] ? (id)gesture.view : gesture.view.window;
    return ![NGDeepestController(window.rootViewController) isKindOfClass:NGSettingsTableController.class];
}
@end

static void NGInstallSettingsGesture(void) {
    static NGSettingsGestureTarget *target;static dispatch_once_t once;
    dispatch_once(&once,^{
        target=[NGSettingsGestureTarget new];
        [NSNotificationCenter.defaultCenter addObserverForName:UIWindowDidBecomeVisibleNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note){NGInstallSettingsGesture();}];
    });
    for(UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if(![scene isKindOfClass:UIWindowScene.class])continue;
        for(UIWindow *window in ((UIWindowScene *)scene).windows) {
            if(window.hidden || window.windowLevel!=UIWindowLevelNormal || !window.rootViewController || [window.accessibilityIdentifier hasPrefix:NGIdentifierPrefix] || objc_getAssociatedObject(window,&NGSettingsGestureKey))continue;
            UILongPressGestureRecognizer *gesture=[[UILongPressGestureRecognizer alloc] initWithTarget:target action:@selector(held:)];
            gesture.numberOfTouchesRequired=2;gesture.minimumPressDuration=.65;gesture.allowableMovement=16;
            gesture.cancelsTouchesInView=NO;gesture.delaysTouchesBegan=NO;gesture.delaysTouchesEnded=NO;gesture.delegate=target;
            gesture.name=@"NaiwaGuo.TwoFingerHold";
            [window addGestureRecognizer:gesture];objc_setAssociatedObject(window,&NGSettingsGestureKey,gesture,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
}
