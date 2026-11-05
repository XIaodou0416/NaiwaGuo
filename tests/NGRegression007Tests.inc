// Mocks mirror the inspected host selectors and callback order, not a server.
static BOOL NGMockLogin007=YES,NGMockDefer007,NGMockFollowSucceeds007=YES;
static NSString *NGMockAccount007=@"fixture-account";
static NSInteger NGMockRelation007,NGMockFailure007;
static NSUInteger NGMockQueries007,NGMockFollows007;
static id NGMockLastRequest007;
static NSMutableArray *NGMockPending007;
@interface NGRelationRequest007 : NSObject
@property(nonatomic,copy) NSString *relativeUserId,*relativeId;
@property(nonatomic) BOOL onlyRelationType;
@property(nonatomic) NSInteger count;
@end
@implementation NGRelationRequest007
- (void)setValue:(id)value forKey:(NSString *)key { if(NGMockFailure007==5) [NSException raise:NSInvalidArgumentException format:@"Fixture incompatible request"]; [super setValue:value forKey:key]; }
@end
@interface SSUserInfoService : NSObject
+ (instancetype)sharedInstance;
- (BOOL)isLogin;
- (NSString *)userId;
- (void)setCurUser:(id)user;
- (void)onLoginWithUser:(id)user;
@end
@implementation SSUserInfoService
+ (instancetype)sharedInstance { static id shared; static dispatch_once_t once;dispatch_once(&once,^{shared=[self new];});return shared; }
- (BOOL)isLogin{return NGMockLogin007;}
- (NSString *)userId{return NGMockAccount007;}
- (void)setCurUser:(id)user {NGMockAccount007=user; NGMockLogin007=user!=nil;}
- (void)onLoginWithUser:(id)user{[self setCurUser:user];}
@end
@interface SSShortVideoFollowUserModel : NSObject
@property(nonatomic) NSInteger followType;
- (BOOL)isFollowed;
@end
@implementation SSShortVideoFollowUserModel
- (BOOL)isFollowed { return self.followType==2 || self.followType==4; }
@end
static NSDictionary *NGMockResponse007(NSInteger relation,BOOL correctID) { return @{@"code":@0,@"data":@{@"userInfo":@{@"userId":correctID ? NGAuthorUserID : @"wrong-author",@"relationType":@(relation)}}}; }
static NSDictionary *NGMockListResponse007(NSInteger relation,BOOL correctID) { return @{@"code":@0,@"data":@{@"relationUserList":@[@{@"userId":@"unrelated",@"relationType":@2},@{@"userInfo":@{@"userId":correctID ? NGAuthorUserID : @"wrong-author",@"relationType":@(relation)}}]}}; }
@interface SSUgcApiService : NSObject
+ (id)getUserRelation:(void(^)(id))configure completion:(void(^)(id,id))completion;
@end
@implementation SSUgcApiService
+ (id)getUserRelation:(void(^)(id))configure completion:(void(^)(id,id))completion {
    NGMockQueries007++; NGRelationRequest007 *request=[NGRelationRequest007 new];configure(request); NGMockLastRequest007=request;
    if (NGMockDefer007) { if (!NGMockPending007) NGMockPending007=[NSMutableArray array]; [NGMockPending007 addObject:[completion copy]];return request; }
    id error=NGMockFailure007==1 ? [NSError errorWithDomain:@"fixture" code:1 userInfo:nil] : nil;
    id response=NGMockFailure007==4 ? @{@"code":@0,@"data":@{@"relationUserList":@42}} : (NGMockFailure007==3 ? @{@"code":@0} : NGMockResponse007(NGMockRelation007,NGMockFailure007!=2));
    completion(error,response);return request;
}
@end
@interface SSShortVideoFollowUserManager : NSObject
+ (instancetype)sharedInstance;
- (void)followUserById:(NSString *)user currentFollowType:(NSInteger)type completion:(void(^)(void))completion;
@end
@implementation SSShortVideoFollowUserManager
+ (instancetype)sharedInstance { static id shared; static dispatch_once_t once; dispatch_once(&once,^{shared=[self new];});return shared; }
- (void)followUserById:(NSString *)user currentFollowType:(NSInteger)type completion:(void(^)(void))completion { if ([user isEqual:NGAuthorUserID]) NGMockFollows007++; if (NGMockFollowSucceeds007) NGMockRelation007=2; completion(); }
@end
@interface SSShortVideoDanmakuLeftAboveView : UIView @end
@implementation SSShortVideoDanmakuLeftAboveView @end
@interface FQCShortVideoDanmakuEntranceView : UIView @end
@implementation FQCShortVideoDanmakuEntranceView @end
static void NGPump007(void) {[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.08]];}
static NSDictionary *NGTestDanmaku007(void) {
    NSString *key=@"com.naiwaguo.playback.hidden.danmakuEntrance";NSDictionary *saved=NGSavePreferences005(@[key]);NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NGInstallPlaybackTextHooks(); SSShortVideoDanmakuLeftAboveView *container=[SSShortVideoDanmakuLeftAboveView new]; FQCShortVideoDanmakuEntranceView *button=[FQCShortVideoDanmakuEntranceView new]; [container addSubview:button]; UIView *content=[UIView new];
    [d setBool:YES forKey:key];NGDecoratePlaybackTextView(container);NGDecoratePlaybackTextView(button);NGDecoratePlaybackTextView(content);
    BOOL hidden=container.hidden && button.hidden && !content.hidden;
    container.hidden=NO;button.hidden=NO;BOOL rewrites=container.hidden && button.hidden;
    [d setBool:NO forKey:key];NGDecoratePlaybackTextView(container);NGDecoratePlaybackTextView(button);BOOL restore=!container.hidden && !button.hidden;
    button.hidden=YES;NGDecoratePlaybackTextView(button);BOOL nativeHidden=button.hidden;
    BOOL setting=NO;for (NSArray *section in NGPlaybackVisibilityItems())for(NSDictionary *item in section)if([item[@"key"] isEqual:@"danmakuEntrance"])setting=[item[@"title"] containsString:@"发弹幕"];
    NGRestorePreferences005(saved);return @{@"entranceHiddenWithoutHidingDanmakuContent":@(hidden),@"disableRestoresEntrance":@(restore),@"nativeVisibilityRewritesRemainHidden":@(rewrites),@"preservesNativeHiddenState":@(nativeHidden),@"settingIsSearchable":@(setting)};
}
static NSDictionary *NGTestAuthorAccess007(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;NSArray *keys=@[NGPreferenceChannelRestoreMigration,@"com.naiwaguo.playback.hidden.header.follow",NGPreferenceDock,NGPreferenceFontScale,NGPreferenceSponsorNever,@"host.test007",@"com.naiwaguo.test007.number"];NSDictionary *saved=NGSavePreferences005(keys);
    NGInstallAccessPreferenceHooks();NGMockQueries007=0;NGMockFollows007=0;NGMockFailure007=0;NGMockRelation007=0;NGMockLogin007=YES;NGMockAccount007=@"fixture-account";NGMockDefer007=NO;
    [d setBool:YES forKey:NGPreferenceDock];[d setDouble:.7 forKey:NGPreferenceFontScale];[d setBool:YES forKey:@"host.test007"];[d setBool:YES forKey:NGPreferenceSponsorNever];[d setInteger:7 forKey:@"com.naiwaguo.test007.number"];
    NGAccessTestBypass=NO;NGMockDefer007=YES;NGAccessCheck();BOOL launchLocked=!NGAccessAllowed() && NGAccessStatus==NGAccessChecking;void(^initialCallback)(id,id)=NGMockPending007.lastObject;NGMockDefer007=NO;initialCallback(nil,NGMockResponse007(0,YES));NGPump007();
    NGRelationRequest007 *request=NGMockLastRequest007;BOOL requestTarget=[request.relativeUserId isEqual:NGAuthorUserID] && [request.relativeId isEqual:NGAuthorUserID] && request.onlyRelationType;
    BOOL mask=![d boolForKey:NGPreferenceDock] && [d objectForKey:NGPreferenceFontScale]==nil && [d integerForKey:@"com.naiwaguo.test007.number"]==0 && [d boolForKey:@"host.test007"] && [d boolForKey:NGPreferenceSponsorNever] && [d.dictionaryRepresentation[NGPreferenceDock] boolValue];
    [d setBool:YES forKey:NGPreferenceChannelRestoreMigration]; [d setBool:YES forKey:@"com.naiwaguo.playback.hidden.header.follow"]; NGRestoreLegacyTopChannels(d); BOOL migration=[d.dictionaryRepresentation[@"com.naiwaguo.playback.hidden.header.follow"] boolValue];
    BOOL noAutoFollow=NGMockFollows007==0 && NGAccessStatus==NGAccessNotFollowing;
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene];NGSettingsViewController *home=[NGSettingsViewController new];UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home];window.rootViewController=nav;[window makeKeyAndVisible];[home loadViewIfNeeded];NGPump007();
    UITableViewCell *cell=[home tableView:home.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    [home tableView:home.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];BOOL grey=!cell.userInteractionEnabled && cell.contentView.alpha<.5 && nav.viewControllers.count==1 && !home.accessBanner.hidden;
    NGMockFollowSucceeds007=YES;NGAccessFollowAuthor();NGPump007();BOOL unlock=NGAccessAllowed() && NGMockFollows007==1 && NGMockQueries007>=2 && [d boolForKey:NGPreferenceDock] && fabs([d doubleForKey:NGPreferenceFontScale]-.7)<.001;
    SSTabBarController *runtimeHost=[SSTabBarController new];window.rootViewController=runtimeHost;[runtimeHost.view layoutIfNeeded];NGRefreshDock(runtimeHost);BOOL runtimeRestore=objc_getAssociatedObject(runtimeHost,&NGDockAssociation)!=nil;
    NGMockRelation007=0;NGAccessCheck();runtimeRestore &= objc_getAssociatedObject(runtimeHost,&NGDockAssociation)==nil;
    NGMockRelation007=2;NGAccessCheck();NGRefreshDock(runtimeHost);runtimeRestore &= objc_getAssociatedObject(runtimeHost,&NGDockAssociation)!=nil;window.rootViewController=nav;
    BOOL unfollow=YES;for(NSNumber *value in @[@0,@1,@3]){NGMockRelation007=value.integerValue;NGAccessCheck();NGPump007();unfollow &= !NGAccessAllowed() && NGAccessStatus==NGAccessNotFollowing;}
    NGMockRelation007=4;NGAccessCheck();NGPump007();BOOL mutual=NGAccessAllowed();
    NGMockDefer007=YES;NGAccessCheck();void(^listCallback)(id,id)=NGMockPending007.lastObject;NGMockDefer007=NO;listCallback(nil,NGMockListResponse007(2,YES));NGPump007();BOOL listSchema=NGAccessAllowed();
    NGMockRelation007=0;NGAccessCheck();NGPump007();NGMockFollowSucceeds007=NO;NGAccessFollowAuthor();NGPump007();BOOL failedFollow=!NGAccessAllowed();
    BOOL errors=YES;for(NSNumber *failure in @[@1,@2,@3,@4,@5]){NGMockFailure007=failure.integerValue;NGAccessCheck();NGPump007();errors &= !NGAccessAllowed() && NGAccessStatus==NGAccessUnavailable;}NGMockFailure007=0;
    NGMockDefer007=YES;[NGMockPending007 removeAllObjects];NGAccessCheck();void(^oldCallback)(id,id)=NGMockPending007.lastObject;NGMockAccount007=@"new-fixture-account";NGAccessCheck();void(^currentCallback)(id,id)=NGMockPending007.lastObject;oldCallback(nil,NGMockResponse007(2,YES));NGPump007();BOOL stale=!NGAccessAllowed();currentCallback(nil,NGMockResponse007(0,YES));NGPump007();stale &= !NGAccessAllowed() && NGAccessStatus==NGAccessNotFollowing;
    NGAccessCheck();void(^timeoutCallback)(id,id)=NGMockPending007.lastObject;NGAccessExpireRequest(NGAccessGeneration);timeoutCallback(nil,NGMockResponse007(2,YES));NGPump007();BOOL timeout=!NGAccessAllowed() && NGAccessStatus==NGAccessUnavailable;
    NGMockDefer007=NO;NGMockLogin007=NO;NGAccessCheck();BOOL logout=!NGAccessAllowed() && NGAccessStatus==NGAccessUnavailable;
    ++NGAccessGeneration;NGAccessTestBypass=YES;NGMockLogin007=YES;NGMockFollowSucceeds007=YES;NGMockAccount007=@"fixture-account";[NGMockPending007 removeAllObjects];NGRestorePreferences005(saved);NGAccessSetState(NGAccessUnknown,nil);NGRefreshAfterAccessChange();window.hidden=YES;[old makeKeyAndVisible];
    return @{@"lockedStartupPreservesAlreadyMigratedChoices":@(migration),@"newSessionLocksBeforeNetworkResult":@(launchLocked),@"nativeQueryTargetsExactAuthor":@(requestTarget),@"lockedReadsMaskFeaturesWithoutDeletingPreferences":@(mask),@"noFollowMutationBeforeConfirmation":@(noAutoFollow),@"lockedFeatureListCannotNavigate":@(grey),@"confirmedFollowRequiresFreshQueryBeforeUnlock":@(unlock),@"unfollowedStatesRelock":@(unfollow),@"mutualFollowIsAccepted":@(mutual),@"relationListSchemaIsMatchedByAuthorID":@(listSchema),@"failedFollowDoesNotUnlock":@(failedFollow),@"runtimeDockLocksAndRestoresSavedChoice":@(runtimeRestore),@"networkWrongAuthorAndMalformedResponsesFailClosed":@(errors),@"staleAccountResponseCannotUnlock":@(stale),@"timedOutResponseCannotUnlockLater":@(timeout),@"logoutLocksFeatures":@(logout)};
}
static NSDictionary *NGTestSponsorPrompt007(void) {
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceSponsorNever]);NSUserDefaults *d=NSUserDefaults.standardUserDefaults;[d setBool:NO forKey:NGPreferenceSponsorNever];
    NGSponsorPromptController *prompt=[NGSponsorPromptController new];[prompt loadViewIfNeeded];
    BOOL content=[prompt.messageLabel.text isEqual:NGSponsorPromptMessage] && prompt.codeView.image && [prompt.closeButton.currentTitle isEqual:@"直接关闭"] && [prompt.neverButton.currentTitle isEqual:@"以后再也不弹"];
    [prompt closePrompt];BOOL once=![d boolForKey:NGPreferenceSponsorNever];[prompt neverPrompt];BOOL never=[d boolForKey:NGPreferenceSponsorNever];
    BOOL layout=YES;for(NSValue *size in @[[NSValue valueWithCGSize:CGSizeMake(320,568)],[NSValue valueWithCGSize:CGSizeMake(402,874)],[NSValue valueWithCGSize:CGSizeMake(768,1024)]]){prompt.view.frame=(CGRect){CGPointZero,size.CGSizeValue};[prompt.view layoutIfNeeded];CGRect close=[prompt.closeButton convertRect:prompt.closeButton.bounds toView:prompt.view];CGRect next=[prompt.neverButton convertRect:prompt.neverButton.bounds toView:prompt.view];layout &= CGRectContainsRect(prompt.view.bounds,close) && CGRectContainsRect(prompt.view.bounds,next) && prompt.codeView.contentMode==UIViewContentModeScaleAspectFit;}
    NGRestorePreferences005(saved);return @{@"exactRequestedTextCodeAndButtons":@(content),@"directCloseDoesNotPersistSuppression":@(once),@"neverAgainPersistsSuppression":@(never),@"smallPhoneAndTabletButtonsStayReachable":@(layout)};
}
static void NGRunAccessUI007(UIWindow *window,NSString *mode) {
    NGAccessTestBypass=NO;NGAccessTestPrompts=YES;NGMockLogin007=YES;NGMockAccount007=@"fixture-account";NGMockFailure007=0;NGMockFollows007=0;NGMockQueries007=0;
    BOOL preview=[mode isEqual:@"access-preview007"],restart=[mode isEqual:@"access-restart007"],seed=[mode isEqual:@"sponsor-never-seed007"];
    NGMockRelation007=preview || restart ? 0 : 2;NGMockDefer007=restart;
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;if(preview || seed)[d setBool:NO forKey:NGPreferenceSponsorNever];[d setBool:YES forKey:NGPreferenceDock];
    NGStart();NGSettingsViewController *home=[NGSettingsViewController new];UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home];window.rootViewController=nav;
    if (preview) {
        __block BOOL confirmation=NO,locked=NO,donation=NO;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,.9*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            UIAlertController *alert=(id)home.presentedViewController;confirmation=[alert isKindOfClass:UIAlertController.class] && [alert.message containsString:NGAuthorUserID] && [alert.actions.lastObject.title isEqual:@"确认关注"] && NGMockFollows007==0;
            NGSmokeImage(window,@"author-follow-prompt007.png");[home dismissViewControllerAnimated:NO completion:^{[home showSponsorPromptIfNeeded];}];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.6*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            NGSponsorPromptController *prompt=(id)home.presentedViewController;donation=[prompt isKindOfClass:NGSponsorPromptController.class] && [prompt.messageLabel.text isEqual:NGSponsorPromptMessage];NGSmokeImage(window,@"sponsor-prompt007.png");[prompt closePrompt];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2.2*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            locked=!NGAccessAllowed() && !home.accessBanner.hidden && home.tableView.visibleCells.firstObject.contentView.alpha<.5;NGSmokeImage(window,@"author-locked-settings007.png");NGAccessFollowAuthor();
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2.8*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            BOOL unlocked=NGAccessAllowed() && home.accessBanner.hidden && NGMockFollows007==1;NGSmokeImage(window,@"author-unlocked-settings007.png");NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(confirmation && donation && locked && unlocked),@"explicitConfirmationBeforeMutation":@(confirmation),@"sponsorHasRequestedContent":@(donation),@"lockedListStaysGreyAfterAnimation":@(locked),@"confirmedFollowUnlocksAfterQuery":@(unlocked)});
        });
    } else if (restart) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.0*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            BOOL locked=!NGAccessAllowed() && ![d boolForKey:NGPreferenceDock] && [d.dictionaryRepresentation[NGPreferenceDock] boolValue] && NGMockQueries007>0;
            void(^callback)(id,id)=NGMockPending007.lastObject;if(callback)callback(nil,NGMockResponse007(0,YES));
            NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(locked && !NGAccessAllowed()),@"newProcessDoesNotReuseAuthorization":@(locked),@"unfollowedRemainsLocked":@(!NGAccessAllowed())});
        });
    } else {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.1*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            if(seed){NGSponsorPromptController *prompt=(id)home.presentedViewController;if([prompt isKindOfClass:NGSponsorPromptController.class])[prompt neverPrompt];}
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.8*NSEC_PER_SEC),dispatch_get_main_queue(),^{BOOL saved=[d boolForKey:NGPreferenceSponsorNever],hidden=home.presentedViewController==nil;NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(saved && hidden && NGAccessAllowed()),@"neverAgainPreferenceRetained":@(saved),@"noSponsorPresented":@(hidden),@"authorizationIndependentFromDonation":@(NGAccessAllowed())});});
    }
}
