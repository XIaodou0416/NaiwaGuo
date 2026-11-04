static NSDictionary *NGTestPlaybackCorners002(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; BOOL full=[d boolForKey:NGPreferencePlaybackFullscreen],glass=[d boolForKey:NGPreferenceCollectionGlass];
    [d setBool:YES forKey:NGPreferencePlaybackFullscreen]; [d setBool:YES forKey:NGPreferenceCollectionGlass]; NGInstallPlaybackCornerHooks();
    UIView *parent=[[UIView alloc] initWithFrame:CGRectMake(0,0,402,874)]; FQVCollectionView *feed=[[FQVCollectionView alloc] initWithFrame:parent.bounds collectionViewLayout:[UICollectionViewFlowLayout new]]; [parent addSubview:feed];
    UIImage *image=[UIImage systemImageNamed:@"circle.fill"]; UIImageView *left=[[UIImageView alloc] initWithImage:image],*right=[[UIImageView alloc] initWithImage:image];
    left.frame=CGRectMake(-0.5,771.5,13,13); right.frame=CGRectMake(389.5,771.5,13,13); [parent addSubview:left]; [parent addSubview:right]; NGDecoratePlaybackCorners(parent);
    BOOL removed=left.hidden && right.hidden;
    for (NSUInteger i=0;i<100;i++) { left.hidden=NO; right.hidden=NO; NGDecoratePlaybackCorners(parent); }
    BOOL stays=left.hidden && right.hidden;
    UIButton *button=[UIButton buttonWithType:UIButtonTypeSystem]; button.frame=CGRectMake(350,680,40,40); [parent addSubview:button]; UIImageView *other=[[UIImageView alloc] initWithImage:image]; other.frame=CGRectMake(20,700,13,13); [parent addSubview:other]; NGDecoratePlaybackCorners(parent);
    BOOL safe=!button.hidden && !other.hidden && [parent hitTest:CGPointMake(360,690) withEvent:nil]==button && left.image==image;
    [d setBool:NO forKey:NGPreferencePlaybackFullscreen]; [d setBool:NO forKey:NGPreferenceCollectionGlass]; NGRefreshPlaybackCorners(); BOOL restore=!left.hidden && !right.hidden;
    [d setBool:YES forKey:NGPreferenceCollectionGlass]; NGDecoratePlaybackCorners(parent); UIView *elsewhere=[UIView new]; [elsewhere addSubview:left]; BOOL moved=!left.hidden && left.image==image;
    [d setBool:full forKey:NGPreferencePlaybackFullscreen]; [d setBool:glass forKey:NGPreferenceCollectionGlass];
    return @{@"capturedPairRemoved":@(removed),@"hostShowAndRelayoutStayHidden":@(stays),@"controlsImagesAndHitTestsPreserved":@(safe),@"disablingBothRestoresLatestVisibility":@(restore),@"reparentRestoresImage":@(moved)};
}
static UIImage *NGTestBackdropImage(UIColor *color) { return [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(100,100)] imageWithActions:^(UIGraphicsImageRendererContext *context){ [color setFill]; UIRectFill(CGRectMake(0,0,100,100)); }]; }
static NSDictionary *NGTestShareRepaint002(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; BOOL previous=[d boolForKey:NGPreferenceShareGlass]; [d setBool:YES forKey:NGPreferenceShareGlass];
    Class draw=NSClassFromString(@"DanceUI._UIGraphicsView"); if (!draw) { draw=objc_allocateClassPair(UIView.class,"DanceUI._UIGraphicsView",0); objc_registerClassPair(draw); }
    SSUGKmpSharePanelContainerViewController *controller=[SSUGKmpSharePanelContainerViewController new]; [controller loadViewIfNeeded]; controller.view.frame=CGRectMake(0,0,402,294);
    UIView *backing=[[draw alloc] initWithFrame:controller.view.bounds]; backing.backgroundColor=UIColor.whiteColor; UIImage *flat=NGTestBackdropImage(UIColor.whiteColor); backing.layer.contents=(__bridge id)flat.CGImage; [controller.view addSubview:backing];
    UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(16,20,100,24)]; label.text=@"分享至"; [controller.view addSubview:label];
    UIButton *button=[UIButton buttonWithType:UIButtonTypeSystem]; button.frame=CGRectMake(20,70,52,52); [controller.view addSubview:button];
    UIImageView *icon=[[UIImageView alloc] initWithImage:flat]; icon.frame=CGRectMake(90,70,52,52); [controller.view addSubview:icon];
    UIVisualEffectView *native=[[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleLight]]; native.frame=controller.view.bounds; native.userInteractionEnabled=NO; [controller.view insertSubview:native atIndex:0];
    NGInstallCommentGlassHooks(); NGDecorateShareController(controller);
    UIVisualEffectView *material=objc_getAssociatedObject(controller.view,&NGCommentGlassAssociation);
    BOOL first=material && backing.layer.contents==nil && native.effect==nil && CGColorGetAlpha(backing.layer.backgroundColor)==0;
    [material removeFromSuperview]; [controller.view addSubview:[UIView new]];
    BOOL reinserted=material.superview==controller.view;
    material.effect=nil; material.hidden=YES; material.alpha=0; NGDecorateShareController(controller);
    BOOL materialRestored=material.effect && !material.hidden && material.alpha==1;
    UIColor *latest=[UIColor colorWithWhite:0.93 alpha:1]; UIVisualEffect *latestEffect=[UIBlurEffect effectWithStyle:UIBlurEffectStyleDark];
    for (NSUInteger i=0;i<100;i++) { backing.backgroundColor=UIColor.whiteColor; backing.layer.backgroundColor=latest.CGColor; backing.layer.contents=(__bridge id)flat.CGImage; native.effect=latestEffect; }
    BOOL sticky=backing.layer.contents==nil && CGColorGetAlpha(backing.layer.backgroundColor)==0 && native.effect==nil && objc_getAssociatedObject(controller.view,&NGCommentGlassAssociation)==material;
    UIImage *colored=NGTestBackdropImage(UIColor.systemRedColor); backing.layer.contents=(__bridge id)colored.CGImage; BOOL contentSafe=backing.layer.contents==(__bridge id)colored.CGImage && icon.image==flat && [label.text isEqualToString:@"分享至"] && [controller.view hitTest:CGPointMake(40,90) withEvent:nil]==button;
    backing.layer.contents=(__bridge id)flat.CGImage;
    UIView *late=[[draw alloc] initWithFrame:CGRectZero]; [controller.view insertSubview:late atIndex:1]; late.frame=controller.view.bounds; late.backgroundColor=UIColor.whiteColor; [late setNeedsLayout]; [late layoutIfNeeded]; late.layer.backgroundColor=UIColor.whiteColor.CGColor;
    BOOL lateSafe=CGColorGetAlpha(late.layer.backgroundColor)==0;
    CALayer *unrelated=[CALayer layer]; unrelated.backgroundColor=latest.CGColor; unrelated.contents=(__bridge id)flat.CGImage;
    [d setBool:NO forKey:NGPreferenceShareGlass]; NGDecorateShareController(controller);
    BOOL restores=CGColorEqualToColor(backing.layer.backgroundColor,latest.CGColor) && backing.layer.contents==(__bridge id)flat.CGImage && native.effect==latestEffect;
    [d setBool:previous forKey:NGPreferenceShareGlass];
    return @{@"removedMaterialReinsertedOnHostRebuild":@(reinserted),@"materialEffectAndVisibilityRecovered":@(materialRestored),@"initialLayerAndDrawingBackplatesCleared":@(first),@"hundredLayerAndEffectRepaintsRemainGlass":@(sticky),@"textIconsAndNativeHitTargetsPreserved":@(contentSafe),@"zeroSizeInsertedDrawingRefreshesAfterLayout":@(lateSafe),@"latestLayerContentsColorAndEffectRestore":@(restores),@"unrelatedLayersUnchanged":@(CGColorEqualToColor(unrelated.backgroundColor,latest.CGColor) && unrelated.contents==(__bridge id)flat.CGImage)};
}
// Generate a private, temporary renamed copy for an actually new registration.
// The system font binary is never included in the source or release archives.
static uint32_t NGTestBE32(const uint8_t *p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void NGTestPut32(uint8_t *p,uint32_t n) { p[0]=n>>24; p[1]=n>>16; p[2]=n>>8; p[3]=n; }
static uint32_t NGTestFontChecksum(const uint8_t *p,NSUInteger length) { uint32_t sum=0; for (NSUInteger i=0;i<length;i+=4) { uint32_t value=0; for (NSUInteger j=0;j<4;j++) value=(value<<8)|(i+j<length ? p[i+j] : 0); sum+=value; } return sum; }
static NSURL *NGTestUniqueFontURL(NSURL *source) {
    NSMutableData *data=[[NSData dataWithContentsOfURL:source] mutableCopy]; if (data.length<12) return nil;
    uint8_t *p=data.mutableBytes; NSUInteger count=(p[4]<<8)|p[5],head=0; if (12+count*16>data.length) return nil;
    for (NSUInteger i=0;i<count;i++) {
        uint8_t *record=p+12+i*16; NSUInteger offset=NGTestBE32(record+8),length=NGTestBE32(record+12); if (offset+length>data.length) return nil;
        if (!memcmp(record,"head",4)) head=offset;
        if (!memcmp(record,"name",4)) {
            const uint8_t original[]={0,'G',0,'e',0,'o',0,'r',0,'g',0,'i',0,'a'};
            const uint8_t replaced[]={0,'N',0,'G',0,'T',0,'e',0,'s',0,'t',0,'1'};
            for (NSUInteger j=0;j+14<=length;j++) if (!memcmp(p+offset+j,original,14)) memcpy(p+offset+j,replaced,14);
            for (NSUInteger j=0;j+7<=length;j++) if (!memcmp(p+offset+j,"Georgia",7)) memcpy(p+offset+j,"NGTest1",7);
        }
    }
    if (!head || head+12>data.length) return nil; NGTestPut32(p+head+8,0);
    for (NSUInteger i=0;i<count;i++) { uint8_t *record=p+12+i*16; NGTestPut32(record+4,NGTestFontChecksum(p+NGTestBE32(record+8),NGTestBE32(record+12))); }
    NGTestPut32(p+head+8,0xB1B0AFBA-NGTestFontChecksum(p,data.length));
    NSURL *url=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"NaiwaGuo-unique-test-font.ttf"]]; return [data writeToURL:url atomically:YES] ? url : nil;
}
static NSDictionary *NGTestTypography002(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSArray *keys=@[NGPreferenceFontScale,NGPreferenceFontName,NGPreferenceFontFile]; NSMutableDictionary *saved=[NSMutableDictionary dictionary]; for (NSString *key in keys) { saved[key]=[d objectForKey:key] ?: NSNull.null; [d removeObjectForKey:key]; } NGRefreshTypography();
    UIWindow *previous=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:previous.windowScene]; UIViewController *root=[UIViewController new]; window.rootViewController=root; [window makeKeyAndVisible];
    UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(10,60,300,40)]; label.text=@"字号与字体 Aa"; label.font=[UIFont systemFontOfSize:20]; [root.view addSubview:label];
    UITextField *field=[[UITextField alloc] initWithFrame:CGRectMake(10,110,300,40)]; field.font=[UIFont systemFontOfSize:18]; field.text=@"输入框"; field.attributedPlaceholder=[[NSAttributedString alloc] initWithString:@"占位" attributes:@{NSFontAttributeName:[UIFont italicSystemFontOfSize:16]}]; [root.view addSubview:field];
    UITextView *editor=[[UITextView alloc] initWithFrame:CGRectMake(10,160,300,70)]; editor.font=[UIFont systemFontOfSize:22]; editor.text=@"编辑文字"; [root.view addSubview:editor];
    UILabel *rich=[[UILabel alloc] initWithFrame:CGRectMake(10,240,300,50)]; NSMutableAttributedString *source=[[NSMutableAttributedString alloc] initWithString:@"标题正文" attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:12],NSForegroundColorAttributeName:UIColor.systemBlueColor}]; [source addAttribute:NSFontAttributeName value:[UIFont boldSystemFontOfSize:20] range:NSMakeRange(0,2)]; rich.attributedText=source; [root.view addSubview:rich];
    [d setDouble:0.8 forKey:NGPreferenceFontScale]; NGRefreshTypography();
    BOOL scale=fabs(label.font.pointSize-16)<0.01 && fabs(field.font.pointSize-14.4)<0.01 && fabs(editor.font.pointSize-17.6)<0.01;
    UIFont *large=[rich.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil],*small=[rich.attributedText attribute:NSFontAttributeName atIndex:3 effectiveRange:nil];
    BOOL spans=fabs(large.pointSize-16)<0.01 && fabs(small.pointSize-9.6)<0.01 && [[rich.attributedText attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:nil] isEqual:UIColor.systemBlueColor];
    for (NSUInteger i=0;i<100;i++) { label.font=[UIFont systemFontOfSize:20]; label.text=@"复用文本"; rich.attributedText=source; [label setNeedsLayout]; [label layoutIfNeeded]; }
    BOOL noAccumulation=fabs(label.font.pointSize-16)<0.01 && fabs(((UIFont *)[rich.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil]).pointSize-16)<0.01;
    UILabel *newLabel=[UILabel new]; newLabel.font=[UIFont systemFontOfSize:30]; newLabel.text=@"新文字"; [root.view addSubview:newLabel]; NGApplyTypography(newLabel); BOOL newViews=fabs(newLabel.font.pointSize-24)<0.01;
    [d setObject:@"Georgia" forKey:NGPreferenceFontName]; NGRefreshTypography(); BOOL custom=[label.font.familyName containsString:@"Georgia"] && fabs(label.font.pointSize-16)<0.01;
    [d setDouble:1 forKey:NGPreferenceFontScale]; [d removeObjectForKey:NGPreferenceFontName]; NGRefreshTypography(); BOOL restore=fabs(label.font.pointSize-20)<0.01 && fabs(field.font.pointSize-18)<0.01 && fabs(editor.font.pointSize-22)<0.01 && [rich.attributedText isEqualToAttributedString:source];
    CTFontRef ct=CTFontCreateWithName(CFSTR("Georgia"),16,NULL); NSURL *fontURL=CFBridgingRelease(CTFontCopyAttribute(ct,kCTFontURLAttribute)); CFRelease(ct);
    NSURL *uniqueFont=NGTestUniqueFontURL(fontURL); NSError *importError=nil; BOOL imported=uniqueFont && NGImportTypographyFont(uniqueFont,&importError); NSString *importedName=NGTypographyName(); NSString *importedFile=[d stringForKey:NGPreferenceFontFile]; NGLoadTypographyFont();
    BOOL duplicate=imported && NGTypographyRegisterURL([NGTypographyDirectory() URLByAppendingPathComponent:importedFile],nil);
    if (uniqueFont) [NSFileManager.defaultManager removeItemAtURL:uniqueFont error:nil];
    BOOL persisted=imported && duplicate && [importedName isEqualToString:@"NGTest1"] && [NSFileManager.defaultManager fileExistsAtPath:[NGTypographyDirectory() URLByAppendingPathComponent:importedFile].path] && [UIFont fontWithName:importedName size:16];
    NSURL *invalid=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"NaiwaGuo-invalid-font.ttf"]]; [@"this is not a font" writeToURL:invalid atomically:YES encoding:NSUTF8StringEncoding error:nil]; NSError *error=nil;
    BOOL rejects=!NGImportTypographyFont(invalid,&error) && error && [NGTypographyName() isEqualToString:importedName]; [NSFileManager.defaultManager removeItemAtURL:invalid error:nil];
    NGTypographyController *settings=[NGTypographyController new]; [settings loadViewIfNeeded]; [settings tableView:settings.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:3 inSection:2]];
    BOOL reset=NGTypographyScale()==1 && !NGTypographyName().length && fabs(label.font.pointSize-20)<0.01;
    if (importedFile.length) [NSFileManager.defaultManager removeItemAtURL:[NGTypographyDirectory() URLByAppendingPathComponent:importedFile] error:nil];
    for (NSString *key in keys) { if (saved[key]==NSNull.null) [d removeObjectForKey:key]; else [d setObject:saved[key] forKey:key]; } NGRefreshTypography(); window.hidden=YES; [previous makeKeyAndVisible];
    return @{@"labelFieldAndEditorScale":@(scale),@"mixedAttributedFontsAndColorsPreserved":@(spans),@"hundredWritesDoNotCompound":@(noAccumulation),@"futureViewsPickUpScale":@(newViews),@"customTypefaceApplies":@(custom),@"defaultRestoresOriginalFontsAndSpans":@(restore),@"validFontImportAndReregistration":@(persisted),@"invalidFontKeepsPreviousSelection":@(rejects),@"resetControlRestoresDefaults":@(reset)};
}

