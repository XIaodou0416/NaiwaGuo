static NSDictionary *NGTestLocalSettings011(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;NSString *bundle=NSBundle.mainBundle.bundleIdentifier;
    NSDictionary *saved=[defaults persistentDomainForName:bundle] ?: @{};
    [defaults removePersistentDomainForName:bundle];
    [defaults setBool:YES forKey:NGPreferenceDock];[defaults setBool:YES forKey:NGPreferencePlaybackFullscreen];[defaults setBool:YES forKey:@"com.naiwaguo.playback.hidden.danmakuEntrance"];
    [defaults setBool:YES forKey:NGPreferenceSponsorNever];[defaults setDouble:120 forKey:NGPreferenceDockHeight];[defaults setBool:YES forKey:NGPreferenceChannelRestoreMigration];
    BOOL count=NGEnabledFeatureCount()==3;
    [defaults setDouble:.7 forKey:NGPreferenceFontScale];[defaults setObject:@"Georgia" forKey:NGPreferenceFontName];[defaults setObject:@"bold" forKey:NGPreferencePlaybackPreset];
    NSArray<NSString *> *retiredKeys = @[@"installation", @"installationSecret", @"lastSent", @"blockedIDs", @"noticeShown"];
    for (NSString *suffix in retiredKeys) [defaults setObject:@"retired" forKey:[@"com.naiwaguo.service." stringByAppendingString:suffix]];
    [defaults setObject:@"host-private" forKey:@"host.accountToken"];
    NGRemoveRetiredServicePreferences(defaults);
    NGRemoveRetiredServicePreferences(defaults); // Repeated startup is safe.
    BOOL cleaned=YES;
    for (NSString *suffix in retiredKeys) cleaned &= [defaults objectForKey:[@"com.naiwaguo.service." stringByAppendingString:suffix]]==nil;
    BOOL preserved=[[defaults stringForKey:@"host.accountToken"] isEqual:@"host-private"] && [defaults boolForKey:NGPreferenceSponsorNever];
    BOOL enabled=NGFullscreenEnabled() && NGFeatureBool(NGPreferenceDock) && NGPlaybackElementHidden(@"danmakuEntrance") && fabs(NGTypographyScale()-.7)<.001 && [NGPlaybackCurrentPreset() isEqual:@"bold"];
    NGSettingsBackupController *backup=[NGSettingsBackupController new];[backup loadViewIfNeeded];[backup viewWillAppear:NO];
    BOOL backupUI=[backup tableView:backup.tableView numberOfRowsInSection:0]==2;
    BOOL preview=[NGSettingsGroups()[4][@"action"] isEqual:@"settingsBackup"];
    BOOL removed=NSClassFromString(@"NGUsageSettingsController")==nil;for(NSDictionary *group in NGSettingsGroups())for(NSDictionary *row in group[@"rows"])removed &= ![row[@"action"] isEqual:@"usagePrivacy"];
    [defaults setPersistentDomain:saved forName:bundle];
    return @{@"featureCountExcludesPromptsSizesAndMigrations":@(count),@"retiredServiceDataRemoved":@(cleaned),@"hostAndLocalSettingsPreserved":@(preserved),@"featuresAvailableWithoutServer":@(enabled),@"backupKeepsTwoOriginalActions":@(backupUI),@"backupCategoryOpensDirectDestination":@(preview),@"statisticsPageAndRouteRemoved":@(removed)};
}

@interface NGSampleHold011 : UILongPressGestureRecognizer
@property(nonatomic) UIGestureRecognizerState sampledState;
@end
@implementation NGSampleHold011
- (UIGestureRecognizerState)state { return self.sampledState; }
@end
static void NGRunGestureUI011(UIWindow *window) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;[defaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];[defaults setBool:YES forKey:NGPreferenceSponsorNever];
    NGInstallSettingsGesture();
    UILongPressGestureRecognizer *installed=objc_getAssociatedObject(window,&NGSettingsGestureKey);NGInstallSettingsGesture();
    NSUInteger copies=0;for(UIGestureRecognizer *g in window.gestureRecognizers)if([g.name isEqual:@"NaiwaGuo.TwoFingerHold"])copies++;
    BOOL config=installed && installed.numberOfTouchesRequired==2 && fabs(installed.minimumPressDuration-.65)<.001 && !installed.cancelsTouchesInView && !installed.delaysTouchesBegan && copies==1;
    NGSettingsGestureTarget *target=[NGSettingsGestureTarget new];NGSampleHold011 *sample=[NGSampleHold011 new];[window addGestureRecognizer:sample];sample.sampledState=UIGestureRecognizerStateBegan;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,.25*NSEC_PER_SEC),dispatch_get_main_queue(),^{[target held:sample];});
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        UINavigationController *nav=(id)window.rootViewController.presentedViewController;
        NGSettingsViewController *home=[nav isKindOfClass:UINavigationController.class] && [nav.topViewController isKindOfClass:NGSettingsViewController.class] ? (id)nav.topViewController : nil;
        BOOL opened=home!=nil;[target held:sample];BOOL duplicate=!nav.presentedViewController;
        [home tableView:home.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:4]];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,.8*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            BOOL backup=[nav.topViewController isKindOfClass:NGSettingsBackupController.class] && nav.viewControllers.count==2;
            [nav popViewControllerAnimated:NO];[defaults setBool:YES forKey:NGPreferencePlaybackFullscreen];[home viewWillAppear:NO];BOOL count=[home.brandHeader.subtitleLabel.text isEqual:@"已开启 1 项功能"];
            NGSmokeWrite(@"gesture011.json",@{@"passed":@(config && opened && duplicate && backup && count),@"twoFingerHoldInstalledOnceWithoutCancellingTouches":@(config),@"recognizedHoldOpensSettings":@(opened),@"repeatedHoldDoesNotStackPanels":@(duplicate),@"backupOpensInOneTap":@(backup),@"featureCountRefreshesOnReturn":@(count),@"physicalMultitouchNotSimulated":@YES});
        });
    });
}
