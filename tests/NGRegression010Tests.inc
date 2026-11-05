static BOOL NGMockLogged010;
static NSString *NGMockUID010;
@interface TTAccount : NSObject
+ (instancetype)sharedAccount;
- (BOOL)isLogin;
- (NSString *)userIdString;
@end
@implementation TTAccount
+ (instancetype)sharedAccount { static TTAccount *account;static dispatch_once_t once;dispatch_once(&once,^{account=[self new];});return account; }
- (BOOL)isLogin { return NGMockLogged010; }
- (NSString *)userIdString { return NGMockUID010; }
@end

static NSDictionary *NGTestSettingsData010(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;NSString *bundle=NSBundle.mainBundle.bundleIdentifier;
    NSDictionary *saved=[defaults persistentDomainForName:bundle] ?: @{};
    [defaults removePersistentDomainForName:bundle];
    [defaults setBool:YES forKey:NGPreferenceDock];[defaults setDouble:.7 forKey:NGPreferenceFontScale];
    [defaults setObject:@[@0,@0,@0,@1] forKey:NGPreferenceDockSelectedColor];
    [defaults setObject:@[@"shop"] forKey:NGPreferenceHiddenDockItems];
    [defaults setObject:@"host-private" forKey:@"host.accountToken"];
    [defaults setObject:@"fixture-installation" forKey:NGUsageInstallation];[defaults setObject:@"fixture-secret" forKey:NGUsageSecret];
    [defaults setBool:YES forKey:NGUsageNoticeShown];
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(12,12)];
    NSData *icon=UIImagePNGRepresentation([renderer imageWithActions:^(UIGraphicsImageRendererContext *context){[UIColor.blackColor setFill];UIRectFill(CGRectMake(0,0,12,12));}]);
    [defaults setObject:icon forKey:NGDockIconImageKey(0)];
    NSError *error=nil;NSData *data=NGCreateSettingsBackup(&error);NSDictionary *archive=NGReadSettingsBackup(data,&error);
    NSDictionary *preferences=archive[@"preferences"];
    BOOL roundtrip=archive && [preferences[NGPreferenceDock] boolValue] && [preferences[NGDockIconImageKey(0)] isEqual:icon];
    BOOL private=preferences[NGUsageInstallation]==nil && preferences[NGUsageSecret]==nil && preferences[NGUsageNoticeShown]==nil && preferences[@"host.accountToken"]==nil;
    [defaults setBool:NO forKey:NGPreferenceDock];[defaults setDouble:1 forKey:NGPreferenceFontScale];[defaults setBool:YES forKey:NGPreferenceCommentGlass];
    BOOL restored=NGRestoreSettingsBackup(archive,&error) && [defaults boolForKey:NGPreferenceDock] && fabs([defaults doubleForKey:NGPreferenceFontScale]-.7)<.001 && ![defaults boolForKey:NGPreferenceCommentGlass] && [[defaults dataForKey:NGDockIconImageKey(0)] isEqual:icon];
    BOOL host=[[defaults stringForKey:@"host.accountToken"] isEqual:@"host-private"] && [[defaults stringForKey:NGUsageSecret] isEqual:@"fixture-secret"] && [defaults boolForKey:NGUsageNoticeShown];
    BOOL rejected=YES;
    for(NSDictionary *invalid in @[@{@"host.accountToken":@"replace"},@{NGUsageNoticeShown:@YES},@{NGPreferenceDock:@[]},@{NGPreferenceDockSelectedColor:@[@0,@0,@0,@"bad"]},@{NGPreferenceFontFile:@"../invalid.ttf"}]) {
        NSMutableDictionary *bad=[archive mutableCopy];bad[@"preferences"]=invalid;
        NSData *badData=[NSPropertyListSerialization dataWithPropertyList:bad format:NSPropertyListBinaryFormat_v1_0 options:0 error:nil];
        rejected &= NGReadSettingsBackup(badData,nil)==nil;
    }
    rejected &= NGReadSettingsBackup([@"not a backup" dataUsingEncoding:NSUTF8StringEncoding],nil)==nil;
    BOOL unchanged=[[defaults stringForKey:@"host.accountToken"] isEqual:@"host-private"] && [defaults boolForKey:NGPreferenceDock];
    NGMockLogged010=YES;NGMockUID010=@"123456789";
    BOOL identity=[NGHongguoUserID() isEqual:@"123456789"];NGMockLogged010=NO;identity &= !NGHongguoUserID().length;NGMockLogged010=YES;NGMockUID010=@"invalid";identity &= !NGHongguoUserID().length;NGMockLogged010=NO;
    NGUsageSettingsController *usage=[NGUsageSettingsController new];[usage loadViewIfNeeded];BOOL automatic=YES;
    for(NSUInteger i=0;i<2;i++){UITableViewCell *cell=[usage tableView:usage.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:0]];automatic &= ![cell.accessoryView isKindOfClass:UISwitch.class];}
    automatic &= [[usage tableView:usage.tableView titleForFooterInSection:0] containsString:@"自动"];
    [defaults removeObjectForKey:NGUsageInstallation];[defaults removeObjectForKey:NGUsageSecret];
    NSDictionary *payload=NGUsagePayload(defaults,@"123456789");BOOL payloadFields=payload.count==7 && [payload[@"schema_version"] isEqual:@1] && [payload[@"plugin_version"] isEqual:NGVersion] && [payload[@"hongguo_id"] isEqual:@"123456789"] && [payload[@"secret"] length]>=64;
    BOOL diagnostics=NGPreferenceSnapshot()[NGUsageSecret]==nil && NGPreferenceSnapshot()[NGUsageInstallation]==nil;
    NSArray *first=NGSettingsGroups();BOOL cached=YES;for(NSUInteger i=0;i<1000;i++)cached &= NGSettingsGroups()==first;
    UIColor *light=[NGSettingsAccent() resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleLight]],*dark=[NGSettingsAccent() resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];
    CGFloat lr=0,lg=0,lb=0,dr=0,dg=0,db=0;[light getRed:&lr green:&lg blue:&lb alpha:nil];[dark getRed:&dr green:&dg blue:&db alpha:nil];BOOL monochrome=fabs(lr-lg)<.01 && fabs(lg-lb)<.01 && lr<.2 && fabs(dr-dg)<.01 && fabs(dg-db)<.01 && dr>.8;
    BOOL routes=[NGSettingsDestinationForAction(@"settingsBackup") isKindOfClass:NGSettingsBackupController.class] && [NGSettingsDestinationForAction(@"usagePrivacy") isKindOfClass:NGUsageSettingsController.class];
    [defaults setPersistentDomain:saved forName:bundle];
    return @{@"backupRoundTripIncludingCustomImages":@(roundtrip),@"backupExcludesAccountsNoticeAndInstallationSecrets":@(private),@"restoreReplacesOnlyPluginSettings":@(restored),@"restorePreservesHostAndStatisticsNotice":@(host),@"invalidBackupsRejectedBeforeMutation":@(rejected && unchanged),@"accountAdapterHandlesLoginLogoutAndInvalidID":@(identity),@"automaticStatisticsHasNoUserSwitch":@(automatic),@"usagePayloadHasOnlyDeclaredFields":@(payloadFields),@"diagnosticsExcludeServiceSecrets":@(diagnostics),@"classificationCachedAcrossRepeatedReads":@(cached),@"accentIsBlackAndWhite":@(monochrome),@"backupAndPrivacyDestinationsExist":@(routes)};
}

