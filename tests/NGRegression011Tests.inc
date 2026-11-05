static NSDictionary *NGTestAccountBlocks011(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;NSString *bundle=NSBundle.mainBundle.bundleIdentifier;
    NSDictionary *saved=[defaults persistentDomainForName:bundle] ?: @{};
    [defaults removePersistentDomainForName:bundle];NGBlocked=NO;
    [defaults setBool:YES forKey:NGPreferenceDock];[defaults setBool:YES forKey:NGPreferencePlaybackFullscreen];[defaults setBool:YES forKey:@"com.naiwaguo.playback.hidden.danmakuEntrance"];
    [defaults setBool:YES forKey:NGPreferenceSponsorNever];[defaults setDouble:120 forKey:NGPreferenceDockHeight];[defaults setBool:YES forKey:NGPreferenceChannelRestoreMigration];
    BOOL count=NGEnabledFeatureCount()==3;
    [defaults setDouble:.7 forKey:NGPreferenceFontScale];[defaults setObject:@"Georgia" forKey:NGPreferenceFontName];[defaults setObject:@"bold" forKey:NGPreferencePlaybackPreset];
    NGMockLogged010=YES;NGMockUID010=@"123456";NGUsageApplyBlock(NGMockUID010,YES);NGUsageSyncAccess();
    BOOL blocked=NGBlocked && !NGFullscreenEnabled() && !NGFeatureBool(NGPreferenceDock) && !NGPlaybackElementHidden(@"danmakuEntrance") && NGTypographyScale()==1 && !NGTypographyName().length && [NGPlaybackCurrentPreset() isEqual:@"original"] && NGEnabledFeatureCount()==0;
    BOOL preserved=[defaults boolForKey:NGPreferenceDock] && [defaults boolForKey:NGPreferencePlaybackFullscreen] && [defaults doubleForKey:NGPreferenceFontScale]==.7 && [defaults boolForKey:NGPreferenceSponsorNever];
    NGBlocked=NO;NGUsageSyncAccess();BOOL cache=NGBlocked;
    NGMockUID010=@"999";NGUsageSyncAccess();BOOL different=!NGBlocked && NGFeatureBool(NGPreferenceDock);
    NGMockUID010=@"123456";NGUsageSyncAccess();NGUsageApplyBlock(NGMockUID010,NO);NGUsageSyncAccess();
    BOOL unblocked=!NGBlocked && NGFullscreenEnabled() && NGPlaybackElementHidden(@"danmakuEntrance") && fabs(NGTypographyScale()-.7)<.001 && [NGPlaybackCurrentPreset() isEqual:@"bold"];
    NGSettingsBackupController *backup=[NGSettingsBackupController new];[backup loadViewIfNeeded];[backup viewWillAppear:NO];
    BOOL backupUI=[backup tableView:backup.tableView numberOfRowsInSection:0]==2;
    BOOL preview=[NGSettingsGroups()[4][@"action"] isEqual:@"settingsBackup"];
    BOOL removed=NSClassFromString(@"NGUsageSettingsController")==nil;for(NSDictionary *group in NGSettingsGroups())for(NSDictionary *row in group[@"rows"])removed &= ![row[@"action"] isEqual:@"usagePrivacy"];
    [defaults setPersistentDomain:saved forName:bundle];NGMockLogged010=NO;NGMockUID010=nil;NGUsageSyncAccess();
    return @{@"featureCountExcludesPromptsSizesAndMigrations":@(count),@"blockedAccountStopsFeaturesWithoutDeletingPreferences":@(blocked && preserved),@"cachedBlockSurvivesProcessStateReset":@(cache),@"accountSwitchDoesNotBlockOtherUser":@(different),@"unbanRestoresStoredFeatureConfiguration":@(unblocked),@"backupKeepsTwoOriginalActions":@(backupUI),@"backupCategoryOpensDirectDestination":@(preview),@"statisticsPageAndRouteRemoved":@(removed)};
}

