@interface FQVTouchIgoringView : UIView @end
@implementation FQVTouchIgoringView @end
@interface FQVShortVideoListViewController : UIViewController @end
@implementation FQVShortVideoListViewController
- (BOOL)prefersStatusBarHidden { return NO; }
@end
@interface SSRootViewController : UINavigationController @end
@implementation SSRootViewController
- (BOOL)prefersStatusBarHidden { return NO; }
- (UIViewController *)childViewControllerForStatusBarHidden { return self.topViewController; }
@end
@interface FQVShortVideoSettingsViewController : UIViewController @end
@implementation FQVShortVideoSettingsViewController @end
@interface FQVShortVideoSettingsSwitchCell : UITableViewCell @end
@implementation FQVShortVideoSettingsSwitchCell @end

static NSDictionary *NGTestEpisodeToolbar(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    BOOL saved = [defaults boolForKey:NGPreferencePlaybackHeaderGlass];
    [defaults setBool:YES forKey:NGPreferencePlaybackHeaderGlass];
    FQVShortVideoListViewController *controller = [FQVShortVideoListViewController new];
    controller.view.frame = CGRectMake(0, 0, 402, 874);
    FQVTouchIgoringView *toolbar = [[FQVTouchIgoringView alloc] initWithFrame:CGRectMake(0, 62, 402, 44)]; [controller.view addSubview:toolbar];
    UIButton *back = [UIButton buttonWithType:UIButtonTypeSystem]; back.frame = CGRectMake(20, 10, 24, 24); [toolbar addSubview:back];
    UILabel *episode = [[UILabel alloc] initWithFrame:CGRectMake(44, 9, 47, 26)]; episode.text = @"第5集"; [toolbar addSubview:episode];
    UIView *right = [[UIView alloc] initWithFrame:CGRectMake(285, 0, 102, 44)]; [toolbar addSubview:right];
    UIView *speed = [[UIView alloc] initWithFrame:CGRectMake(0, 10, 62, 24)]; [right addSubview:speed];
    UILabel *rate = [[UILabel alloc] initWithFrame:CGRectMake(26, 2, 36, 20)]; rate.text = @"1.25x"; [speed addSubview:rate];
    UIButton *more = [UIButton buttonWithType:UIButtonTypeSystem]; more.frame = CGRectMake(78, 10, 24, 24); [right addSubview:more];
    NGInstallEpisodeGlassHooks(); NGDecorateEpisodeController(controller);
    NSDictionary *surfaces = objc_getAssociatedObject(toolbar, &NGEpisodeGlassAssociation);
    UIVisualEffectView *leftGlass = surfaces[@"BackEpisodes"], *speedGlass = surfaces[@"Speed"], *moreGlass = surfaces[@"More"];
    BOOL grouping = surfaces.count == 3 && CGRectContainsRect(leftGlass.frame, back.frame) && CGRectGetMaxX(leftGlass.frame) >= CGRectGetMinX(episode.frame) + [episode.text sizeWithAttributes:@{NSFontAttributeName:episode.font}].width - 1 && CGRectGetMaxX(speedGlass.frame) < CGRectGetMinX(moreGlass.frame);
    BOOL touches = [toolbar hitTest:CGPointMake(30, 20) withEvent:nil] == back && [toolbar hitTest:CGPointMake(375, 20) withEvent:nil] == more;
    id material = speedGlass;
    for (NSUInteger i = 0; i < 100; i++) NGDecorateEpisodeController(controller);
    BOOL reused = material && [objc_getAssociatedObject(toolbar, &NGEpisodeGlassAssociation) objectForKey:@"Speed"] == material && toolbar.subviews.count == 6;
    episode.frame = CGRectMake(44, 9, 280, 26); episode.text = @"第128集";
    [toolbar layoutIfNeeded];
    BOOL compactAfterTextUpdate = leftGlass.frame.size.width < 150 && CGRectGetMaxX(leftGlass.frame) >= 44 + [episode.text sizeWithAttributes:@{NSFontAttributeName:episode.font}].width - 1;
    [controller viewWillAppear:NO]; [controller viewDidAppear:NO];
    BOOL reentry = leftGlass.frame.size.width < 150 && !leftGlass.hidden;
    [defaults setBool:NO forKey:NGPreferencePlaybackHeaderGlass]; NGDecorateEpisodeController(controller);
    BOOL disabled = leftGlass.hidden && speedGlass.hidden && moreGlass.hidden;
    [defaults setBool:saved forKey:NGPreferencePlaybackHeaderGlass];
    return @{@"threeSeparateGroups": @(grouping), @"lateEpisodeTextStaysCompact": @(compactAfterTextUpdate), @"entryImmediatelyDecorated": @(reentry), @"nativeTouchesPreserved": @(touches), @"reusesMaterials": @(reused), @"toggleRestores": @(disabled)};
}

