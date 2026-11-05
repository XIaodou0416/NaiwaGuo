@interface SSShortVideoDanmakuLeftAboveView : UIView @end
@implementation SSShortVideoDanmakuLeftAboveView @end
@interface FQCShortVideoDanmakuEntranceView : UIView @end
@implementation FQCShortVideoDanmakuEntranceView @end
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
static NSDictionary *NGTestSponsorPrompt007(void) {
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceSponsorNever]);NSUserDefaults *d=NSUserDefaults.standardUserDefaults;[d setBool:NO forKey:NGPreferenceSponsorNever];
    NGSponsorPromptController *prompt=[NGSponsorPromptController new];[prompt loadViewIfNeeded];
    BOOL content=[prompt.messageLabel.text isEqual:NGSponsorPromptMessage] && prompt.codeView.image && [prompt.closeButton.currentTitle isEqual:@"直接关闭"] && [prompt.neverButton.currentTitle isEqual:@"以后再也不弹"];
    [prompt closePrompt];BOOL once=![d boolForKey:NGPreferenceSponsorNever];[prompt neverPrompt];BOOL never=[d boolForKey:NGPreferenceSponsorNever];
    BOOL layout=YES;for(NSValue *size in @[[NSValue valueWithCGSize:CGSizeMake(320,568)],[NSValue valueWithCGSize:CGSizeMake(402,874)],[NSValue valueWithCGSize:CGSizeMake(768,1024)]]){prompt.view.frame=(CGRect){CGPointZero,size.CGSizeValue};[prompt.view layoutIfNeeded];CGRect close=[prompt.closeButton convertRect:prompt.closeButton.bounds toView:prompt.view];CGRect next=[prompt.neverButton convertRect:prompt.neverButton.bounds toView:prompt.view];layout &= CGRectContainsRect(prompt.view.bounds,close) && CGRectContainsRect(prompt.view.bounds,next) && prompt.codeView.contentMode==UIViewContentModeScaleAspectFit;}
    NGRestorePreferences005(saved);return @{@"exactRequestedTextCodeAndButtons":@(content),@"directCloseDoesNotPersistSuppression":@(once),@"neverAgainPersistsSuppression":@(never),@"smallPhoneAndTabletButtonsStayReachable":@(layout)};
}
