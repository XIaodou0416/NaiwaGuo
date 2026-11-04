static BOOL NGPlaybackViewVisible(UIView *view, UIWindow *window) {
    if (!view || !window || view.window != window || view.hidden || view.alpha < 0.01) return NO;
    for (UIView *node = view.superview; node; node = node.superview) if (node.hidden || node.alpha < 0.01) return NO;
    CGRect rect = [view convertRect:view.bounds toView:window];
    CGRect visible = CGRectIntersection(rect, window.bounds);
    if (CGRectIsNull(visible) || CGRectGetWidth(visible) < window.bounds.size.width * 0.75) return NO;
    return CGRectGetHeight(visible) >= MIN(240, window.bounds.size.height * 0.35);
}

static UIView *NGVisiblePlaybackPlayer(void) {
    UIView *best = nil;
    CGFloat bestArea = 0;
    for (UIWindow *window in NGCaptureWindows()) {
        if (window.windowLevel != UIWindowLevelNormal) continue;
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
        for (NSUInteger index = 0; index < queue.count && index < 12000; index++) {
            UIView *view = queue[index];
            NSString *name = NGClassName(view);
            if ([name isEqualToString:@"FQVShortVideoPlayerCellView"] ||
                [name isEqualToString:@"FQVShortVideoLandscapePlayerCellView"]) {
                if (NGPlaybackViewVisible(view, window)) {
                    CGRect rect = [view convertRect:view.bounds toView:window];
                    CGFloat area = CGRectGetWidth(rect) * CGRectGetHeight(rect);
                    if (area > bestArea) { best = view; bestArea = area; }
                }
            }
            [queue addObjectsFromArray:view.subviews];
        }
    }
    return best;
}

static BOOL NGPlaybackVisibleForStatusBar(void) {
    return NGVisiblePlaybackPlayer() != nil;
}

static BOOL NGStatusBarPlaybackVisible;
static BOOL NGShouldHideSystemStatusBar(void) {
    if (![NSUserDefaults.standardUserDefaults boolForKey:NGPreferenceHideStatusBar]) return NO;
    UIViewController *top = NGTopController();
    if ([top isKindOfClass:NGSettingsTableController.class] || [NGClassName(top) hasPrefix:@"NGLivePreview"]) return NO;
    return NGStatusBarPlaybackVisible;
}

static void NGUpdateStatusBarAppearance(void) {
    UIWindow *window = NGBaseWindow();
    NSMutableArray<UIViewController *> *queue = [NSMutableArray array];
    if (window.rootViewController) [queue addObject:window.rootViewController];
    for (NSUInteger index = 0; index < queue.count && index < 200; index++) {
        UIViewController *controller = queue[index];
        if (controller.isViewLoaded) [controller setNeedsStatusBarAppearanceUpdate];
        if (controller.presentedViewController) [queue addObject:controller.presentedViewController];
        [queue addObjectsFromArray:controller.childViewControllers];
    }
}

static void NGUpdatePlaybackStatusBar(void) {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ NGUpdatePlaybackStatusBar(); });
        return;
    }
    NGStatusBarPlaybackVisible = NGPlaybackVisibleForStatusBar();
    NGUpdateStatusBarAppearance();
    // Older host builds delegate status-bar visibility to UIApplication instead.
    if ([NSBundle.mainBundle.infoDictionary[@"UIViewControllerBasedStatusBarAppearance"] isEqual:@NO]) {
        static NSNumber *original;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        if (NGShouldHideSystemStatusBar()) {
            if (!original) original = @(UIApplication.sharedApplication.statusBarHidden);
            [UIApplication.sharedApplication setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
        } else if (original) {
            [UIApplication.sharedApplication setStatusBarHidden:original.boolValue withAnimation:UIStatusBarAnimationFade]; original = nil;
        }
#pragma clang diagnostic pop
    }
}

static void NGInstallStatusBarHookForClass(Class target) {
    if (!target) return;
    SEL selector = @selector(prefersStatusBarHidden);
    Method method = class_getInstanceMethod(target, selector);
    if (!method) return;
    const char *types = method_getTypeEncoding(method);
    IMP original = method_getImplementation(method);
    IMP replacement = imp_implementationWithBlock(^BOOL(UIViewController *controller) {
        if (NGShouldHideSystemStatusBar()) return YES;
        return ((BOOL (*)(id, SEL))original)(controller, selector);
    });
    if (!class_addMethod(target, selector, replacement, types)) class_replaceMethod(target, selector, replacement, types);
}

static void NGInstallPlaybackStatusBarHooks(void) {
    static NSMutableSet<NSString *> *installed;
    if (!installed) installed = [NSMutableSet set];
    for (NSString *name in @[@"UIViewController", @"UINavigationController", @"UITabBarController", @"SSRootViewController", @"SSTabBarController", @"FQVNavigationController", @"FQVModalNavigationController", @"SSVideoSeriesFeedViewController", @"SSVideoFeedContainerViewController", @"FQVShortVideoListViewController", @"FQRJXCategoryListContainerViewController"]) {
        Class target = NSClassFromString(name);
        if (!target || [installed containsObject:name]) continue;
        NGInstallStatusBarHookForClass(target);
        SEL selector = @selector(childViewControllerForStatusBarHidden);
        Method method = class_getInstanceMethod(target, selector);
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^UIViewController *(UIViewController *controller) {
            if (NGShouldHideSystemStatusBar()) return nil;
            return ((id (*)(id, SEL))original)(controller, selector);
        });
        if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
        [installed addObject:name];
    }
}