static NSDictionary *NGTestSearch002(void) {
    NSDictionary *destinations=@{@"typography":NGTypographyController.class,@"sponsor":NGSupportViewController.class,@"community":NGCommunityViewController.class,@"dockItems":NGDockItemsSettingsController.class,@"dockColors":NGDockColorSettingsController.class,@"shareGlassStyle":NGShareGlassSettingsController.class,@"dockIcons":NGDockIconSettingsController.class,@"playbackIconPresets":NGPlaybackPresetSettingsController.class,@"playbackVisibility":NGPlaybackVisibilitySettingsController.class,@"playbackLayout":NGPlaybackLayoutSettingsController.class,@"commentGlassOpacity":NGCommentGlassOpacityController.class};
    BOOL mapped=YES; for (NSString *action in destinations) mapped &= [NGSettingsDestinationForAction(action) isKindOfClass:destinations[action]];
    BOOL safe=YES; for (NSString *action in @[@"clearEvents",@"capture",@"applyPlaybackGlass",@"qqGroup",@"tgNews",@"tgChat",@"unknown"]) safe &= NGSettingsDestinationForAction(action)==nil;
    NGSettingsViewController *home=[NGSettingsViewController new]; UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home]; [home loadViewIfNeeded];
    BOOL wired=home.navigationItem.searchController.delegate==home;
    BOOL previous=[NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackFullscreen];
    NSDictionary *row=home.groups[0][@"rows"][2]; home.pendingSearchMatch=@{@"group":@0,@"row":row};
    [home openPendingSearchResult];
    NGSettingsGroupViewController *group=(id)nav.topViewController; [group loadViewIfNeeded];
    BOOL located=[group isKindOfClass:NGSettingsGroupViewController.class] && group.groupIndex==0 && [group.highlightSetting isEqualToString:NGPreferencePlaybackFullscreen];
    BOOL unchanged=previous==[NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackFullscreen];
    UITableViewCell *cell=[group tableView:group.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];
    [group tableView:group.tableView willDisplayCell:cell forRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];
    BOOL highlighted=cell.backgroundConfiguration.strokeWidth==1 && cell.backgroundConfiguration.strokeColor!=nil;
    [home openPendingSearchResult]; BOOL once=nav.viewControllers.count==2 && home.pendingSearchMatch==nil;
    NGTypographyController *fonts=[NGTypographyController new]; [fonts loadViewIfNeeded];
    return @{@"allTenDedicatedPagesMapped":@(mapped),@"externalAndDestructiveActionsOnlyLocate":@(safe),@"searchDismissalDelegateConnected":@(wired),@"toggleRoutesToCorrectGroupAndKey":@(located),@"searchDoesNotTogglePreference":@(unchanged),@"locatedRowHighlightSurvivesSharedStyle":@(highlighted),@"pendingNavigationConsumedOnce":@(once),@"multilineFootersUseAutomaticHeight":@(fonts.tableView.sectionFooterHeight==UITableViewAutomaticDimension)};
}
