// Regression coverage for user-visible pixels, input feedback and cold startup.
static NSUInteger NGColorPixels005(UIImage *image, UIColor *color) {
    if (!image.CGImage) return 0;
    size_t w=CGImageGetWidth(image.CGImage),h=CGImageGetHeight(image.CGImage);
    NSMutableData *bytes=[NSMutableData dataWithLength:w*h*4];
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(bytes.mutableBytes,w,h,8,w*4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGContextDrawImage(context,CGRectMake(0,0,w,h),image.CGImage); CGContextRelease(context); CGColorSpaceRelease(space);
    CGFloat r,g,b,a; [color getRed:&r green:&g blue:&b alpha:&a]; NSUInteger count=0;
    const unsigned char *p=bytes.bytes;
    for (NSUInteger i=0;i<w*h;i++,p+=4) if (p[3]>180 && fabs(p[0]-r*255)<40 && fabs(p[1]-g*255)<40 && fabs(p[2]-b*255)<40) count++;
    return count;
}
static UIImage *NGSnapshot005(UIView *view) {
    return [[[UIGraphicsImageRenderer alloc] initWithBounds:view.bounds] imageWithActions:^(UIGraphicsImageRendererContext *context){ [view drawViewHierarchyInRect:view.bounds afterScreenUpdates:YES]; }];
}
static NSDictionary *NGSavePreferences005(NSArray<NSString *> *keys) {
    NSMutableDictionary *saved=[NSMutableDictionary dictionary];
    for (NSString *key in keys) { saved[key]=[NSUserDefaults.standardUserDefaults objectForKey:key] ?: NSNull.null; [NSUserDefaults.standardUserDefaults removeObjectForKey:key]; }
    return saved;
}
static void NGRestorePreferences005(NSDictionary *saved) {
    for (NSString *key in saved) { if (saved[key]==NSNull.null) [NSUserDefaults.standardUserDefaults removeObjectForKey:key]; else [NSUserDefaults.standardUserDefaults setObject:saved[key] forKey:key]; }
}
static NSDictionary *NGTestDockColors005(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceDockCollapse,NGPreferenceDockContent,NGPreferenceDockClear,NGPreferenceDockNormalColor,NGPreferenceDockSelectedColor,NGDockIconImageKey(0)]);
    [d setBool:YES forKey:NGPreferenceDock]; [d setObject:@[@1,@0,@0,@1] forKey:NGPreferenceDockNormalColor]; [d setObject:@[@0,@0,@1,@1] forKey:NGPreferenceDockSelectedColor];
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; NGRefreshDock(host);
    NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); BOOL baked=YES,visible=YES,cached=YES;
    for (NSNumber *style in @[@(UIUserInterfaceStyleLight),@(UIUserInterfaceStyleDark)]) for (NSInteger mode=0;mode<3;mode++) {
        [d setInteger:mode forKey:NGPreferenceDockContent]; window.overrideUserInterfaceStyle=style.integerValue; [dock refreshImages]; [dock layoutGlass]; [dock.view layoutIfNeeded];
        UITabBarItem *item=dock.viewControllers.firstObject.tabBarItem;
        baked &= item.image.renderingMode==UIImageRenderingModeAlwaysOriginal && item.selectedImage.renderingMode==UIImageRenderingModeAlwaysOriginal && NGColorPixels005(item.image,UIColor.redColor)>6 && NGColorPixels005(item.selectedImage,UIColor.blueColor)>6;
        UIImage *snapshot=NGSnapshot005(dock.view);
        visible &= NGColorPixels005(snapshot,UIColor.redColor)>6 && NGColorPixels005(snapshot,UIColor.blueColor)>6;
        NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:[NSString stringWithFormat:@"dock-colors-005-%ld-%ld.png",(long)style.integerValue,(long)mode]];
        [UIImagePNGRepresentation(snapshot) writeToFile:path atomically:YES];
        NSData *normal=UIImagePNGRepresentation(item.image),*selected=UIImagePNGRepresentation(item.selectedImage); NGDockRenderedItemState *cachedState=objc_getAssociatedObject(item,&NGDockRenderedItemAssociation);
        for (NSUInteger i=0;i<100;i++) [dock refreshImages];
        cached &= cachedState==objc_getAssociatedObject(item,&NGDockRenderedItemAssociation) && [normal isEqualToData:UIImagePNGRepresentation(item.image)] && [selected isEqualToData:UIImagePNGRepresentation(item.selectedImage)];
    }
    NGDockColorSettingsController *editor=[NGDockColorSettingsController new]; [editor loadViewIfNeeded];
    UIColorPickerViewController *picker=[UIColorPickerViewController new]; editor.editingSelected=YES;
    [editor colorPickerViewController:picker didSelectColor:UIColor.greenColor continuously:YES]; [dock refreshImages];
    BOOL live=[NGDockColor(YES,dock.inkStyle) isEqual:UIColor.greenColor] && [NGDockColor(NO,dock.inkStyle) isEqual:UIColor.redColor] && [dock.searchIcon.tintColor isEqual:UIColor.greenColor] && [dock.tabBar.tintColor isEqual:UIColor.greenColor];
    picker.selectedColor=UIColor.blueColor; [editor colorPickerViewControllerDidFinish:picker]; [dock refreshImages];
    BOOL finished=NGColorPixels005(dock.viewControllers.firstObject.tabBarItem.selectedImage,UIColor.blueColor)>6;
    editor.editingSelected=NO; picker.selectedColor=UIColor.redColor;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [editor colorPickerViewControllerDidSelectColor:picker];
