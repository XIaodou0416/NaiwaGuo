@interface FQVShortVideoBaseInsertView : UIView @end
@implementation FQVShortVideoBaseInsertView @end
@interface SSShortVideoBottomDistributionView : UIView @end
@implementation SSShortVideoBottomDistributionView @end
@interface SSShortVideoDistributionContentView : UIView @end
@implementation SSShortVideoDistributionContentView @end
@interface FQCShortPlaySeriesCommentCell : UICollectionViewCell @end
@implementation FQCShortPlaySeriesCommentCell @end
@interface FQVShortVideoGradientLayerView : UIView @end
@implementation FQVShortVideoGradientLayerView @end
@interface SSMine737KMPRootView : UIView @end
@implementation SSMine737KMPRootView @end
@interface FQMSEcomXTabMallViewController : UIViewController @end
@implementation FQMSEcomXTabMallViewController @end

@interface NG075Gallery : NSObject <UICollectionViewDataSource> @end
@implementation NG075Gallery
- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section { return 30; }
- (UICollectionViewCell *)collectionView:(UICollectionView *)view cellForItemAtIndexPath:(NSIndexPath *)path {
    UICollectionViewCell *cell = [view dequeueReusableCellWithReuseIdentifier:@"gallery" forIndexPath:path];
    cell.contentView.backgroundColor = [UIColor colorWithHue:(path.item % 9) / 9.0 saturation:0.55 brightness:0.88 alpha:1];
    UILabel *title = [cell.contentView viewWithTag:7501];
    if (!title) { title = [UILabel new]; title.tag=7501; title.textColor=UIColor.whiteColor; title.numberOfLines=2; title.textAlignment=NSTextAlignmentCenter; [cell.contentView addSubview:title]; }
    title.frame=cell.bounds; title.text=[NSString stringWithFormat:@"模拟内容\n%ld",(long)path.item+1];
    cell.layer.cornerRadius=12; cell.clipsToBounds=YES; return cell;
}
@end