static void NGRunBlockedUI011(UIWindow *window,BOOL restart) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    NGSponsorTestPrompts=YES;[defaults setBool:YES forKey:NGPreferenceSponsorNever];
    if(!restart){[defaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];[defaults setBool:YES forKey:NGPreferenceDock];NGUsageApplyBlock(@"000000011",YES);[defaults synchronize];}
    NGMockLogged010=YES;NGMockUID010=@"000000011";NGUsageSyncAccess();
    NGSettingsViewController *home=[NGSettingsViewController new];UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home];window.rootViewController=nav;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        BOOL grey=NGBlocked && !home.tableView.userInteractionEnabled && home.tableView.alpha<.5 && !home.navigationItem.searchController.searchBar.userInteractionEnabled;
        BOOL alert=[home.presentedViewController isKindOfClass:UIAlertController.class] && [home.presentedViewController.title isEqual:@"已被拉黑"];
        [home tableView:home.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
        BOOL cannotNavigate=nav.viewControllers.count==1;
        BOOL count=[home.brandHeader.subtitleLabel.text containsString:@"0 项功能"];
        NSString *mode=restart ? @"blocked-restart011" : @"blocked-seed011";
        NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(grey && alert && cannotNavigate && count),@"settingsGreyAndDisabled":@(grey),@"blockedPromptVisible":@(alert),@"navigationDenied":@(cannotNavigate),@"disabledFeatureCountIsZero":@(count)});
        if(restart){NGUsageApplyBlock(NGMockUID010,NO);NGUsageSyncAccess();NGSmokeWrite(@"unblocked011.json",@{@"passed":@(!NGBlocked && home.tableView.userInteractionEnabled && home.tableView.alpha==1 && NGFeatureBool(NGPreferenceDock))});}
    });
}

static void NGRunBlockLive011(UIWindow *window) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    [defaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];[defaults setBool:YES forKey:NGPreferenceDock];[defaults setBool:YES forKey:NGPreferenceSponsorNever];
    NGUsageTestNetworking=YES;NGSponsorTestPrompts=YES;NGMockLogged010=YES;NGMockUID010=@"000000011";
    NGSettingsViewController *home=[NGSettingsViewController new];window.rootViewController=[[UINavigationController alloc] initWithRootViewController:home];
    __block BOOL blocked=NO,unblocked=NO,deleted=NO,deleting=NO;__block NSUInteger ticks=0;
    [NSTimer scheduledTimerWithTimeInterval:.5 repeats:YES block:^(NSTimer *timer){
        ticks++;
        if(NGBlocked && !home.tableView.userInteractionEnabled && home.tableView.alpha<.5 && [home.presentedViewController.title isEqual:@"已被拉黑"]){blocked=YES;NGSmokeWrite(@"block-live-stage011.json",@{@"blocked":@YES});}
        if(blocked && !NGBlocked && home.tableView.userInteractionEnabled && NGFeatureBool(NGPreferenceDock))unblocked=YES;
        if(!unblocked && ticks%4==0)NGUsageSend(YES);
        if(unblocked && !deleting && !deleted){
            deleting=YES;NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NGUsageOrigin stringByAppendingString:@"/api/guo/v1/forget"]]];
            request.HTTPMethod=@"POST";[request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];request.HTTPBody=[NSJSONSerialization dataWithJSONObject:NGUsagePayload(defaults,NGMockUID010) options:0 error:nil];request.timeoutInterval=8;
            [[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData *data,NSURLResponse *response,NSError *error){dispatch_async(dispatch_get_main_queue(),^{deleted=!error && ((NSHTTPURLResponse *)response).statusCode==200;});}] resume];
        }
        if(deleted || ticks>=70){[timer invalidate];NGSmokeWrite(@"block-live011.json",@{@"passed":@(blocked && unblocked && deleted),@"productionHTTPSBlockGreysSettingsAndShowsAlert":@(blocked),@"productionHTTPSUnbanRestoresSavedFeatures":@(unblocked),@"syntheticInstallationDeleted":@(deleted)});}
    }];
}

@interface NGSampleHold011 : UILongPressGestureRecognizer
@property(nonatomic) UIGestureRecognizerState sampledState;
@end
@implementation NGSampleHold011
- (UIGestureRecognizerState)state { return self.sampledState; }
@end
static void NGRunGestureUI011(UIWindow *window) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;[defaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];[defaults setBool:YES forKey:NGPreferenceSponsorNever];
    NGMockLogged010=NO;NGUsageSyncAccess();NGInstallSettingsGesture();
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