static void NGRunUsageLive010(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;NGUsageTestNetworking=YES;
    NGMockLogged010=YES;NGMockUID010=@"000000010";NGUsageForeground();
    __block BOOL sent=NO,deleted=NO,deleting=NO;__block NSUInteger ticks=0;
    [NSTimer scheduledTimerWithTimeInterval:.25 repeats:YES block:^(NSTimer *timer){
        ticks++;
        if(!sent && [defaults doubleForKey:NGUsageLastSent]>0){sent=YES;deleting=YES;
            NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NGUsageOrigin stringByAppendingString:@"/api/guo/v1/forget"]]];request.HTTPMethod=@"POST";[request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];request.HTTPBody=[NSJSONSerialization dataWithJSONObject:NGUsagePayload(defaults,@"000000010") options:0 error:nil];request.timeoutInterval=8;
            [[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData *data,NSURLResponse *response,NSError *error){dispatch_async(dispatch_get_main_queue(),^{deleted=!error && ((NSHTTPURLResponse *)response).statusCode==200;deleting=NO;});}] resume];
        }
        if((sent && deleted && !deleting) || ticks>=100){[timer invalidate];NGSmokeWrite(@"usage-live010.json",@{@"passed":@(sent && deleted),@"automaticHTTPSUsageWithoutSwitchAccepted":@(sent),@"syntheticInstallationDeletedAfterTest":@(deleted),@"syntheticAccountOnly":@YES});}
    }];
}
