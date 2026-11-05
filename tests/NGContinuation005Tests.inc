// Fixtures mirror only the verified host APIs from the supplied page captures.
@interface SSBaseReaderImageView : UIImageView @end
@implementation SSBaseReaderImageView @end
@interface SSAvatarView : UIView
@property(nonatomic,strong) SSBaseReaderImageView *avatarView;
@end
@implementation SSAvatarView @end
@interface SSVideoSeriesFeedSideBarUserInfoCell : UITableViewCell
@property(nonatomic,strong) SSAvatarView *avatarView;
- (void)updateUserInfo;
@end
@implementation SSVideoSeriesFeedSideBarUserInfoCell
- (void)updateUserInfo {}
@end
@interface FQVShortVideoNewSpeedSelectController : UIViewController @end
@implementation FQVShortVideoNewSpeedSelectController @end
@interface FQVShortVideoSmartSpeedSelectCell : UITableViewCell @end
@implementation FQVShortVideoSmartSpeedSelectCell @end
static BOOL NGRectNear005(CGRect a,CGRect b) {
    return NGFSNear(a.origin.x,b.origin.x) && NGFSNear(a.origin.y,b.origin.y) && NGFSNear(a.size.width,b.size.width) && NGFSNear(a.size.height,b.size.height);
}
static NSDictionary *NGTestFullscreenTransitions005(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSDictionary *saved=NGSavePreferences005(@[NGPreferencePlaybackFullscreen]);
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene];
    NGFullscreenFixture *host=[NGFullscreenFixture new]; host.landscape=YES; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; [host.feed layoutIfNeeded];
    FQVCollectionView *feed=host.feed; CGFloat w=window.bounds.size.width,h=window.bounds.size.height,n=host.nativeHeight;
    UIView *render=[[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:0]] viewWithTag:7402];
    CGRect small=CGRectMake(37,213,w*0.60,w*0.60*9/16); render.frame=small;
    CGRect original=[render convertRect:render.bounds toView:window];
    [d setBool:YES forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    BOOL smallPreserved=NGRectNear005([render convertRect:render.bounds toView:window],original);
    UIView *channel=[[UIView alloc] initWithFrame:host.view.bounds]; [host.view insertSubview:channel atIndex:0]; [channel addSubview:feed];
    BOOL channels=YES;
    for (NSNumber *fraction in @[@(-0.65),@0.45,@1,@0]) {
        channel.frame=CGRectMake(w*fraction.doubleValue,0,w,h); [feed setNeedsLayout]; [feed layoutIfNeeded]; NGFullscreenRefreshFeed(feed);
        CGRect page=[feed convertRect:feed.bounds toView:window],video=[render convertRect:render.bounds toView:window];
        channels &= NGFSNear(page.size.height,h) && NGFSNear(video.size.width,small.size.width) && NGFSNear(video.size.height,small.size.height) && NGFSNear(video.origin.y,small.origin.y);
    }
    render.frame=CGRectMake(0,0,w,n);
    BOOL transforms=YES,insets=YES;
    for (NSUInteger i=0;i<30;i++) {
        render.transform=CGAffineTransformMake(0.65,0,0,0.65,0,-90);
        CGRect expected=CGRectMake(w*0.175,n*0.175-90,w*0.65,n*0.65);
        transforms &= NGRectNear005([render convertRect:render.bounds toView:window],expected);
        render.transform=CGAffineTransformIdentity;
        transforms &= NGFSNear([render convertRect:render.bounds toView:window].size.height,h);
        CGRect inset=CGRectMake(43,52,w*0.60,n*0.44); render.frame=inset;
        insets &= NGRectNear005([render convertRect:render.bounds toView:window],inset);
        render.frame=CGRectMake(0,0,w,n);
    }
    UIView *engine=render.superview; engine.frame=CGRectMake(0,35,w,n);
    render.frame=small; NGFullscreenRefreshFeed(feed);
    CGRect nested=small; nested.origin.y+=35;
    BOOL nestedPreserved=NGRectNear005([render convertRect:render.bounds toView:window],nested);
    render.transform=CGAffineTransformMakeScale(0.7,0.7);
    [d setBool:NO forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    BOOL restores=CGAffineTransformEqualToTransform(render.transform,CGAffineTransformMakeScale(0.7,0.7)) && CGAffineTransformIsIdentity(feed.transform);
    window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"smallWideVideoKeepsExactNativeFrame":@(smallPreserved),@"horizontalChannelSwipeKeepsBottomCoverage":@(channels),@"thirtyPanelTransformOpenCloseCycles":@(transforms),@"thirtyInsetFrameOpenCloseCycles":@(insets),@"nestedPlayerOffsetPreserved":@(nestedPreserved),@"disableRestoresHostSheetTransform":@(restores)};
}
static NSDictionary *NGTestDockActions005(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceDockHaptics,NGPreferenceDockCollapse,NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize,NGPreferenceDockAction,NGPreferenceDockAvatar,NGPreferenceHiddenDockItems]);
    BOOL defaults=[NGDockAction() isEqual:@"search"] && !NGDockAvatarEnabled();
    [d setObject:@"invalid" forKey:NGPreferenceDockAction]; BOOL invalid=[NGDockAction() isEqual:@"search"];
    [d setBool:YES forKey:NGPreferenceDock];
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; NGRefreshDock(host);
    NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); BOOL destinations=YES,labels=YES;
    for (NSUInteger i=0;i<4;i++) {
        [d setObject:NGDockItemKeys()[i] forKey:NGPreferenceDockAction]; [dock refreshImages]; NSUInteger taps=host.taps;
        [dock.searchHit sendActionsForControlEvents:UIControlEventTouchUpInside];
        destinations &= host.taps==taps+1 && host.selectedIndex==i;
        labels &= [dock.searchHit.accessibilityLabel isEqual:NGDockItemTitles()[i]];
    }
    [d setObject:@"mine" forKey:NGPreferenceDockAction];
    UIControl *last=NGDockOriginalButtons(host.customBar).lastObject; last.enabled=NO; NSUInteger taps=host.taps;
    BOOL disabled=!NGDockInvokeAction(dock) && host.taps==taps; last.enabled=YES;
    [d setObject:@"settings" forKey:NGPreferenceDockAction]; [dock refreshImages]; labels &= [dock.searchHit.accessibilityLabel isEqual:@"插件设置"];
    [d setBool:YES forKey:NGPreferenceDockAvatar]; NGDockAvatarSource=nil; NGDockAvatarPhoto=nil; NGDockAvatarCircle=nil;
    NGInstallDockAvatarHooks();
    SSBaseReaderImageView *author=[SSBaseReaderImageView new]; author.image=[UIImage systemImageNamed:@"heart.fill"]; [host.view addSubview:author];
    BOOL excludesAuthor=NGDockCurrentAvatar()==nil;
    SSVideoSeriesFeedSideBarUserInfoCell *cell=[SSVideoSeriesFeedSideBarUserInfoCell new]; cell.avatarView=[SSAvatarView new]; cell.avatarView.avatarView=[SSBaseReaderImageView new]; [cell.contentView addSubview:cell.avatarView]; [cell.avatarView addSubview:cell.avatarView.avatarView]; [host.view addSubview:cell];
    UIImage *(^photo)(UIColor *)=^UIImage *(UIColor *color) { return [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(60,90)] imageWithActions:^(UIGraphicsImageRendererContext *ctx){ [color setFill]; UIRectFill(CGRectMake(0,0,60,90)); }]; };
    cell.avatarView.avatarView.image=photo(UIColor.orangeColor); [cell updateUserInfo]; [dock refreshImages];
    BOOL account=NGColorPixels005(dock.searchIcon.image,UIColor.orangeColor)>100 && dock.searchIcon.image.renderingMode==UIImageRenderingModeAlwaysOriginal;
    BOOL centered=YES,avatar=YES;
    [d setInteger:3 forKey:NGPreferenceDockCollapse];
    for (NSNumber *size in @[@60,@80,@100]) {
        [d setDouble:size.doubleValue forKey:NGPreferenceDockHeight]; [dock layoutGlass]; [dock.systemHost.view layoutIfNeeded]; [dock layoutGlass]; [dock setCollapsed:YES]; [dock layoutGlass];
        CGRect circle=[dock.collapsePlatter convertRect:dock.collapsePlatter.bounds toView:dock.view];
        centered &= dock.collapsed && NGFSNear(CGRectGetMidX(circle),dock.view.bounds.size.width/2) && dock.searchKey.alpha==0;
        UIImage *icon=dock.viewControllers[dock.selectedIndex].tabBarItem.image; avatar &= NGColorPixels005(icon,UIColor.orangeColor)>100 && NGFSNear(icon.size.width,38);
        [dock setCollapsed:NO];
    }
    cell.avatarView.avatarView.image=photo(UIColor.purpleColor); [dock refreshImages];
    BOOL updated=NGColorPixels005(dock.searchIcon.image,UIColor.purpleColor)>100;
    [cell removeFromSuperview]; cell=nil;
    BOOL unloaded=NGColorPixels005(NGDockCurrentAvatar(),UIColor.purpleColor)>100;
    [[SSVideoSeriesFeedViewController new] onAccountLogoutWithUser:nil]; [dock refreshImages];
    BOOL logout=NGDockAvatarCircle==nil && NGDockAvatarPhoto==nil && NGDockAvatarSource==nil;
    NGDockAppearanceSettingsController *settings=[NGDockAppearanceSettingsController new]; [settings loadViewIfNeeded];
    BOOL rows=[settings tableView:settings.tableView numberOfRowsInSection:2]==4 && [settings tableView:settings.tableView numberOfRowsInSection:5]==6;
    [settings tableView:settings.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:7]];
    BOOL reset=NGDockCollapseSide()==0 && [NGDockAction() isEqual:@"search"] && !NGDockAvatarEnabled();
    window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"searchAndAvatarDefaults":@(defaults),@"invalidActionFallsBackToSearch":@(invalid),@"allFourNativeDestinationsDispatchOnce":@(destinations),@"buttonAccessibilityFollowsFunction":@(labels),@"disabledNativeDestinationDoesNotDispatch":@(disabled),@"videoAuthorIsNeverAccountAvatar":@(excludesAuthor),@"nativeAccountPhotoKeepsOwnColors":@(account),@"centerCollapseAtThreeSizes":@(centered),@"centerCollapsedAvatarHasOwnColors":@(avatar),@"accountPhotoUpdatesImmediately":@(updated),@"sidebarUnloadKeepsMemoryOnlyPhoto":@(unloaded),@"logoutClearsAccountPhoto":@(logout),@"allNewSettingsVisible":@(rows),@"resetRestoresActionAndAppearance":@(reset)};
}
static NSDictionary *NGTestSpeedGlass005(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSDictionary *saved=NGSavePreferences005(@[NGPreferenceShareGlass,NGPreferenceShareGlassClear]);
    FQVShortVideoNewSpeedSelectController *controller=[FQVShortVideoNewSpeedSelectController new]; controller.view.frame=CGRectMake(0,0,402,440); controller.view.backgroundColor=UIColor.whiteColor;
    FQVShortVideoSmartSpeedSelectCell *cell=[[FQVShortVideoSmartSpeedSelectCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil]; cell.frame=CGRectMake(0,80,402,54); cell.backgroundColor=UIColor.whiteColor; cell.textLabel.text=@"1.5x"; UISwitch *toggle=[UISwitch new]; cell.accessoryView=toggle; [controller.view addSubview:cell];
    NGInstallCommentGlassHooks(); [controller viewDidLayoutSubviews]; BOOL off=objc_getAssociatedObject(controller.view,&NGCommentGlassAssociation)==nil;
    [d setBool:YES forKey:NGPreferenceShareGlass]; [controller viewDidLayoutSubviews]; UIVisualEffectView *glass=objc_getAssociatedObject(controller.view,&NGCommentGlassAssociation);
    BOOL regular=glass && [glass.accessibilityIdentifier containsString:@"Speed"] && [cell.backgroundColor isEqual:UIColor.clearColor];
    [d setBool:YES forKey:NGPreferenceShareGlassClear]; [controller viewDidLayoutSubviews];
    BOOL clear=NO; if (@available(iOS 26.0,*)) clear=[glass.effect isKindOfClass:UIGlassEffect.class] && [objc_getAssociatedObject(glass,&NGPanelMaterialStateAssociation) hasPrefix:@"1|"];
    for (NSUInteger i=0;i<50;i++) { cell.backgroundColor=UIColor.whiteColor; [cell layoutSubviews]; [controller viewDidLayoutSubviews]; }
    BOOL reused=glass==objc_getAssociatedObject(controller.view,&NGCommentGlassAssociation) && [cell.backgroundColor isEqual:UIColor.clearColor] && cell.accessoryView==toggle;
    [d setBool:NO forKey:NGPreferenceShareGlass]; [controller viewDidLayoutSubviews];
    BOOL restore=!glass.superview && [controller.view.backgroundColor isEqual:UIColor.whiteColor] && [cell.backgroundColor isEqual:UIColor.whiteColor];
    NGRestorePreferences005(saved);
    return @{@"defaultOff":@(off),@"speedControllerGetsRegularGlass":@(regular),@"speedControllerGetsClearGlass":@(clear),@"fiftyRelayoutsKeepMaterialAndNativeControl":@(reused),@"disableRestoresNativePanel":@(restore)};
}
static void NGRunPanelMotion005(UIWindow *window) {
    NGStart(); NSUserDefaults *d=NSUserDefaults.standardUserDefaults; [d setBool:YES forKey:NGPreferencePlaybackFullscreen];
    NGFullscreenFixture *host=[NGFullscreenFixture new]; window.rootViewController=host; [host.view layoutIfNeeded]; [host.feed layoutIfNeeded]; NGUpdatePlaybackFullscreen();
    UIView *render=[[host.feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:0]] viewWithTag:7402];
    CGFloat full=window.bounds.size.height,native=host.nativeHeight;
    __block NSUInteger samples=0,phase=0; __block CGFloat previous=full,minHeight=full,maxHeight=0; __block BOOL shrinks=YES,expands=YES;
    [NSTimer scheduledTimerWithTimeInterval:0.012 repeats:YES block:^(NSTimer *timer){
        CALayer *presentation=render.layer.presentationLayer;
        if (!presentation) return;
        CALayer *base=window.layer.presentationLayer ?: window.layer;
        CGFloat h=[presentation convertRect:presentation.bounds toLayer:base].size.height;
        if (phase==1) shrinks &= h<=previous+2;
        if (phase==2) expands &= h>=previous-2;
        previous=h; minHeight=MIN(minHeight,h); maxHeight=MAX(maxHeight,h); samples++;
        if (phase==3) {
            [timer invalidate]; BOOL final=NGFSNear([render convertRect:render.bounds toView:window].size.height,full);
            BOOL bounded=minHeight>=native*0.65-2 && maxHeight<=full+2;
            NGSmokeWrite(@"panel-motion-005.json",@{@"passed":@(samples>=45 && shrinks && expands && bounded && final),@"samples":@(samples),@"openingNeverZoomsOutward":@(shrinks),@"closingNeverJumpsBack":@(expands),@"nativeAnimationBoundsPreserved":@(bounded),@"closedReturnsToFullscreen":@(final),@"minHeight":@(minHeight),@"maxHeight":@(maxHeight)});
        }
    }];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.35*NSEC_PER_SEC),dispatch_get_main_queue(),^{ phase=1; [UIView animateWithDuration:0.35 animations:^{ render.transform=CGAffineTransformMake(0.65,0,0,0.65,0,-90); }]; });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.8*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NGSmokeImage(window,@"panel-motion-005.png"); phase=2; [UIView animateWithDuration:0.35 animations:^{ render.transform=CGAffineTransformIdentity; }]; });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.4*NSEC_PER_SEC),dispatch_get_main_queue(),^{ phase=3; });
}
