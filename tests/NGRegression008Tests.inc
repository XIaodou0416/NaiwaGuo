static NSDictionary *NGTestAuthorSelfAccess008(void) {
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock]);
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    [defaults setBool:YES forKey:NGPreferenceDock];
    NGAccessTestBypass=NO; NGMockLogin007=YES; NGMockAccount007=@"fixture-account";
    NGMockDefer007=YES; NGMockFailure007=0; NGMockQueries007=0; NGMockFollows007=0;
    NGAccessCheck(); void(^previousCallback)(id,id)=NGMockPending007.lastObject;
    NGMockAccount007=NGAuthorUserID; NGMockFailure007=1;
    NSUInteger queries=NGMockQueries007; NGAccessCheck(); NGAccessFollowAuthor();
    BOOL owner=NGAccessAllowed() && NGAccessStatus==NGAccessAuthor && [defaults boolForKey:NGPreferenceDock] && NGMockQueries007==queries && NGMockFollows007==0;
    previousCallback(nil,NGMockResponse007(0,YES));
    BOOL stale=NGAccessAllowed() && NGAccessStatus==NGAccessAuthor;
    __block BOOL uninterrupted=YES;
    id observer=[NSNotificationCenter.defaultCenter addObserverForName:NGAccessChangedNotification object:nil queue:nil usingBlock:^(NSNotification *note){uninterrupted &= NGAccessAllowed();}];
    NGAccessCheck(); NGAccessCheck();
    [NSNotificationCenter.defaultCenter removeObserver:observer];
    BOOL repeated=uninterrupted && NGAccessStatus==NGAccessAuthor && NGMockQueries007==queries;
    NGInstallAuthorAccessHooks();
    [SSUserInfoService.sharedInstance setCurUser:@"other-account"];
    BOOL revoked=!NGAccessAllowed() && ![defaults boolForKey:NGPreferenceDock];
    NGMockDefer007=NO; NGMockFailure007=0; NGMockRelation007=0; NGAccessCheck();
    BOOL other=!NGAccessAllowed() && NGAccessStatus==NGAccessNotFollowing && NGMockQueries007>queries;
    NGMockAccount007=[NGAuthorUserID stringByAppendingString:@"0"]; NGAccessCheck();
    BOOL exact=!NGAccessAllowed() && NGAccessStatus==NGAccessNotFollowing;
    NGMockAccount007=(id)@106173406210; NGAccessCheck();
    BOOL numeric=NGAccessAllowed() && NGAccessStatus==NGAccessAuthor;
    NGMockLogin007=NO; NGAccessCheck();
    BOOL logout=!NGAccessAllowed() && NGAccessStatus==NGAccessUnavailable;
    ++NGAccessGeneration; NGAccessTestBypass=YES; NGMockLogin007=YES; NGMockAccount007=@"fixture-account";
    NGMockFailure007=0; NGMockDefer007=NO; [NGMockPending007 removeAllObjects];
    NGRestorePreferences005(saved); NGAccessSetState(NGAccessUnknown,nil); NGRefreshAfterAccessChange();
    return @{@"authorUsesSavedFeaturesWithoutSelfFollowOrRelationNetwork":@(owner),@"staleOtherAccountResponseCannotRevokeAuthor":@(stale),@"recheckingAuthorDoesNotTemporarilyLock":@(repeated),@"accountChangeImmediatelyRevokesExemption":@(revoked),@"otherAccountStillRequiresFollow":@(other),@"authorIDMustMatchExactly":@(exact),@"nativeNumericAuthorIDIsAccepted":@(numeric),@"loggedOutAuthorIDDoesNotUnlock":@(logout)};
}

static void NGRunAuthorSelfUI008(UIWindow *window,NSString *mode) {
    BOOL restart=[mode isEqualToString:@"author-self-restart008"];
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    BOOL retained=[defaults boolForKey:NGPreferenceDock];
    if (!restart) [defaults setBool:YES forKey:NGPreferenceDock];
    [defaults setBool:YES forKey:NGPreferenceSponsorNever];
    NGAccessTestBypass=NO; NGAccessTestPrompts=YES; NGMockLogin007=YES; NGMockAccount007=NGAuthorUserID;
    NGMockFailure007=1; NGMockQueries007=0; NGMockFollows007=0;
    NGStart(); NGSettingsViewController *home=[NGSettingsViewController new];
    window.rootViewController=[[UINavigationController alloc] initWithRootViewController:home];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.3*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        BOOL unlocked=NGAccessAllowed() && NGAccessStatus==NGAccessAuthor && [defaults boolForKey:NGPreferenceDock];
        BOOL enabled=home.accessBanner.hidden && home.tableView.visibleCells.firstObject.userInteractionEnabled && home.presentedViewController==nil;
        BOOL noSelfFollow=NGMockQueries007==0 && NGMockFollows007==0;
        if (!restart) NGSmokeImage(window,@"author-self-settings008.png");
        NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(unlocked && enabled && noSelfFollow && (!restart || retained)),@"authorSettingsUnlockedWithoutFollowPrompt":@(enabled && unlocked),@"noSelfFollowOrRelationQuery":@(noSelfFollow),@"savedFeaturesAvailableAfterRestart":@(!restart || retained)});
    });
}
