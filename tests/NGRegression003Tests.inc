@interface NGCopyingTextLabel : UILabel
@property(nonatomic) NSUInteger textWrites,fontWrites,layouts;
@end
@implementation NGCopyingTextLabel
- (NSAttributedString *)attributedText { return [[super attributedText] mutableCopy]; }
- (void)setAttributedText:(NSAttributedString *)text { self.textWrites++; [super setAttributedText:text]; [self setNeedsLayout]; }
- (void)setFont:(UIFont *)font { self.fontWrites++; [super setFont:font]; }
- (void)layoutSubviews { self.layouts++; [super layoutSubviews]; }
@end
@interface NGNestedFontField : UITextField
@property(nonatomic,strong) UILabel *renderLabel;
@end
@implementation NGNestedFontField
- (instancetype)initWithFrame:(CGRect)frame { if ((self=[super initWithFrame:frame])) { _renderLabel=[UILabel new]; [self addSubview:_renderLabel]; } return self; }
- (void)setFont:(UIFont *)font { [super setFont:font]; self.renderLabel.font=font; }
@end
@interface SSVideoSeriesFeedViewController : UIViewController
@property(nonatomic) NSUInteger searchInvocations;
@end
@implementation SSVideoSeriesFeedViewController
- (void)simpleSearchViewDidClicked { self.searchInvocations++; }
@end
static NSDictionary *NGTestTypography003(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; id scale=[d objectForKey:NGPreferenceFontScale],name=[d objectForKey:NGPreferenceFontName];
    [d setDouble:0.8 forKey:NGPreferenceFontScale]; [d setObject:@"Georgia" forKey:NGPreferenceFontName]; NGRefreshTypography();
    UIWindow *old=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:old.windowScene]; UIViewController *page=[UIViewController new]; window.rootViewController=page; [window makeKeyAndVisible];
    NGCopyingTextLabel *label=[[NGCopyingTextLabel alloc] initWithFrame:CGRectMake(10,100,300,50)]; label.font=[UIFont systemFontOfSize:20]; label.attributedText=[[NSAttributedString alloc] initWithString:@"copying native text" attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:20]}]; [page.view addSubview:label]; NGApplyTypography(label);
    NSUInteger before=label.textWrites+label.fontWrites; CFTimeInterval start=CACurrentMediaTime();
    for (NSUInteger i=0;i<300;i++) { [label setNeedsLayout]; [label layoutIfNeeded]; NGApplyTypography(label); }
    BOOL bounded=label.textWrites+label.fontWrites==before && CACurrentMediaTime()-start<2;
    UIFont *font=[label.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil]; BOOL transformed=fabs(font.pointSize-16)<0.01 && [font.familyName containsString:@"Georgia"];
    NGNestedFontField *field=[[NGNestedFontField alloc] initWithFrame:CGRectMake(10,160,300,50)]; field.font=[UIFont systemFontOfSize:20]; [page.view addSubview:field]; NGApplyTypography(field); NGApplyTypography(field.renderLabel);
    BOOL once=fabs(field.font.pointSize-16)<0.01 && fabs(field.renderLabel.font.pointSize-16)<0.01;
    for (NSUInteger i=0;i<100;i++) label.font=[UIFont systemFontOfSize:20]; BOOL stable=fabs(label.font.pointSize-16)<0.01;
    [d removeObjectForKey:NGPreferenceFontScale]; [d removeObjectForKey:NGPreferenceFontName]; NGRefreshTypography(); BOOL restore=fabs(label.font.pointSize-20)<0.01;
    if (scale) [d setObject:scale forKey:NGPreferenceFontScale]; if (name) [d setObject:name forKey:NGPreferenceFontName]; NGRefreshTypography(); window.hidden=YES; [old makeKeyAndVisible];
    return @{@"freshAttributedCopiesDoNotWriteDuring300Layouts":@(bounded),@"customFontAndScaleStillApply":@(transformed),@"internalTextRendererNotScaledTwice":@(once),@"hundredNativeFontWritesStayStable":@(stable),@"restoreDefaultAfterStress":@(restore)};
}
static NSDictionary *NGTestCorners003(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; BOOL full=[d boolForKey:NGPreferencePlaybackFullscreen],glass=[d boolForKey:NGPreferenceCollectionGlass]; [d setBool:NO forKey:NGPreferencePlaybackFullscreen]; [d setBool:YES forKey:NGPreferenceCollectionGlass];
    UIView *root=[[UIView alloc] initWithFrame:CGRectMake(0,0,402,874)],*wrapper=[[UIView alloc] initWithFrame:root.bounds],*decoration=[[UIView alloc] initWithFrame:root.bounds]; [root addSubview:wrapper]; [wrapper addSubview:[[FQVShortVideoPlayerCellView alloc] initWithFrame:root.bounds]]; [root addSubview:decoration];
    UIImageView *left=[UIImageView new],*right=[UIImageView new]; [decoration addSubview:left]; [decoration addSubview:right]; left.frame=CGRectMake(-0.5,771.5,13,13); right.frame=CGRectMake(389.5,771.5,13,13);
    UIImage *image=[UIImage systemImageNamed:@"circle.fill"]; left.image=image; right.image=image; [left setNeedsLayout]; [left layoutIfNeeded]; [right setNeedsLayout]; [right layoutIfNeeded];
    BOOL nested=left.hidden && right.hidden;
    for (NSUInteger i=0;i<100;i++) { left.hidden=NO; right.hidden=NO; left.image=image; right.image=image; } BOOL rewrites=left.hidden && right.hidden;
    [d setBool:NO forKey:NGPreferenceCollectionGlass]; NGRefreshPlaybackCorners(); BOOL restore=!left.hidden && !right.hidden;
    UIView *plain=[[UIView alloc] initWithFrame:root.bounds]; [plain addSubview:left]; [plain addSubview:right]; [d setBool:YES forKey:NGPreferenceCollectionGlass]; [left setNeedsLayout]; [left layoutIfNeeded]; BOOL safe=!left.hidden && !right.hidden;
    [d setBool:full forKey:NGPreferencePlaybackFullscreen]; [d setBool:glass forKey:NGPreferenceCollectionGlass];
    return @{@"collectionNestedPairFoundAfterLateImagesAndFrames":@(nested),@"hundredHostRewritesRemainHidden":@(rewrites),@"disableRestoresWithoutFullscreen":@(restore),@"nonPlaybackReparentIsUntouched":@(safe)};
}
static NSDictionary *NGTestIconStates003(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; id old=[d objectForKey:NGPreferencePlaybackPreset]; BOOL all=YES,revert=YES,latest=YES;
    for (NSString *preset in @[@"soft",@"bold",@"badge",@"duotone"]) for (NSUInteger index=0;index<=2;index+=2) {
        [d setObject:preset forKey:NGPreferencePlaybackPreset]; UIView *action=index==0 ? [SSShortVideoRightInteractionCollectView new] : [SSShortVideoRightInteractionLikeView new];
        id model=index==0 ? [SSShortVideoRightInteractionCollectModel new] : [SSShortVideoRightInteractionLikeModel new]; [(id)action setModel:model]; NGApplyPlaybackPreset(action,index);
        UIImageView *icon=(id)[action viewWithTag:100]; NSData *normal=UIImagePNGRepresentation(icon.image); SEL setter=NSSelectorFromString(index==0 ? @"setIsCollected:" : @"setIsLiked:");
        ((void(*)(id,SEL,BOOL))[model methodForSelector:setter])(model,setter,YES); NSData *selected=UIImagePNGRepresentation(icon.image);
        all &= ![normal isEqualToData:selected] && ((NGPlaybackPresetState *)objc_getAssociatedObject(action,&NGPlaybackPresetAssociation)).lastSelected;
        ((void(*)(id,SEL,BOOL))[model methodForSelector:setter])(model,setter,NO); revert &= [normal isEqualToData:UIImagePNGRepresentation(icon.image)];
        id next=index==0 ? [SSShortVideoRightInteractionCollectModel new] : [SSShortVideoRightInteractionLikeModel new]; [(id)action setModel:next]; ((void(*)(id,SEL,BOOL))[model methodForSelector:setter])(model,setter,YES);
        latest &= !((NGPlaybackPresetState *)objc_getAssociatedObject(action,&NGPlaybackPresetAssociation)).lastSelected;
    }
    if (old) [d setObject:old forKey:NGPreferencePlaybackPreset]; else [d removeObjectForKey:NGPreferencePlaybackPreset];
    return @{@"allFourPresetsReactToNativeLikeAndCollectModels":@(all),@"unlikeAndUncollectRestoreNormalImmediately":@(revert),@"reusedCellIgnoresOldModelCallbacks":@(latest)};
}
static NSDictionary *NGTestDock003(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSArray *keys=@[NGPreferenceDock,NGPreferenceDockClear,NGPreferenceDockCollapse,NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize,NGPreferenceDockContent]; NSMutableDictionary *saved=[NSMutableDictionary dictionary]; for (NSString *key in keys) { saved[key]=[d objectForKey:key] ?: NSNull.null; [d removeObjectForKey:key]; } [d setBool:YES forKey:NGPreferenceDock];
    UIWindow *previous=NGBaseWindow(),*window=[[UIWindow alloc] initWithWindowScene:previous.windowScene]; SSTabBarController *host=[SSTabBarController new]; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; NGRefreshDock(host); NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); [dock.view layoutIfNeeded];
    CGRect capsule=[dock.platter convertRect:dock.platter.bounds toView:dock.view]; BOOL native=dock.platter && dock.view.superview==host.customBar && !dock.systemHost;
    BOOL geometry=fabs(CGRectGetMinX(dock.searchKey.frame)-CGRectGetMaxX(capsule)-12)<1 && fabs(dock.searchKey.bounds.size.height-capsule.size.height)<1;
    [d setBool:YES forKey:NGPreferenceDockClear]; [dock layoutGlass]; BOOL clear=objc_getAssociatedObject(dock.platter,&NGDockAppliedGlassKey)!=nil;
    [d setBool:NO forKey:NGPreferenceDockClear]; [dock layoutGlass]; BOOL regular=objc_getAssociatedObject(dock.platter,&NGDockAppliedGlassKey)==nil;
    [d setDouble:80 forKey:NGPreferenceDockHeight]; [d setDouble:85 forKey:NGPreferenceDockLength]; [d setDouble:70 forKey:NGPreferenceDockSearchSize]; [dock layoutGlass]; BOOL sizes=fabs(dock.tabBar.transform.a-0.8)<0.01 && fabs(dock.searchKey.bounds.size.width-dock.metrics.height*0.7)<1;
    [d setInteger:2 forKey:NGPreferenceDockCollapse]; [dock layoutGlass]; [dock.systemHost.view layoutIfNeeded]; [dock layoutGlass]; BOOL collapse=dock.systemHost && dock.collapsePlatter && !dock.collapsePan.cancelsTouchesInView && !dock.collapsePan.delaysTouchesBegan;
    [dock setCollapsed:YES]; NGRefreshDock(host); UIImage *circle=dock.viewControllers[dock.selectedIndex].tabBarItem.image;
    BOOL circleIcon=dock.collapsed && fabs(circle.size.width-26)<0.01 && fabs(circle.size.height-26)<0.01;
    [dock setCollapsed:NO]; NGRefreshDock(host);
    BOOL reference=fabs(NGDockCapsuleWidth(NGDockFallbackMetrics,4,4,402,1)-286)<0.01 && NGDockCapsuleWidth(NGDockFallbackMetrics,3,4,402,1)<286;
    [d setBool:NO forKey:NGPreferenceDock]; NGRefreshDock(host); BOOL restore=!objc_getAssociatedObject(host,&NGDockAssociation) && !host.customBar.hidden && NGDockOriginalButtons(host.customBar).firstObject.layer.opacity==1 && NGDockOriginalButtons(host.customBar).firstObject.userInteractionEnabled;
    window.hidden=YES; [previous makeKeyAndVisible]; for (NSString *key in keys) { if (saved[key]==NSNull.null) [d removeObjectForKey:key]; else [d setObject:saved[key] forKey:key]; }
    return @{@"collapsedCurrentPageIconSurvivesHostRefresh":@(circleIcon),@"standaloneNativePlatterInsideHostBar":@(native),@"upstreamTwelvePointGapAndEqualCircleHeight":@(geometry),@"clearWritesNativePlatterGlass":@(clear),@"regularRestoresOriginalMaterial":@(regular),@"independentSizeControlsApply":@(sizes),@"collapseUsesUIKitControllerAndNonblockingPan":@(collapse),@"geometryMatchesUpstreamReferenceFormula":@(reference),@"disableRestoresNativeContentAndHits":@(restore)};
}

