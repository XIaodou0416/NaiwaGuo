static NSDictionary *NGTestSettingsData010(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;NSString *bundle=NSBundle.mainBundle.bundleIdentifier;
    NSDictionary *saved=[defaults persistentDomainForName:bundle] ?: @{};
    [defaults removePersistentDomainForName:bundle];
    [defaults setBool:YES forKey:NGPreferenceDock];[defaults setDouble:.7 forKey:NGPreferenceFontScale];
    [defaults setObject:@[@0,@0,@0,@1] forKey:NGPreferenceDockSelectedColor];
    [defaults setObject:@[@"shop"] forKey:NGPreferenceHiddenDockItems];
    [defaults setObject:@"host-private" forKey:@"host.accountToken"];
    [defaults setObject:@"fixture-installation" forKey:@"com.naiwaguo.service.installation"];[defaults setObject:@"fixture-secret" forKey:@"com.naiwaguo.service.installationSecret"];
    [defaults setObject:@[@"987654321"] forKey:@"com.naiwaguo.service.blockedIDs"];
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(12,12)];
    NSData *icon=UIImagePNGRepresentation([renderer imageWithActions:^(UIGraphicsImageRendererContext *context){[UIColor.blackColor setFill];UIRectFill(CGRectMake(0,0,12,12));}]);
    [defaults setObject:icon forKey:NGDockIconImageKey(0)];
    NSError *error=nil;NSData *data=NGCreateSettingsBackup(&error);NSDictionary *archive=NGReadSettingsBackup(data,&error);
    NSDictionary *preferences=archive[@"preferences"];
    BOOL roundtrip=archive && [preferences[NGPreferenceDock] boolValue] && [preferences[NGDockIconImageKey(0)] isEqual:icon];
    BOOL private=preferences[@"com.naiwaguo.service.installation"]==nil && preferences[@"com.naiwaguo.service.installationSecret"]==nil && preferences[@"com.naiwaguo.service.blockedIDs"]==nil && preferences[@"host.accountToken"]==nil;
    [defaults setBool:NO forKey:NGPreferenceDock];[defaults setDouble:1 forKey:NGPreferenceFontScale];[defaults setBool:YES forKey:NGPreferenceCommentGlass];
    BOOL restored=NGRestoreSettingsBackup(archive,&error) && [defaults boolForKey:NGPreferenceDock] && fabs([defaults doubleForKey:NGPreferenceFontScale]-.7)<.001 && ![defaults boolForKey:NGPreferenceCommentGlass] && [[defaults dataForKey:NGDockIconImageKey(0)] isEqual:icon];
    BOOL host=[[defaults stringForKey:@"host.accountToken"] isEqual:@"host-private"] && [[defaults stringForKey:@"com.naiwaguo.service.installationSecret"] isEqual:@"fixture-secret"] && [[defaults arrayForKey:@"com.naiwaguo.service.blockedIDs"] containsObject:@"987654321"];
    BOOL rejected=YES;
    for(NSDictionary *invalid in @[@{@"host.accountToken":@"replace"},@{@"com.naiwaguo.service.blockedIDs":@YES},@{NGPreferenceDock:@[]},@{NGPreferenceDockSelectedColor:@[@0,@0,@0,@"bad"]},@{NGPreferenceFontFile:@"../invalid.ttf"}]) {
        NSMutableDictionary *bad=[archive mutableCopy];bad[@"preferences"]=invalid;
        NSData *badData=[NSPropertyListSerialization dataWithPropertyList:bad format:NSPropertyListBinaryFormat_v1_0 options:0 error:nil];
        rejected &= NGReadSettingsBackup(badData,nil)==nil;
    }
    rejected &= NGReadSettingsBackup([@"not a backup" dataUsingEncoding:NSUTF8StringEncoding],nil)==nil;
    BOOL unchanged=[[defaults stringForKey:@"host.accountToken"] isEqual:@"host-private"] && [defaults boolForKey:NGPreferenceDock];
    BOOL diagnostics=NGPreferenceSnapshot()[@"com.naiwaguo.service.installationSecret"]==nil && NGPreferenceSnapshot()[@"com.naiwaguo.service.installation"]==nil;
    NSArray *first=NGSettingsGroups();BOOL cached=YES;for(NSUInteger i=0;i<1000;i++)cached &= NGSettingsGroups()==first;
    UIColor *light=[NGSettingsAccent() resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleLight]],*dark=[NGSettingsAccent() resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];
    CGFloat lr=0,lg=0,lb=0,dr=0,dg=0,db=0;[light getRed:&lr green:&lg blue:&lb alpha:nil];[dark getRed:&dr green:&dg blue:&db alpha:nil];BOOL monochrome=fabs(lr-lg)<.01 && fabs(lg-lb)<.01 && lr<.2 && fabs(dr-dg)<.01 && fabs(dg-db)<.01 && dr>.8;
    BOOL routes=[NGSettingsDestinationForAction(@"settingsBackup") isKindOfClass:NGSettingsBackupController.class] && NGSettingsDestinationForAction(@"usagePrivacy")==nil;
    [defaults setPersistentDomain:saved forName:bundle];
    return @{@"backupRoundTripIncludingCustomImages":@(roundtrip),@"backupExcludesAccountsBlocksAndInstallationSecrets":@(private),@"restoreReplacesOnlyPluginSettings":@(restored),@"restorePreservesHostAndAccountBlocks":@(host),@"invalidBackupsRejectedBeforeMutation":@(rejected && unchanged),@"diagnosticsExcludeServiceSecrets":@(diagnostics),@"classificationCachedAcrossRepeatedReads":@(cached),@"accentIsBlackAndWhite":@(monochrome),@"backupRouteExistsAndPrivacyRouteRemoved":@(routes)};
}