static NSDictionary *NGTestDockColors(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    id normal = [defaults objectForKey:NGPreferenceDockNormalColor], selected = [defaults objectForKey:NGPreferenceDockSelectedColor];
    UITabBarItem *item = [[UITabBarItem alloc] initWithTitle:@"首页" image:nil tag:0]; item.accessibilityLabel = @"首页";
    NGDockApplyRenderedImages(item, 0, UIUserInterfaceStyleLight);
    UIImage *firstNormal = item.image, *firstSelected = item.selectedImage;
    [defaults setObject:@[@1, @0, @0, @1] forKey:NGPreferenceDockNormalColor];
    NGDockApplyRenderedImages(item, 0, UIUserInterfaceStyleLight);
    BOOL normalChanges = ![UIImagePNGRepresentation(firstNormal) isEqual:UIImagePNGRepresentation(item.image)] && [UIImagePNGRepresentation(firstSelected) isEqual:UIImagePNGRepresentation(item.selectedImage)];
    UIImage *secondNormal = item.image;
    [defaults setObject:@[@0, @1, @0, @1] forKey:NGPreferenceDockSelectedColor];
    NGDockApplyRenderedImages(item, 0, UIUserInterfaceStyleLight);
    BOOL selectedChanges = [UIImagePNGRepresentation(secondNormal) isEqual:UIImagePNGRepresentation(item.image)] && ![UIImagePNGRepresentation(firstSelected) isEqual:UIImagePNGRepresentation(item.selectedImage)];
    id cached = objc_getAssociatedObject(item, &NGDockRenderedItemAssociation); NGDockApplyRenderedImages(item, 0, UIUserInterfaceStyleLight);
    BOOL reused = objc_getAssociatedObject(item, &NGDockRenderedItemAssociation) == cached;
    if (normal) [defaults setObject:normal forKey:NGPreferenceDockNormalColor]; else [defaults removeObjectForKey:NGPreferenceDockNormalColor];
    if (selected) [defaults setObject:selected forKey:NGPreferenceDockSelectedColor]; else [defaults removeObjectForKey:NGPreferenceDockSelectedColor];
    return @{@"independentNormalColor": @(normalChanges), @"independentSelectedColor": @(selectedChanges), @"cachedRendering": @(reused)};
}

static NSDictionary *NGTestStatusBarRouting(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    BOOL saved = [defaults boolForKey:NGPreferenceHideStatusBar]; UIWindow *previous = NGBaseWindow();
    FQVShortVideoListViewController *player = [FQVShortVideoListViewController new];
    FQVShortVideoPlayerCellView *surface = [[FQVShortVideoPlayerCellView alloc] initWithFrame:CGRectMake(0, 0, 402, 874)]; [player.view addSubview:surface];
    SSRootViewController *root = [[SSRootViewController alloc] initWithRootViewController:player];
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:previous.windowScene]; window.rootViewController = root; [window makeKeyAndVisible];
    [root.view setNeedsLayout]; [root.view layoutIfNeeded]; [player.view layoutIfNeeded];
    NGInstallPlaybackStatusBarHooks(); [defaults setBool:YES forKey:NGPreferenceHideStatusBar]; NGUpdatePlaybackStatusBar();
    BOOL routed = root.prefersStatusBarHidden && !root.childViewControllerForStatusBarHidden && player.prefersStatusBarHidden;
    [defaults setBool:NO forKey:NGPreferenceHideStatusBar]; NGUpdatePlaybackStatusBar();
    BOOL restored = !root.prefersStatusBarHidden && root.childViewControllerForStatusBarHidden == player && !player.prefersStatusBarHidden;
    [defaults setBool:YES forKey:NGPreferenceHideStatusBar];
    NGSettingsViewController *settings = [NGSettingsViewController new]; [root pushViewController:settings animated:NO]; NGUpdatePlaybackStatusBar();
    BOOL settingsVisible = !root.prefersStatusBarHidden && root.childViewControllerForStatusBarHidden == settings;
    window.hidden = YES; [previous makeKeyWindow]; [defaults setBool:saved forKey:NGPreferenceHideStatusBar]; NGUpdatePlaybackStatusBar();
    return @{@"rootAndPlayerOverrides": @(routed), @"restoresOriginalRouting": @(restored), @"settingsKeepsStatusBar": @(settingsVisible)};
}

