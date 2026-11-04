@interface TTPlayerView : UIView @end
@implementation TTPlayerView
- (void)drawRect:(CGRect)rect {
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    NSArray *colors = @[(__bridge id)[UIColor colorWithRed:0.08 green:0.40 blue:0.60 alpha:1].CGColor,
                        (__bridge id)[UIColor colorWithRed:0.90 green:0.49 blue:0.22 alpha:1].CGColor,
                        (__bridge id)[UIColor colorWithRed:0.15 green:0.60 blue:0.42 alpha:1].CGColor];
    CGGradientRef gradient = CGGradientCreateWithColors(space, (__bridge CFArrayRef)colors, NULL);
    CGContextDrawLinearGradient(context, gradient, CGPointZero, CGPointMake(self.bounds.size.width, self.bounds.size.height), 0);
    CGGradientRelease(gradient); CGColorSpaceRelease(space);
    [UIColor.whiteColor setStroke]; CGContextSetLineWidth(context, 2);
    CGContextStrokeEllipseInRect(context, CGRectMake(self.bounds.size.width / 2 - 25, self.bounds.size.height / 2 - 25, 50, 50));
}
@end
@interface FQVPlayer : UIView @end
@implementation FQVPlayer @end
@interface NGFullscreenFixture : UIViewController <UICollectionViewDataSource>
@property(nonatomic, strong) FQVCollectionView *feed;
@property(nonatomic) CGFloat nativeHeight;
@property(nonatomic) BOOL landscape;
@property(nonatomic) NSUInteger taps;
@end
@implementation NGFullscreenFixture
- (void)viewDidLoad {
    [super viewDidLoad]; self.view.backgroundColor = [UIColor colorWithWhite:0.10 alpha:1];
    self.nativeHeight = self.view.bounds.size.height - 90;
    UICollectionViewFlowLayout *layout = [UICollectionViewFlowLayout new];
    layout.itemSize = CGSizeMake(self.view.bounds.size.width, self.nativeHeight);
    layout.minimumLineSpacing = 0; layout.minimumInteritemSpacing = 0;
    self.feed = [[FQVCollectionView alloc] initWithFrame:CGRectMake(0, 0, self.view.bounds.size.width, self.nativeHeight) collectionViewLayout:layout];
    self.feed.backgroundColor = UIColor.clearColor; self.feed.pagingEnabled = YES;
    self.feed.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    [self.feed registerClass:FQVShortVideoCell.class forCellWithReuseIdentifier:@"page"];
    self.feed.dataSource = self; [self.view addSubview:self.feed];
}
- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section { return 5; }
- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)path {
    FQVShortVideoCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"page" forIndexPath:path];
    for (UIView *view in cell.contentView.subviews.copy) [view removeFromSuperview];
    CGRect bounds = CGRectMake(0, 0, self.feed.bounds.size.width, self.nativeHeight);
    FQVShortVideoPlayerCellView *player = [[FQVShortVideoPlayerCellView alloc] initWithFrame:bounds];
    player.clipsToBounds = YES; player.tag = 7401; player.backgroundColor = [UIColor colorWithWhite:0.12 alpha:1];
    [cell.contentView addSubview:player];
    UIView *branch = [[UIView alloc] initWithFrame:bounds]; [player addSubview:branch];
    FQVPlayer *engine = [[FQVPlayer alloc] initWithFrame:bounds]; [branch addSubview:engine];
    CGFloat h = self.landscape ? bounds.size.width * 9 / 16 : bounds.size.height;
    TTPlayerView *render = [[TTPlayerView alloc] initWithFrame:CGRectMake(0, (bounds.size.height - h) / 2, bounds.size.width, h)];
    render.tag = 7402; render.clipsToBounds = YES; [engine addSubview:render];
    UIView *hud = [[UIView alloc] initWithFrame:bounds]; hud.tag = 7403; [player addSubview:hud];
    UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(24, 100, bounds.size.width - 48, 60)];
    label.text = @"模拟测试画面 · 非红果实拍"; label.textColor = UIColor.whiteColor; label.numberOfLines = 2; label.font = [UIFont boldSystemFontOfSize:19]; [hud addSubview:label];
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem]; button.tag = 7404;
    button.frame = CGRectMake(24, bounds.size.height - 95, 240, 48); [button setTitle:@"原生播放控件 · 点击测试" forState:UIControlStateNormal];
    [button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [button addTarget:self action:@selector(tapped) forControlEvents:UIControlEventTouchUpInside]; [hud addSubview:button];
    return cell;
}
- (void)tapped { self.taps++; }
@end
static BOOL NGFSNear(CGFloat a, CGFloat b) { return fabs(a - b) < 0.75; }
static void NGFSScreenshot(UIWindow *window, NSString *name) {
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithBounds:window.bounds];
    UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES]; }];
    NSString *documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    [UIImagePNGRepresentation(image) writeToFile:[documents stringByAppendingPathComponent:name] atomically:YES];
}
static NSDictionary *NGTestFullscreenPlayback(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    BOOL enabled = [defaults boolForKey:NGPreferencePlaybackFullscreen];
    [defaults setBool:NO forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    UIWindow *previous = NGBaseWindow();
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:previous.windowScene];
    NGFullscreenFixture *host = [NGFullscreenFixture new]; host.landscape = YES;
    window.rootViewController = host; [window makeKeyAndVisible]; [host.view layoutIfNeeded];
    FQVCollectionView *feed = host.feed; [feed reloadData]; [feed layoutIfNeeded];
    CGFloat native = host.nativeHeight, full = window.bounds.size.height;
    NSIndexPath *first = [NSIndexPath indexPathForItem:0 inSection:0];
    UICollectionViewCell *cell = [feed cellForItemAtIndexPath:first];
    UIView *render = [cell viewWithTag:7402], *hud = [cell viewWithTag:7403];
    UIButton *button = [cell viewWithTag:7404];
    CGRect nativeRender = render.frame, nativeHUD = [hud convertRect:hud.bounds toView:window];
    CGPoint tap = [button convertPoint:CGPointMake(50, 20) toView:window];
    UITabBar *dock = [[UITabBar alloc] initWithFrame:CGRectMake(16, full - 84, window.bounds.size.width - 32, 84)];
    dock.items = @[[[UITabBarItem alloc] initWithTitle:@"首页" image:[UIImage systemImageNamed:@"house"] tag:0], [[UITabBarItem alloc] initWithTitle:@"剧场" image:[UIImage systemImageNamed:@"play.rectangle"] tag:1], [[UITabBarItem alloc] initWithTitle:@"我的" image:[UIImage systemImageNamed:@"person.crop.circle"] tag:2]];
    dock.selectedItem = dock.items.firstObject; [host.view addSubview:dock]; [dock layoutIfNeeded];
    NGFSScreenshot(window, @"fullscreen-before.png");
    [defaults setBool:YES forKey:NGPreferencePlaybackFullscreen]; [defaults setBool:NO forKey:@"com.naiwaguo.playback.fullscreenLandscape"];
    NGInstallPlaybackFullscreenHooks(); NGUpdatePlaybackFullscreen();
    CGRect expanded = [feed convertRect:feed.bounds toView:window], video = [render convertRect:render.bounds toView:window];
    CGRect square = [render convertRect:CGRectMake(10, 10, 20, 20) toView:window];
    BOOL covers = NGFSNear(expanded.size.height, full) && video.origin.y <= 0.5 && CGRectGetMaxY(video) >= full - 0.5 && video.size.width >= window.bounds.size.width;
    BOOL ratio = NGFSNear(square.size.width, square.size.height);
    BOOL geometry = NGFSNear(feed.bounds.size.height, native) && NGFSNear(feed.contentSize.height, native * 5) && feed.contentOffset.y == 0;
    CGRect hudAfter = [hud convertRect:hud.bounds toView:window];
    BOOL controls = NGFSNear(hudAfter.origin.y, nativeHUD.origin.y) && NGFSNear(hudAfter.size.height, nativeHUD.size.height) && [window hitTest:tap withEvent:nil] == button;
    [button sendActionsForControlEvents:UIControlEventTouchUpInside]; controls &= host.taps == 1;
    NGFSScreenshot(window, @"fullscreen-after.png");
    BOOL repeated = YES;
    for (NSUInteger i = 0; i < 100; i++) {
        feed.frame = CGRectMake(0, 0, window.bounds.size.width, native);
        render.frame = nativeRender; hud.frame = CGRectMake(0, 0, window.bounds.size.width, native);
        [feed setNeedsLayout]; [feed layoutIfNeeded];
        repeated &= NGFSNear(feed.bounds.size.height, native) && NGFSNear([render convertRect:render.bounds toView:window].size.height, full);
    }
    feed.frame = feed.frame; render.frame = render.frame; hud.frame = hud.frame;
    BOOL noOpWrites = NGFSNear(feed.bounds.size.height, native) && NGFSNear(render.bounds.size.height, nativeRender.size.height);
    // Bounds/center writes and changed aspect ratios happen after decoder reuse.
    render.bounds = CGRectMake(0, 0, window.bounds.size.width, native);
    render.center = CGPointMake(window.bounds.size.width / 2, native / 2);
    BOOL newAspect = NGFSNear([render convertRect:render.bounds toView:window].size.height, full);
    render.frame = nativeRender;
    BOOL returningAspect = NGFSNear([render convertRect:render.bounds toView:window].size.height, full);
    // A legacy separate landscape preference must no longer defeat full-screen.
    [defaults setBool:NO forKey:@"com.naiwaguo.playback.fullscreenLandscape"]; NGUpdatePlaybackFullscreen();
    BOOL landscapeIntegrated = NGFSNear([render convertRect:render.bounds toView:window].size.height, full);
    BOOL pages = YES;
    for (NSNumber *index in @[@2, @4, @0]) {
        NSIndexPath *path = [NSIndexPath indexPathForItem:index.integerValue inSection:0];
        [feed scrollToItemAtIndexPath:path atScrollPosition:UICollectionViewScrollPositionTop animated:NO]; [feed layoutIfNeeded];
        UICollectionViewCell *page = [feed cellForItemAtIndexPath:path]; UIView *surface = [page viewWithTag:7402];
        CGRect rect = [surface convertRect:surface.bounds toView:window];
        pages &= surface && NGFSNear(feed.contentOffset.y, native * index.integerValue) && NGFSNear(rect.origin.y, 0) && NGFSNear(rect.size.height, full);
    }
    [feed setContentOffset:CGPointMake(0, native * 0.25)]; [feed layoutIfNeeded];
    CGRect partial = [[feed cellForItemAtIndexPath:first] convertRect:[feed cellForItemAtIndexPath:first].bounds toView:window];
    BOOL swiping = NGFSNear(partial.origin.y, -full * 0.25) && NGFSNear(partial.size.height, full) && NGFSNear(feed.contentOffset.y, native * 0.25);
    [feed setContentOffset:CGPointZero]; [feed layoutIfNeeded];
    cell = [feed cellForItemAtIndexPath:first]; render = [cell viewWithTag:7402];
    UIView *parent = render.superview; [render removeFromSuperview];
    BOOL detached = CGAffineTransformIsIdentity(render.transform);
    TTPlayerView *replacement = [[TTPlayerView alloc] initWithFrame:nativeRender]; replacement.tag = 7402; [parent addSubview:replacement]; [replacement layoutIfNeeded];
    BOOL replacementWorks = NGFSNear([replacement convertRect:replacement.bounds toView:window].size.height, full);
    [defaults setBool:NO forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    BOOL restores = CGAffineTransformIsIdentity(feed.transform) && CGAffineTransformIsIdentity(replacement.transform) && CGRectEqualToRect(replacement.frame, nativeRender) && NGFSNear(feed.frame.size.height, native) && NGFSNear(feed.contentSize.height, native * 5);
    // A tall portrait render fills the same viewport with equal X/Y scaling.
    host.landscape = NO; [feed reloadData]; [feed layoutIfNeeded];
    [defaults setBool:YES forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    render = [[feed cellForItemAtIndexPath:first] viewWithTag:7402];
    square = [render convertRect:CGRectMake(10, 10, 20, 20) toView:window];
    BOOL portrait = NGFSNear([render convertRect:render.bounds toView:window].size.height, full) && NGFSNear(square.size.width, square.size.height);
    NGFSScreenshot(window, @"fullscreen-portrait.png");
    window.windowLevel = UIWindowLevelAlert; NGFullscreenRefreshFeed(feed);
    BOOL overlayExcluded = CGAffineTransformIsIdentity(feed.transform) && CGAffineTransformIsIdentity(render.transform);
    UIView *unrelated = [[UIView alloc] initWithFrame:CGRectMake(10, 20, 60, 80)]; unrelated.transform = CGAffineTransformMakeScale(0.7, 0.8); unrelated.frame = CGRectMake(30, 40, 90, 100);
    BOOL unrelatedUnchanged = CGRectEqualToRect(unrelated.frame, CGRectMake(30, 40, 90, 100));
    [defaults setBool:NO forKey:NGPreferencePlaybackFullscreen]; NGUpdatePlaybackFullscreen();
    window.hidden = YES; [previous makeKeyWindow];
    [defaults setBool:enabled forKey:NGPreferencePlaybackFullscreen]; [defaults removeObjectForKey:@"com.naiwaguo.playback.fullscreenLandscape"]; NGUpdatePlaybackFullscreen();
    return @{@"coversReservedBottom": @(covers), @"uniformVideoScale": @(ratio), @"nativePagingGeometry": @(geometry),
             @"noOpFrameWrites": @(noOpWrites), @"decoderAspectChanges": @(newAspect && returningAspect),
             @"nativeHUDAndTouches": @(controls), @"hundredHostRewrites": @(repeated), @"landscapeIntegratedIntoFullscreen": @(landscapeIntegrated),
             @"firstMiddleLastPages": @(pages), @"fractionalSwipeRemainsAligned": @(swiping), @"detachedRenderRestores": @(detached),
             @"lateReplacementFills": @(replacementWorks), @"disableRestores": @(restores), @"portraitFills": @(portrait),
             @"overlayWindowExcluded": @(overlayExcluded), @"unrelatedViewsUnchanged": @(unrelatedUnchanged)};
}