#pragma clang diagnostic pop
    [d setInteger:1 forKey:NGPreferenceDockContent]; [d setInteger:2 forKey:NGPreferenceDockCollapse]; [dock layoutGlass]; [dock.systemHost.view layoutIfNeeded]; [dock layoutGlass]; [dock setCollapsed:YES]; [dock refreshImages];
    NSUInteger bluePixels=NGColorPixels005(dock.viewControllers[dock.selectedIndex].tabBarItem.image,UIColor.blueColor);
    BOOL collapsed=dock.collapsed && bluePixels>6;
    NGSmokeWrite(@"dock-colors-005-detail.json",@{@"collapsed":@(dock.collapsed),@"bluePixels":@(bluePixels),@"sourceIconMode":@(NGDockIcon(dock.selectedIndex,YES).renderingMode),@"imageMode":@(dock.viewControllers[dock.selectedIndex].tabBarItem.image.renderingMode),@"imageSize":NSStringFromCGSize(dock.viewControllers[dock.selectedIndex].tabBarItem.image.size),@"index":@(dock.selectedIndex),@"hasPlatter":@(dock.collapsePlatter!=nil),@"color":NGDockColor(YES,dock.inkStyle).description});
    UIImage *photo=[[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(30,30)] imageWithActions:^(UIGraphicsImageRendererContext *context){ [UIColor.orangeColor setFill]; UIRectFill(CGRectMake(0,0,30,30)); }];
    [d setObject:UIImagePNGRepresentation(photo) forKey:NGDockIconImageKey(0)]; [dock refreshImages];
    BOOL custom=NGColorPixels005(dock.viewControllers.firstObject.tabBarItem.image,UIColor.orangeColor)>6;
    [dock setCollapsed:NO]; [dock refreshImages];
    custom &= NGColorPixels005(dock.viewControllers.firstObject.tabBarItem.image,UIColor.orangeColor)>6 && NGColorPixels005(dock.viewControllers.firstObject.tabBarItem.selectedImage,UIColor.orangeColor)>6;
    window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"allThreeModesRetainNormalAndSelectedPixels":@(baked),@"nativeLightAndDarkPlatterDisplaysBothColors":@(visible),@"hundredRefreshesPreserveCachedImages":@(cached),@"modernPickerUpdatesSelectedAndSearchImmediately":@(live),@"pickerDonePersistsFinalColor":@(finished),@"collapsedSymbolUsesSelectedColor":@(collapsed),@"customPhotosKeepOwnColorsExpandedAndCollapsed":@(custom)};
}
static NSDictionary *NGTestDockHaptics005(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceDockHaptics,NGPreferenceDockCollapse]); [d setBool:YES forKey:NGPreferenceDock];
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded];
    SSVideoSeriesFeedViewController *feed=[SSVideoSeriesFeedViewController new]; NSMutableArray *pages=[host.viewControllers mutableCopy]; pages[0]=feed; host.viewControllers=pages; NGRefreshDock(host);
    NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation);
    __block NSUInteger feedback=0;
    Method method=class_getInstanceMethod(UISelectionFeedbackGenerator.class,@selector(selectionChanged)); IMP original=method_getImplementation(method);
    IMP counter=imp_implementationWithBlock(^(__unused UISelectionFeedbackGenerator *generator){ feedback++; }); method_setImplementation(method,counter);
    NGDockAppearanceSettingsController *settings=[NGDockAppearanceSettingsController new]; [settings loadViewIfNeeded];
    UITableViewCell *cell=[settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:0]]; UISwitch *toggle=(id)cell.accessoryView;
    [dock selectPage:1]; BOOL off=!toggle.on && feedback==0 && host.taps==1;
    toggle.on=YES; [toggle sendActionsForControlEvents:UIControlEventValueChanged];
    [dock tabBar:dock.tabBar didSelectItem:dock.viewControllers[2].tabBarItem]; BOOL tap=feedback==1 && host.taps==2;
    [dock.searchHit sendActionsForControlEvents:UIControlEventTouchUpInside]; BOOL search=feedback==2;
    NSUInteger count=feedback;
    for (NSUInteger i=0;i<100;i++) { dock.selectedIndex=i%4; [dock refreshImages]; [dock layoutGlass]; }
    [d setInteger:1 forKey:NGPreferenceDockCollapse]; [dock layoutGlass]; [dock.systemHost.view layoutIfNeeded]; [dock layoutGlass]; [dock setCollapsed:YES]; [dock setCollapsed:NO];
    BOOL silentRefresh=feedback==count;
    [dock tabBarController:dock.systemHost shouldSelectViewController:dock.viewControllers[0]]; BOOL native=feedback==count+1; count=feedback;
    dock.buttons[1].enabled=NO; [dock selectPage:1]; [dock selectPage:NSNotFound]; dock.buttons[1].enabled=YES;
    BOOL invalid=feedback==count;
    toggle.on=NO; [toggle sendActionsForControlEvents:UIControlEventValueChanged]; [dock selectPage:2]; [dock searchTapped]; BOOL switchedOff=feedback==count;
    [d setBool:YES forKey:NGPreferenceDockHaptics]; [d setBool:NO forKey:NGPreferenceDock]; [dock selectPage:1]; BOOL disabled=feedback==count;
    method_setImplementation(method,original); imp_removeBlock(counter);
    window.hidden=YES; [old makeKeyAndVisible]; NGRestorePreferences005(saved);
    return @{@"defaultOffAndNativeTapStillWorks":@(off),@"enabledTapCallsFeedbackOnce":@(tap),@"searchButtonCallsFeedbackOnce":@(search),@"refreshSelectionAndCollapseDoNotVibrate":@(silentRefresh),@"nativeControllerTapCallsFeedbackOnce":@(native),@"invalidAndDisabledButtonsDoNotVibrate":@(invalid),@"toggleOffStopsFeedbackImmediately":@(switchedOff),@"disabledDockDoesNotVibrate":@(disabled)};
}
static void NGRunDefaults005(UIWindow *window,BOOL retained) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSMutableDictionary *checks=[NSMutableDictionary dictionary];
    NSArray *keys=@[NGPreferenceHUD,NGPreferencePerformance,NGPreferenceRuntimeGlass,NGPreferenceDock,NGPreferenceDockHaptics,NGPreferenceDockClear,NGPreferencePlaybackGlass,NGPreferencePlaybackHeaderGlass,NGPreferenceCollectionGlass,NGPreferenceCommentGlass,NGPreferenceShareGlass,NGPreferenceShareGlassClear,NGPreferencePlaybackFullscreen];
    BOOL values=YES; for (NSString *key in keys) values &= [d boolForKey:key]==(retained && ([key isEqual:NGPreferenceDock] || [key isEqual:NGPreferenceDockHaptics]));
    checks[@"featureDefaultsOrSavedChoicesCorrect"]=@(values);
    BOOL nativeAvailable=NO; if (@available(iOS 26.0,*)) nativeAvailable=YES;
    checks[@"nativeCompatibilityAutomaticallyMatchesSystem"]=@(NGRuntimeGlassHooksInstalled==nativeAvailable && NGRuntimeGlassRequested==nativeAvailable);
    checks[@"debugOverlayOff"]=@(!NGCurrentOverlay() || NGCurrentOverlay().hidden);
    checks[@"defaultTypographyAndIcons"]=@(![d stringForKey:NGPreferenceFontName].length && (! [d objectForKey:NGPreferenceFontScale] || fabs([d doubleForKey:NGPreferenceFontScale]-1)<0.001) && [NGPlaybackCurrentPreset() isEqual:@"original"]);
    BOOL rowsOff=YES; for (NSDictionary *group in NGSettingsGroups()) for (NSDictionary *row in group[@"rows"]) if (row[@"key"]) rowsOff &= ![d boolForKey:row[@"key"]];
    checks[@"mainSettingsSwitchesOff"]=@(rowsOff);
    SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [host.view layoutIfNeeded]; NGRefreshDock(host);
    checks[@"dockMatchesOptIn"]=@((objc_getAssociatedObject(host,&NGDockAssociation)!=nil)==retained);
    NGDockAppearanceSettingsController *settings=[NGDockAppearanceSettingsController new]; [settings loadViewIfNeeded]; BOOL toggles=YES;
    for (NSUInteger i=0;i<2;i++) { UISwitch *toggle=(id)[settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:0]].accessoryView; toggles &= toggle.on==retained; if (!retained) { toggle.on=YES; [toggle sendActionsForControlEvents:UIControlEventValueChanged]; } }
    checks[@"dockSettingsShowCorrectSwitches"]=@(toggles);
    checks[@"manualChoicePersists"]=@([d boolForKey:NGPreferenceDock] && [d boolForKey:NGPreferenceDockHaptics]);
    [d synchronize];
    BOOL passed=YES; for (NSNumber *value in checks.allValues) passed &= value.boolValue;
    NGSmokeWrite(retained ? @"retained-defaults-005.json" : @"fresh-defaults-005.json",@{@"passed":@(passed),@"checks":checks});
}