@interface NGFontStressController : UITableViewController @end
@implementation NGFontStressController
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 300; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    NGCopyingTextLabel *label=[NGCopyingTextLabel new]; label.font=[UIFont systemFontOfSize:18]; label.numberOfLines=0; label.attributedText=[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"我的页面 · 字体压力检查 %ld\nNative text keeps its layout stable.",(long)path.row] attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:18]}]; label.translatesAutoresizingMaskIntoConstraints=NO; [cell.contentView addSubview:label];
    [NSLayoutConstraint activateConstraints:@[[label.leadingAnchor constraintEqualToAnchor:cell.contentView.leadingAnchor constant:16],[label.trailingAnchor constraintEqualToAnchor:cell.contentView.trailingAnchor constant:-16],[label.topAnchor constraintEqualToAnchor:cell.contentView.topAnchor constant:8],[label.bottomAnchor constraintEqualToAnchor:cell.contentView.bottomAnchor constant:-8]]]; return cell;
}
@end
static void NGSmokeWrite(NSString *name,NSDictionary *value) {
    NSString *documents=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
    [[NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted error:nil] writeToFile:[documents stringByAppendingPathComponent:name] atomically:YES];
}
static void NGSmokeImage(UIWindow *window,NSString *name) {
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithBounds:window.bounds]; UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){ [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES]; }];
    NSString *documents=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject; [UIImagePNGRepresentation(image) writeToFile:[documents stringByAppendingPathComponent:name] atomically:YES];
}