static NSDictionary *NGTestMorePanelGlass(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    BOOL saved = [defaults boolForKey:NGPreferenceShareGlass]; id savedClear = [defaults objectForKey:NGPreferenceShareGlassClear];
    [defaults setBool:YES forKey:NGPreferenceShareGlass]; [defaults setBool:YES forKey:NGPreferenceShareGlassClear];
    FQVShortVideoSettingsViewController *controller = [FQVShortVideoSettingsViewController new];
    controller.view.frame = CGRectMake(0, 0, 402, 612); controller.view.backgroundColor = UIColor.whiteColor;
    FQVShortVideoSettingsSwitchCell *cell = [[FQVShortVideoSettingsSwitchCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.frame = CGRectMake(20, 100, 362, 54); cell.backgroundColor = UIColor.whiteColor; cell.textLabel.text = @"弹幕";
    UISwitch *toggle = [UISwitch new]; cell.accessoryView = toggle; [controller.view addSubview:cell];
    NGInstallCommentGlassHooks(); NGDecorateShareController(controller);
    UIVisualEffectView *glass = objc_getAssociatedObject(controller.view, &NGCommentGlassAssociation);
    BOOL clear = NO;
    if (@available(iOS 26.0, *)) clear = glass && [glass.effect isKindOfClass:UIGlassEffect.class] && ((UIGlassEffect *)glass.effect).tintColor != nil && [glass.contentView.backgroundColor isEqual:UIColor.clearColor] && [cell.backgroundColor isEqual:UIColor.clearColor];
    id material = objc_getAssociatedObject(glass, &NGPanelMaterialStateAssociation); for (NSUInteger i = 0; i < 100; i++) NGDecorateShareController(controller);
    BOOL reused = material == objc_getAssociatedObject(glass, &NGPanelMaterialStateAssociation);
    cell.backgroundColor = UIColor.whiteColor; [cell setNeedsLayout]; [cell layoutIfNeeded];
    BOOL recycledCell = [cell.backgroundColor isEqual:UIColor.clearColor] && cell.accessoryView == toggle && !toggle.hidden;
    [defaults setBool:NO forKey:NGPreferenceShareGlassClear]; NGDecorateShareController(controller);
    BOOL regular = NO;
    if (@available(iOS 26.0, *)) regular = [glass.effect isKindOfClass:UIGlassEffect.class] && ![objc_getAssociatedObject(glass, &NGPanelMaterialStateAssociation) isEqual:material] && ((UIGlassEffect *)glass.effect).tintColor == nil;
    [defaults setBool:NO forKey:NGPreferenceShareGlass]; NGDecorateShareController(controller);
    BOOL restored = [controller.view.backgroundColor isEqual:UIColor.whiteColor] && [cell.backgroundColor isEqual:UIColor.whiteColor] && !glass.superview;
    [defaults setBool:saved forKey:NGPreferenceShareGlass]; if (savedClear) [defaults setObject:savedClear forKey:NGPreferenceShareGlassClear]; else [defaults removeObjectForKey:NGPreferenceShareGlassClear];
    return @{@"clearMaterialWithoutOpaquePlate": @(clear), @"reusesMaterial": @(reused), @"recycledCellAndControls": @(recycledCell), @"regularMaterial": @(regular), @"restoresOriginalPanel": @(restored)};
}
