static BOOL NGBlack006(UIColor *color) { CGFloat r=1,g=1,b=1,a=0; return [color getRed:&r green:&g blue:&b alpha:&a] && r<.001 && g<.001 && b<.001 && a>.99; }
static NSDictionary *NGTestDockSurface006(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceDockCollapse]); [d setBool:YES forKey:NGPreferenceDock];
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; NGInstallDockHooks(); NGRefreshDock(host);
    NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); UIView *bar=host.customBar;
    UIView *cover=[[UIView alloc] initWithFrame:bar.bounds]; cover.backgroundColor=UIColor.blackColor; [bar addSubview:cover];
    BOOL late=cover.layer.opacity==0 && !cover.userInteractionEnabled;
    BOOL paint=YES;
    // A horizontal channel callback can repaint the old tab background without layout.
    for (NSUInteger i=0;i<100;i++) {
        bar.backgroundColor=UIColor.blackColor; cover.alpha=1; cover.userInteractionEnabled=YES;
        paint &= CGColorGetAlpha(bar.backgroundColor.CGColor)==0 && cover.layer.opacity==0 && !cover.userInteractionEnabled;
        bar.layer.backgroundColor=UIColor.darkGrayColor.CGColor; cover.layer.opacity=0.7;
        paint &= CGColorGetAlpha(bar.layer.backgroundColor)==0 && cover.layer.opacity==0;
    }
    UIView *plain=[UIView new]; plain.backgroundColor=UIColor.redColor; plain.layer.opacity=0.8; plain.userInteractionEnabled=NO;
    BOOL unrelated=[plain.backgroundColor isEqual:UIColor.redColor] && fabs(plain.layer.opacity-0.8)<.001 && !plain.userInteractionEnabled;
    [d setBool:NO forKey:NGPreferenceDock]; NGRefreshDock(host);
    BOOL restored=fabs(cover.layer.opacity-0.7)<.001 && cover.userInteractionEnabled && CGColorEqualToColor(bar.layer.backgroundColor,UIColor.darkGrayColor.CGColor);
    cover.alpha=0.4; bar.backgroundColor=UIColor.blueColor;
    restored &= fabs(cover.layer.opacity-.4)<.001 && [bar.backgroundColor isEqual:UIColor.blueColor];
    window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"newNativeCoverImmediatelySuppressed":@(late),@"hundredChannelBackgroundAndAlphaRepaintsSuppressed":@(paint),@"disableRestoresLatestNativePaintAndInteraction":@(restored),@"unrelatedViewsUnaffected":@(unrelated),@"managedDockWasCreated":@(dock!=nil)};
}
static CGRect NGColorBounds006(UIImage *image,UIColor *color) {
    if (!image.CGImage) return CGRectNull;
    size_t w=CGImageGetWidth(image.CGImage),h=CGImageGetHeight(image.CGImage); NSMutableData *data=[NSMutableData dataWithLength:w*h*4]; CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB(); CGContextRef ctx=CGBitmapContextCreate(data.mutableBytes,w,h,8,w*4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big); CGContextDrawImage(ctx,CGRectMake(0,0,w,h),image.CGImage); CGContextRelease(ctx); CGColorSpaceRelease(space);
    CGFloat r,g,b,a; [color getRed:&r green:&g blue:&b alpha:&a]; const unsigned char *p=data.bytes; NSInteger minX=w,minY=h,maxX=-1,maxY=-1;
    for (NSUInteger y=0;y<h;y++) for (NSUInteger x=0;x<w;x++) { const unsigned char *v=p+4*(y*w+x); if (v[3]>230 && fabs(v[0]-r*255)<25 && fabs(v[1]-g*255)<25 && fabs(v[2]-b*255)<25) { minX=MIN(minX,(NSInteger)x);maxX=MAX(maxX,(NSInteger)x);minY=MIN(minY,(NSInteger)y);maxY=MAX(maxY,(NSInteger)y); } }
    return maxX<0 ? CGRectNull : CGRectMake(minX/image.scale,minY/image.scale,(maxX-minX+1)/image.scale,(maxY-minY+1)/image.scale);
}
static NSDictionary *NGTestDockAvatarAndInk006(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceDockCollapse,NGPreferenceDockAvatar,NGPreferenceDockContent,NGPreferenceDockHeight,NGPreferenceDockSearchSize,NGPreferenceDockSelectedColor,NGPreferenceDockNormalColor,NGDockIconImageKey(0)]);
    [d setBool:YES forKey:NGPreferenceDock]; [d setInteger:3 forKey:NGPreferenceDockCollapse]; [d setInteger:1 forKey:NGPreferenceDockContent]; [d setObject:@[@0,@0,@0,@1] forKey:NGPreferenceDockSelectedColor]; [d setObject:@[@1,@0,@0,@1] forKey:NGPreferenceDockNormalColor];
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; NGRefreshDock(host); NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation);
    BOOL black=YES,appearances=YES; NSUInteger selectedBlackViews=0;
    for (NSNumber *style in @[@(UIUserInterfaceStyleLight),@(UIUserInterfaceStyleDark)]) {
        window.overrideUserInterfaceStyle=style.integerValue;
        for (NSUInteger index=0;index<4;index++) {
            [dock selectPage:index]; [dock layoutGlass]; [dock refreshImages]; [dock.tabBar layoutIfNeeded];
            dock.tabBar.tintColor=UIColor.whiteColor; appearances &= NGBlack006(dock.tabBar.tintColor);
            UITabBarItem *item=dock.viewControllers[index].tabBarItem; black &= NGColorPixels005(item.selectedImage,UIColor.blackColor)>6;
            for (UITabBarItemAppearance *a in @[dock.tabBar.standardAppearance.stackedLayoutAppearance,dock.tabBar.standardAppearance.inlineLayoutAppearance,dock.tabBar.standardAppearance.compactInlineLayoutAppearance]) appearances &= NGBlack006(a.selected.iconColor);
        }
        [dock selectPage:0]; [dock setCollapsed:YES]; [dock setCollapsed:NO]; [dock refreshImages]; [dock layoutGlass]; [dock.tabBar layoutIfNeeded];
        NSMutableArray *nodes=[NSMutableArray arrayWithObject:dock.tabBar];
        for (NSUInteger i=0;i<nodes.count && i<300;i++) { UIView *v=nodes[i]; if ([v isKindOfClass:UIImageView.class] && NGColorPixels005(((UIImageView *)v).image,UIColor.blackColor)>6) selectedBlackViews++; [nodes addObjectsFromArray:v.subviews]; }
        NSString *name=style.integerValue==UIUserInterfaceStyleDark ? @"home-black-dark-006.png" : @"home-black-light-006.png"; NGSmokeImage(window,name);
    }
    [d setBool:YES forKey:NGPreferenceDockAvatar]; UIImageView *source=[UIImageView new]; source.image=[[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(80,80)] imageWithActions:^(UIGraphicsImageRendererContext *ctx){[UIColor.orangeColor setFill];UIRectFill(CGRectMake(0,0,80,80));}]; [host.view addSubview:source]; source.hidden=YES; NGDockAvatarSource=source; NGDockAvatarPhoto=nil; NGDockAvatarCircle=nil;
    BOOL same=YES,ratio=YES; NSMutableArray *measurements=[NSMutableArray array];
    for (NSNumber *size in @[@60,@90,@100]) {
        [d setDouble:size.doubleValue forKey:NGPreferenceDockHeight]; [d setDouble:size.doubleValue forKey:NGPreferenceDockSearchSize]; [dock setCollapsed:NO]; [dock refreshImages]; [dock layoutGlass]; [dock.systemHost.view layoutIfNeeded]; [dock layoutGlass];
        CGRect ep=dock.searchIcon.bounds; same &= NGColorPixels005(dock.searchIcon.image,UIColor.orangeColor)>100;
        [dock setCollapsed:YES]; [dock refreshImages]; [dock layoutGlass];
        UIImage *collapsed=dock.viewControllers[dock.selectedIndex].tabBarItem.image; CGRect cp=NGColorBounds006(collapsed,UIColor.orangeColor); CGFloat scale=NGDockSize(NGPreferenceDockHeight);
        same &= !CGRectIsNull(ep) && !CGRectIsNull(cp) && fabs(ep.size.width-cp.size.width*scale)<1.2;
        ratio &= fabs(ep.size.width/dock.searchKey.bounds.size.width - cp.size.width/dock.collapsePlatter.bounds.size.width)<0.03;
        [measurements addObject:@{@"percent":size,@"expandedAvatarPoints":@(ep.size.width),@"collapsedAvatarPoints":@(cp.size.width*scale)}];
    }
    NGSmokeWrite(@"avatar-size-006.json",@{@"equalVisibleSize":@(same),@"equalRatio":@(ratio),@"measurements":measurements});
    NGDockAvatarSource=nil; NGDockAvatarPhoto=nil; NGDockAvatarCircle=nil; window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"allFourTabsKeepBlackSelectedPixels":@(black),@"allThreeNativeLayoutAppearancesUseBlack":@(appearances),@"homeImageViewsKeepBlackAfterCollapseAndExpand":@(selectedBlackViews>=2),@"avatarVisibleSizeMatchesAt60_90_100Percent":@(same),@"avatarToCircleRatioMatches":@(ratio)};
}
static NSDictionary *NGTestLegacyDock006(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceDockCollapse,NGPreferenceDockHaptics]); [d setBool:YES forKey:NGPreferenceDock]; [d setInteger:3 forKey:NGPreferenceDockCollapse];
    NGDockForceLegacyTest=YES;
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; NGRefreshDock(host); NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation);
    [dock refreshImages]; [dock layoutGlass];
    BOOL built=dock && dock.legacyMaterial && dock.legacyButtons.count==4 && !dock.systemHost && dock.collapseSide==0 && NGDockCollapseSide()==0;
    BOOL effects=[dock.legacyPlatter.effect isKindOfClass:UIBlurEffect.class] && [dock.searchKey.effect isKindOfClass:UIBlurEffect.class] && dock.legacyPlatter.layer.cornerRadius>0 && dock.searchKey.layer.cornerRadius>0;
    NSUInteger taps=host.taps; [dock.legacyButtons[2] sendActionsForControlEvents:UIControlEventTouchUpInside]; BOOL routes=host.selectedIndex==2 && host.taps==taps+1;
    CGRect frame=[dock.legacyButtons[2] convertRect:dock.legacyButtons[2].bounds toView:window]; UIView *hit=[window hitTest:CGPointMake(CGRectGetMidX(frame),CGRectGetMidY(frame)) withEvent:nil]; BOOL touches=hit==dock.legacyButtons[2] || [hit isDescendantOfView:dock.legacyButtons[2]];
    [d setBool:NO forKey:NGPreferenceDock]; NGRefreshDock(host); BOOL restored=!objc_getAssociatedObject(host,&NGDockAssociation) && NGDockOriginalButtons(host.customBar).firstObject.userInteractionEnabled;
    NGDockForceLegacyTest=NO; window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"fallbackDockUsesFourPublicButtons":@(built),@"fallbackUsesRoundedBlurInsteadOfGlass":@(effects),@"fallbackRoutesNativeTabOnce":@(routes),@"fallbackButtonHitTestingWorks":@(touches),@"fallbackDisableRestoresHost":@(restored)};
}