static NSDictionary *NGTestCollection075(void) {
    NSUserDefaults *d = NSUserDefaults.standardUserDefaults;
    NSString *hideKey = @"com.naiwaguo.playback.hidden.text.collection";
    id oldHide = [d objectForKey:hideKey], oldGlass = [d objectForKey:NGPreferenceCollectionGlass];
    [d setBool:NO forKey:hideKey]; [d setBool:YES forKey:NGPreferenceCollectionGlass]; NGInstallCollectionHooks();
    UIView *page = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 402, 874)];
    FQVShortVideoBaseInsertView *carrier = [[FQVShortVideoBaseInsertView alloc] initWithFrame:CGRectMake(0, 740, 402, 44)]; [page addSubview:carrier];
    SSShortVideoBottomDistributionView *row = [[SSShortVideoBottomDistributionView alloc] initWithFrame:carrier.bounds]; [carrier addSubview:row];
    SSShortVideoDistributionContentView *content = [[SSShortVideoDistributionContentView alloc] initWithFrame:row.bounds]; content.backgroundColor = [UIColor colorWithWhite:0 alpha:0.4]; [row addSubview:content];
    UIView *gray = [[UIView alloc] initWithFrame:content.bounds]; gray.backgroundColor = [UIColor colorWithWhite:1 alpha:0.15]; [content addSubview:gray];
    UIButton *action = [UIButton buttonWithType:UIButtonTypeSystem]; action.frame = CGRectInset(content.bounds, 16, 4); [action setTitle:@"观看完整全集 · 全74集" forState:UIControlStateNormal]; [content addSubview:action];
    NGDecorateCollectionRow(row);
    UIVisualEffectView *glass = objc_getAssociatedObject(row, &NGCollectionSurfaceKey);
    BOOL floating = glass && !glass.userInteractionEnabled && glass.frame.origin.x == 8 && glass.frame.size.width == 386 && glass.frame.size.height == 40;
    BOOL click = [page hitTest:CGPointMake(100, 760) withEvent:nil] == action;
    BOOL clear = CGColorGetAlpha(content.backgroundColor.CGColor) == 0 && CGColorGetAlpha(gray.backgroundColor.CGColor) == 0;
    UIColor *newGray = [UIColor colorWithWhite:0.2 alpha:0.7]; gray.backgroundColor = newGray;
    BOOL repaints = CGColorGetAlpha(gray.backgroundColor.CGColor) == 0;
    [d setBool:YES forKey:hideKey]; NGDecorateCollectionRow(row); row.hidden = NO; carrier.hidden = NO;
    BOOL hidden = row.hidden && carrier.hidden && [page hitTest:CGPointMake(100, 760) withEvent:nil] != action;
    [d setBool:NO forKey:hideKey]; NGDecorateCollectionRow(row); BOOL unhidden = !row.hidden && !carrier.hidden;
    [d setBool:NO forKey:NGPreferenceCollectionGlass]; NGDecorateCollectionRow(row);
    BOOL restored = !objc_getAssociatedObject(row, &NGCollectionSurfaceKey) && [gray.backgroundColor isEqual:newGray] && fabs(CGColorGetAlpha(content.backgroundColor.CGColor)-0.4)<0.01;
    if (oldHide) [d setObject:oldHide forKey:hideKey]; else [d removeObjectForKey:hideKey];
    if (oldGlass) [d setObject:oldGlass forKey:NGPreferenceCollectionGlass]; else [d removeObjectForKey:NGPreferenceCollectionGlass];
    return @{@"floatingCapsule":@(floating), @"nativeHitTarget":@(click), @"bothGrayBackplatesCleared":@(clear), @"hostRepaintSuppressed":@(repaints), @"wholeCarrierHiddenAfterHostShow":@(hidden), @"visibilityRestored":@(unhidden), @"latestBackgroundRestored":@(restored)};
}
static NSDictionary *NGTestReview075(void) {
    NSUserDefaults *d = NSUserDefaults.standardUserDefaults; BOOL old = [d boolForKey:NGPreferenceCommentGlass]; [d setBool:YES forKey:NGPreferenceCommentGlass]; NGInstallCommentGlassHooks();
    UIWindow *previous = NGBaseWindow(), *window = [[UIWindow alloc] initWithWindowScene:previous.windowScene];
    FQCShortVideoCommentPanelViewController *controller = [FQCShortVideoCommentPanelViewController new]; window.rootViewController = controller; [window makeKeyAndVisible];
    UIView *root = controller.view;
    FQCShortVideoCommentPanelTitleView *title = [[FQCShortVideoCommentPanelTitleView alloc] initWithFrame:CGRectMake(0, 0, 402, 44)]; [root addSubview:title]; NGDecorateCommentSource(title);
    UICollectionView *list = [[UICollectionView alloc] initWithFrame:CGRectMake(0, 44, 402, 700) collectionViewLayout:[UICollectionViewFlowLayout new]]; list.backgroundColor = UIColor.whiteColor; [root addSubview:list];
    FQCShortPlaySeriesCommentCell *cell = [[FQCShortPlaySeriesCommentCell alloc] initWithFrame:CGRectMake(0, 0, 402, 200)]; cell.backgroundColor = UIColor.whiteColor; cell.contentView.backgroundColor = UIColor.whiteColor; [list addSubview:cell]; [cell layoutIfNeeded];
    BOOL late = [list.backgroundColor isEqual:UIColor.clearColor] && [cell.backgroundColor isEqual:UIColor.clearColor] && [cell.contentView.backgroundColor isEqual:UIColor.clearColor];
    UIView *lateBackdrop = [[UIView alloc] initWithFrame:cell.contentView.bounds]; lateBackdrop.backgroundColor = UIColor.whiteColor; [cell.contentView insertSubview:lateBackdrop atIndex:0];
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem]; button.frame = CGRectMake(280, 140, 100, 40); [button setTitle:@"点赞" forState:UIControlStateNormal]; [cell.contentView addSubview:button];
    UIColor *latest = [UIColor colorWithWhite:0.96 alpha:1];
    for (NSUInteger i=0;i<100;i++) { cell.backgroundColor = latest; cell.contentView.backgroundColor = latest; lateBackdrop.backgroundColor = latest; }
    BOOL repaint = [cell.backgroundColor isEqual:UIColor.clearColor] && [cell.contentView.backgroundColor isEqual:UIColor.clearColor] && [lateBackdrop.backgroundColor isEqual:UIColor.clearColor];
    BOOL touch = [cell hitTest:CGPointMake(310, 160) withEvent:nil] == button;
    [cell prepareForReuse]; [cell setNeedsLayout]; [cell layoutIfNeeded]; BOOL reused = [cell.backgroundColor isEqual:UIColor.clearColor];
    UIView *unrelated = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 402, 400)]; [unrelated addSubview:cell];
    BOOL detached = [cell.backgroundColor isEqual:latest] && [lateBackdrop.backgroundColor isEqual:latest];
    [list addSubview:cell]; [cell setNeedsLayout]; [cell layoutIfNeeded];
    [d setBool:NO forKey:NGPreferenceCommentGlass]; NGDecorateCommentSource(title);
    BOOL restored = [cell.backgroundColor isEqual:latest] && [lateBackdrop.backgroundColor isEqual:latest];
    window.hidden = YES; [previous makeKeyWindow]; [d setBool:old forKey:NGPreferenceCommentGlass];
    return @{@"plainReviewCollectionAndLateCell":@(late), @"hundredRepaintsWithoutScroll":@(repaint), @"nativeActionsPreserved":@(touch), @"reuseStaysClear":@(reused), @"reparentRestores":@(detached), @"disableRestoresLatestColor":@(restored)};
}
static NSDictionary *NGTestUnderlay075(void) {
    NSUserDefaults *d = NSUserDefaults.standardUserDefaults; BOOL old = [d boolForKey:NGPreferenceDock]; [d setBool:YES forKey:NGPreferenceDock]; NGInstallDockUnderlayHooks();
    UIWindow *previous = NGBaseWindow(); BOOL fills=YES, unchanged=YES, contentSizePreserved=YES, offsetPreserved=YES, hits=YES, repeated=YES, restored=YES, bottomInset=YES, lastReachable=YES;
    for (NSUInteger kind=0;kind<2;kind++) {
        UIWindow *window = [[UIWindow alloc] initWithWindowScene:previous.windowScene];
        UIViewController *controller = kind ? [UIViewController new] : [FQMSEcomXTabMallViewController new];
        if (kind) controller.view = [[SSMine737KMPRootView alloc] initWithFrame:previous.bounds];
        window.rootViewController = controller; [window makeKeyAndVisible]; [controller.view layoutIfNeeded];
        UIView *root = controller.view; CGFloat full = root.bounds.size.height, width = root.bounds.size.width;
        UIView *container = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, full-84)]; container.clipsToBounds=YES; [root addSubview:container];
        UIView *inner = [[UIView alloc] initWithFrame:CGRectMake(0, 105, width, full-189)]; inner.clipsToBounds=YES; [container addSubview:inner];
        UICollectionViewFlowLayout *layout=[UICollectionViewFlowLayout new]; layout.itemSize=CGSizeMake((width-36)/2,180); layout.sectionInset=UIEdgeInsetsMake(12,12,12,12); layout.minimumLineSpacing=12; layout.minimumInteritemSpacing=12;
        UICollectionView *list = [[UICollectionView alloc] initWithFrame:inner.bounds collectionViewLayout:layout];
        NG075Gallery *gallery=[NG075Gallery new]; list.dataSource=gallery; [list registerClass:UICollectionViewCell.class forCellWithReuseIdentifier:@"gallery"];
        list.contentInsetAdjustmentBehavior=kind ? UIScrollViewContentInsetAdjustmentAlways : UIScrollViewContentInsetAdjustmentNever; list.contentInset=UIEdgeInsetsMake(0,0,12,0); [inner addSubview:list]; [list reloadData]; [list layoutIfNeeded]; list.contentOffset=CGPointMake(0,80);
        CGSize nativeContentSize=list.contentSize; CGFloat originalTotalBottom=list.adjustedContentInset.bottom;
        UIButton *tile = [UIButton buttonWithType:UIButtonTypeSystem]; tile.frame=CGRectMake(16,100,160,180); [tile setTitle:@"模拟商品 / 节目" forState:UIControlStateNormal]; [list addSubview:tile];
        CGRect tileFrame=tile.frame; CGPoint offset=list.contentOffset;
        NGDockUnderlayRefresh(root);
        fills &= NGFSNear(CGRectGetMaxY([list convertRect:list.bounds toView:root]),full) && NGFSNear(container.bounds.size.height,full);
        unchanged &= CGRectEqualToRect(tile.frame,tileFrame);
        offsetPreserved &= CGPointEqualToPoint(offset,list.contentOffset);
        contentSizePreserved &= CGSizeEqualToSize(nativeContentSize,list.contentSize);
        hits &= [list hitTest:CGPointMake(40,120) withEvent:nil]==tile;
        bottomInset &= NGFSNear(list.adjustedContentInset.bottom,originalTotalBottom+84);
        for (NSUInteger i=0;i<50;i++) { container.frame=CGRectMake(0,0,width,full-84); inner.frame=CGRectMake(0,105,width,full-189); list.frame=CGRectMake(0,0,width,full-189); NGDockUnderlayRefresh(root); }
        repeated &= NGFSNear(list.bounds.size.height,full-105) && NGFSNear(list.adjustedContentInset.bottom,originalTotalBottom+84);
        [list scrollToItemAtIndexPath:[NSIndexPath indexPathForItem:29 inSection:0] atScrollPosition:UICollectionViewScrollPositionBottom animated:NO]; [list layoutIfNeeded];
        lastReachable &= [list cellForItemAtIndexPath:[NSIndexPath indexPathForItem:29 inSection:0]] != nil;
        list.contentOffset=CGPointMake(0,80);
        [tile removeFromSuperview];
        UILabel *caption=[[UILabel alloc] initWithFrame:CGRectMake(16,55,width-32,38)]; caption.text=kind ? @"模拟我的页 · 非实机截图" : @"模拟商城页 · 非实机截图"; caption.textAlignment=NSTextAlignmentCenter; [root addSubview:caption];
        root.backgroundColor=UIColor.systemGroupedBackgroundColor;
        NGDockPreviewController *dock=[NGDockPreviewController new]; [controller addChildViewController:dock]; dock.view.frame=CGRectMake(16,full-84,width-32,84); [root addSubview:dock.view]; [dock didMoveToParentViewController:controller]; [dock.view layoutIfNeeded];
        NGFSScreenshot(window,kind ? @"mine-underlay-075.png" : @"mall-underlay-075.png");
        [d setBool:NO forKey:NGPreferenceDock]; NGDockUnderlayRefresh(root);
        restored &= NGFSNear(container.bounds.size.height,full-84) && NGFSNear(list.bounds.size.height,full-189) && NGFSNear(list.contentInset.bottom,12);
        [d setBool:YES forKey:NGPreferenceDock]; window.hidden=YES;
    }
    [previous makeKeyWindow]; [d setBool:old forKey:NGPreferenceDock];
    return @{@"mallAndMineReachScreenBottom":@(fills), @"tileGeometryPreserved":@(unchanged), @"contentSizePreserved":@(contentSizePreserved), @"contentOffsetPreserved":@(offsetPreserved), @"nativeHitPreserved":@(hits), @"lastRowKeepsDockClearance":@(bottomInset), @"lastItemReachable":@(lastReachable), @"fiftyHostRelayoutsStable":@(repeated), @"disableRestoresFramesAndInsets":@(restored)};
}
static NSDictionary *NGTestSettings075(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; NSMutableDictionary *saved=[NSMutableDictionary dictionary];
    for (NSDictionary *group in NGLiveLayoutControlGroups()) for (NSString *axis in @[@"x",@"y",@"scale"]) { NSString *key=[group[@"prefix"] stringByAppendingFormat:@".%@",axis]; saved[key]=[d objectForKey:key] ?: NSNull.null; [d setDouble:1.3 forKey:key]; }
    NGPlaybackLayoutSettingsController *settings=[NGPlaybackLayoutSettingsController new]; [settings loadViewIfNeeded];
    UITableViewCell *reset=[settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:0]];
    BOOL visible=[reset.textLabel.text containsString:@"重置默认"];
    [settings resetLayout]; BOOL allReset=YES;
    for (NSString *key in saved) allReset &= ![d objectForKey:key];
    NGLivePreviewController *preview=[NGLivePreviewController new]; [preview loadViewIfNeeded];
    for (NSString *key in saved) [d setDouble:1.2 forKey:key]; [preview resetLayout];
    NSMutableArray *nodes=[NSMutableArray arrayWithObject:preview.view]; BOOL liveReset=YES; NSUInteger sliderCount=0;
    for (NSUInteger i=0;i<nodes.count&&i<400;i++) { UIView *v=nodes[i]; if ([v isKindOfClass:UISlider.class]) { UISlider *slider=(id)v; sliderCount++; liveReset &= fabs(slider.value-([slider.accessibilityIdentifier hasSuffix:@"scale"]?1:0))<0.001; } [nodes addObjectsFromArray:v.subviews]; }
    BOOL removed=YES, newSetting=NO;
    for (NSDictionary *group in NGSettingsGroups()) for (NSDictionary *row in group[@"rows"]) { NSString *key=row[@"key"]; removed &= ![@[@"com.naiwaguo.playback.fullscreenLandscape",@"com.naiwaguo.playback.commentVideoFreeze",NGPreferenceRuntimeGlass] containsObject:key ?: @""]; newSetting |= [key isEqual:NGPreferenceCollectionGlass]; }
    for (NSString *key in saved) if (saved[key]==NSNull.null) [d removeObjectForKey:key]; else [d setObject:saved[key] forKey:key];
    BOOL symbols=YES; id old=[d objectForKey:NGPreferenceDockIconStyle];
    for (NSString *style in NGDockIconStyles()) { [d setObject:style forKey:NGPreferenceDockIconStyle]; for (NSUInteger i=0;i<4;i++) symbols &= NGDockIcon(i,NO)!=nil && NGDockIcon(i,YES)!=nil; }
    if(old)[d setObject:old forKey:NGPreferenceDockIconStyle];else[d removeObjectForKey:NGPreferenceDockIconStyle];
    return @{@"explicitResetRow":@(visible), @"allFifteenValuesReset":@(allReset), @"liveSlidersReset":@(liveReset&&sliderCount==15), @"obsoleteSettingsRemoved":@(removed), @"collectionSettingPresent":@(newSetting), @"newDockSymbolsAvailable":@(symbols), @"runtimeGlassAlwaysRequested":@(NGRuntimeGlassRequested)};
}
static NSDictionary *NGTestBackdrop075(void) {
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults; BOOL old=[d boolForKey:NGPreferencePlaybackFullscreen];
    UIWindow *previous=NGBaseWindow(), *window=[[UIWindow alloc] initWithWindowScene:previous.windowScene];
    NGFullscreenFixture *host=[NGFullscreenFixture new]; host.landscape=YES; window.rootViewController=host; [window makeKeyAndVisible]; [host.view layoutIfNeeded]; [host.feed layoutIfNeeded];
    UIView *hud=[[host.feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:0]] viewWithTag:7403];
    FQVShortVideoGradientLayerView *top=[[FQVShortVideoGradientLayerView alloc] initWithFrame:CGRectMake(0,0,402,159)]; [hud addSubview:top];
    FQVShortVideoGradientLayerView *bottom=[[FQVShortVideoGradientLayerView alloc] initWithFrame:CGRectMake(0,504,402,280)]; [hud addSubview:bottom]; bottom.alpha=0.8;
    [d setBool:YES forKey:NGPreferencePlaybackFullscreen]; NGInstallPlaybackFullscreenHooks(); NGUpdatePlaybackFullscreen();
    BOOL cleared=bottom.alpha==0&&top.alpha==1; bottom.alpha=0.6; BOOL repaint=bottom.alpha==0;
    [d setBool:NO forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen(); BOOL restored=fabs(bottom.alpha-0.6)<0.001;
    // Full-height collection pages must also preserve the complete landscape frame.
    host.nativeHeight=window.bounds.size.height; host.feed.frame=window.bounds; ((UICollectionViewFlowLayout *)host.feed.collectionViewLayout).itemSize=window.bounds.size; [host.feed reloadData]; [host.feed layoutIfNeeded];
    [d setBool:YES forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    UIView *render=[[host.feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:0]] viewWithTag:7402];
    CGRect fitted=[render convertRect:render.bounds toView:window];
    BOOL nativeFull=render && NGFSNear(fitted.size.width,window.bounds.size.width) && NGFSNear(fitted.size.height,window.bounds.size.width*9/16) && NGFSNear(CGRectGetMidY(fitted),window.bounds.size.height/2);
    [d setBool:NO forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen(); window.hidden=YES; [previous makeKeyWindow]; [d setBool:old forKey:NGPreferencePlaybackFullscreen];
    return @{@"bottomGradientTransparentTopPreserved":@(cleared), @"hostAlphaRepaintSuppressed":@(repaint), @"latestAlphaRestored":@(restored), @"alreadyFullHeightFeedKeepsLandscapeAspect":@(nativeFull)};
}
