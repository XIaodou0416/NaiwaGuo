static NSArray<NSString *> *NGDockActions(void) { return @[@"search",@"home",@"theater",@"shop",@"mine",@"settings"]; }
static NSArray<NSString *> *NGDockActionTitles(void) { return @[@"搜索短剧",@"首页",@"剧场",@"商城",@"我的",@"插件设置"]; }
static NSArray<NSString *> *NGDockActionSymbols(void) { return @[@"magnifyingglass",@"house",@"play.square",@"storefront",@"person.crop.circle",@"slider.horizontal.3"]; }
static NSString *NGDockAction(void) {
    NSString *action=NGFeatureString(NGPreferenceDockAction);
    return [NGDockActions() containsObject:action] ? action : @"search";
}
static BOOL NGDockAvatarEnabled(void) { return NGFeatureBool(NGPreferenceDockAvatar); }
static __weak UIImageView *NGDockAvatarSource;
static UIImage *NGDockAvatarPhoto,*NGDockAvatarCircle;
static CFTimeInterval NGDockAvatarLastScan;
static BOOL NGDockAvatarRefreshQueued;
static void NGDockQueueAvatarRefresh(void) {
    if (!NGDockAvatarEnabled() || NGDockAvatarRefreshQueued) return;
    NGDockAvatarRefreshQueued=YES;
    dispatch_async(dispatch_get_main_queue(),^{ NGDockAvatarRefreshQueued=NO; NGUpdateDock(); });
}
static UIView *NGDockOwnAvatarCell(UIView *view) {
    for (NSUInteger i=0;view && i<10;i++,view=view.superview)
        if ([NGClassName(view) isEqual:@"SSVideoSeriesFeedSideBarUserInfoCell"]) return view;
    return nil;
}
static void NGDockObserveOwnAvatar(UIView *cell) {
    // These two avatarView properties are verified in the Hongguo 7.4.3 dump.
    // Only the signed-in account cell is eligible, never a video's author avatar.
    id avatar=NGObjectProperty(cell,@"avatarView");
    id imageView=NGObjectProperty(avatar,@"avatarView");
    if ([avatar isKindOfClass:UIImageView.class]) imageView=avatar;
    if (![imageView isKindOfClass:UIImageView.class]) return;
    if (NGDockAvatarSource!=imageView || NGDockAvatarPhoto!=((UIImageView *)imageView).image) {
        NGDockAvatarSource=imageView; NGDockAvatarPhoto=nil; NGDockAvatarCircle=nil; NGDockQueueAvatarRefresh();
    }
}
static UIImage *NGDockCurrentAvatar(void) {
    if (!NGDockAvatarEnabled()) return nil;
    if (!NGDockAvatarSource && CACurrentMediaTime()-NGDockAvatarLastScan>1) {
        NGDockAvatarLastScan=CACurrentMediaTime(); NSMutableArray *nodes=[NSMutableArray array],*controllers=[NSMutableArray array];
        for (UIWindow *window in NGCaptureWindows()) if (window.windowLevel==UIWindowLevelNormal) { [nodes addObject:window]; if (window.rootViewController) [controllers addObject:window.rootViewController]; }
        for (NSUInteger i=0;i<controllers.count && i<80;i++) {
            UIViewController *controller=controllers[i];
            if ([NGClassName(controller) isEqual:@"SSVideoSeriesFeedViewController"]) {
                id sidebar=NGObjectProperty(controller,@"sideBarVc");
                if ([sidebar isKindOfClass:UIViewController.class] && [sidebar isViewLoaded]) [nodes addObject:[sidebar view]];
            }
            [controllers addObjectsFromArray:controller.childViewControllers];
        }
        for (NSUInteger i=0;i<nodes.count && i<6000 && !NGDockAvatarSource;i++) {
            UIView *node=nodes[i];
            if ([NGClassName(node) isEqual:@"SSVideoSeriesFeedSideBarUserInfoCell"]) NGDockObserveOwnAvatar(node);
            else if (![node.accessibilityIdentifier hasPrefix:@"NaiwaGuo."]) [nodes addObjectsFromArray:node.subviews];
        }
    }
    // Keep the last account photo in memory when the native sidebar unloads.
    // Account updates and logout invalidate it; no photo is saved to disk.
    if (!NGDockAvatarSource) return NGDockAvatarCircle;
    UIImage *photo=NGDockAvatarSource.image;
    if (!photo) { NGDockAvatarPhoto=nil; NGDockAvatarCircle=nil; return nil; }
    if (photo==NGDockAvatarPhoto && NGDockAvatarCircle) return NGDockAvatarCircle;
    NGDockAvatarPhoto=photo;
    if (photo.size.width<1 || photo.size.height<1) return nil;
    CGFloat side=64,scale=MAX(side/photo.size.width,side/photo.size.height);
    CGSize size=CGSizeMake(photo.size.width*scale,photo.size.height*scale);
    NGDockAvatarCircle=[[[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(side,side)] imageWithActions:^(UIGraphicsImageRendererContext *context){
        [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(0,0,side,side)] addClip];
        [photo drawInRect:CGRectMake((side-size.width)/2,(side-size.height)/2,size.width,size.height)];
    }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    return NGDockAvatarCircle;
}
static void NGInstallDockAvatarHooks(void) {
    static NSMutableSet *installed; if (!installed) installed=[NSMutableSet set];
    Class cell=NSClassFromString(@"SSVideoSeriesFeedSideBarUserInfoCell");
    if (cell && ![installed containsObject:@"accountCell"]) {
        for (NSString *name in @[@"layoutSubviews",@"updateUserInfo"]) {
            SEL sel=NSSelectorFromString(name); Method method=class_getInstanceMethod(cell,sel); if (!method) continue; IMP original=method_getImplementation(method);
            class_replaceMethod(cell,sel,imp_implementationWithBlock(^(UIView *view){ ((void(*)(id,SEL))original)(view,sel); NGDockObserveOwnAvatar(view); }),method_getTypeEncoding(method));
        }
        [installed addObject:@"accountCell"];
    }
    Class image=NSClassFromString(@"SSBaseReaderImageView");
    if (image && ![installed containsObject:@"accountImage"]) {
        SEL sel=@selector(setImage:); Method method=class_getInstanceMethod(image,sel); if (!method) return; IMP original=method_getImplementation(method);
        class_replaceMethod(image,sel,imp_implementationWithBlock(^(UIImageView *view,UIImage *value){
            ((void(*)(id,SEL,id))original)(view,sel,value);
            UIView *owner=NGDockOwnAvatarCell(view); if (owner) NGDockObserveOwnAvatar(owner);
        }),method_getTypeEncoding(method)); [installed addObject:@"accountImage"];
    }
    Class home=NSClassFromString(@"SSVideoSeriesFeedViewController");
    if (home && ![installed containsObject:@"logout"]) {
        SEL sel=NSSelectorFromString(@"onAccountLogoutWithUser:"); Method method=class_getInstanceMethod(home,sel);
        if (method) {
            IMP original=method_getImplementation(method);
            class_replaceMethod(home,sel,imp_implementationWithBlock(^(id owner,id user){
                NGDockAvatarSource=nil; NGDockAvatarPhoto=nil; NGDockAvatarCircle=nil;
                ((void(*)(id,SEL,id))original)(owner,sel,user); NGDockQueueAvatarRefresh();
            }),method_getTypeEncoding(method)); [installed addObject:@"logout"];
        }
    }
}
