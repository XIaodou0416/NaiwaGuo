#define NG_TEST 1
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import "../src/NaiwaGuo.m"

static NSDictionary *NGTestFrameworkOriginalInfo;

@interface SSTabBarTextButton : UIControl
@end
@implementation SSTabBarTextButton
@end
@interface SSTabBar : UIView
@end
@implementation SSTabBar
@end
@interface SSTabBarController : UITabBarController
@property(nonatomic) NSUInteger taps;
@property(nonatomic, strong) SSTabBar *customBar;
@end

@interface FQCShortVideoCommentPanelViewController : UIViewController
@end
@implementation FQCShortVideoCommentPanelViewController
@end
@interface SSVideoSharePanelViewController : UIViewController
@end
@implementation SSVideoSharePanelViewController
@end
@interface SSUGKmpSharePanelContainerViewController : UIViewController
@end
@implementation SSUGKmpSharePanelContainerViewController
@end
@interface FQCShortVideoCommentPanelTitleView : UIView
@end
@implementation FQCShortVideoCommentPanelTitleView
@end
@interface FQCShortVideoCommentListCollectionView : UICollectionView
@end
@interface SSShortVideoRightInteractionAvatarView : UIView @end
@implementation SSShortVideoRightInteractionAvatarView
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.backgroundColor = [UIColor colorWithWhite:0.94 alpha:1]; self.layer.cornerRadius = 20; self.clipsToBounds = YES;
        UIImageView *image = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"person.crop.circle.fill"]];
        image.tintColor = [UIColor colorWithWhite:0.2 alpha:1]; image.frame = self.bounds; image.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight; image.contentMode = UIViewContentModeScaleAspectFit;
        [self addSubview:image];
    }
    return self;
}
@end
@implementation FQCShortVideoCommentListCollectionView
@end
@interface FQCShortVideoBottomToolBar : UIView @end
@implementation FQCShortVideoBottomToolBar @end
@interface FQVShortVideoPlayerCellView : UIView @end
@implementation FQVShortVideoPlayerCellView @end
@interface FQVShortVideoCell : UICollectionViewCell @end
@implementation FQVShortVideoCell @end
@interface FQVCollectionView : UICollectionView @end
@implementation FQVCollectionView @end
@interface FQVTransitionPlayerLimitScaleTopContentView : UIView @end
@implementation FQVTransitionPlayerLimitScaleTopContentView @end
@interface FQVTransitionPlayerLimitScaleContainerView : UIView @end
@implementation FQVTransitionPlayerLimitScaleContainerView @end
@interface SSShortVideoOutterSeriesInfoView : UIView @end
@implementation SSShortVideoOutterSeriesInfoView @end
@interface SSShortVideoAiUsageView : UIView @end
@implementation SSShortVideoAiUsageView @end
@interface SSVideoSeriesFeedHeaderView : UIView
@end
@implementation SSVideoSeriesFeedHeaderView
@end
@interface SSHGBookStoreCategoryTitleView : UIView
@end
@implementation SSHGBookStoreCategoryTitleView
@end
@interface SSHGBookStoreCategoryTitleCell : UIView
@end
@implementation SSHGBookStoreCategoryTitleCell
@end
@interface FQRJXCategoryIndicatorLineView : UIView
@end
@implementation FQRJXCategoryIndicatorLineView
@end
@interface SSVideoSeriesFeedHeaderSearchView : UIView
@end
@implementation SSVideoSeriesFeedHeaderSearchView
@end
@interface FQReaderSaaSBaseImageView : UIImageView
@end
@implementation FQReaderSaaSBaseImageView
@end
@interface FQRJXCategoryCollectionView : UIView
@end
@implementation FQRJXCategoryCollectionView
@end

#include "NGFollowupTests.inc"
#include "NGFullscreenPlaybackTests.inc"
#include "NGRegression075Tests.inc"

@interface NGTestPanGestureRecognizer : UIPanGestureRecognizer
@property(nonatomic) UIGestureRecognizerState testState;
@property(nonatomic) CGPoint testTranslation;
@end
@implementation NGTestPanGestureRecognizer
- (UIGestureRecognizerState)state { return self.testState; }
- (CGPoint)translationInView:(UIView *)view { return self.testTranslation; }
@end
@implementation SSTabBarController
- (void)tapped:(UIControl *)sender { self.taps++; self.selectedIndex = sender.tag; }
- (void)viewDidLoad {
    [super viewDidLoad];
    NSMutableArray *pages = [NSMutableArray array];
    self.customBar = [[SSTabBar alloc] initWithFrame:CGRectMake(0, 700, 402, 84)];
    for (NSUInteger index = 0; index < 4; index++) {
        [pages addObject:[UIViewController new]];
        SSTabBarTextButton *button = [[SSTabBarTextButton alloc] initWithFrame:CGRectMake(index * 100, 0, 100, 50)];
        button.tag = index;
        UILabel *label = [[UILabel alloc] initWithFrame:button.bounds];
        label.text = @[@"首页", @"剧场", @"商城", @"我的"][index];
        [button addSubview:label];
        [button addTarget:self action:@selector(tapped:) forControlEvents:UIControlEventTouchUpInside];
        [self.customBar addSubview:button];
    }
    self.viewControllers = pages;
    [self.view addSubview:self.customBar];
}
@end

static NSDictionary *NGTestDock(void) {
    SSTabBarController *host = [SSTabBarController new];
    [host loadViewIfNeeded];
    UIWindow *testWindow = [[UIWindow alloc] initWithWindowScene:NGBaseWindow().windowScene];
    testWindow.rootViewController = host;
    testWindow.hidden = NO;
    for (UIViewController *page in host.viewControllers) page.view.backgroundColor = UIColor.redColor;
    host.view.frame = CGRectMake(0, 0, 402, 874);
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceDock];
    [NSUserDefaults.standardUserDefaults setObject:@[] forKey:NGPreferenceHiddenDockItems];
    NGRefreshDock(host);
    NGSystemDock *dock = objc_getAssociatedObject(host, &NGDockAssociation);
    [dock.view layoutIfNeeded];
    [host.view layoutIfNeeded];
    BOOL bounded = CGRectGetHeight(dock.view.bounds) < 140 && CGRectGetMinY([dock.view convertRect:dock.view.bounds toView:host.view]) > CGRectGetHeight(host.view.bounds) / 2;
    BOOL touchThrough = [host.customBar hitTest:[host.view convertPoint:CGPointMake(100,100) toView:host.customBar] withEvent:nil]==nil;
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithBounds:host.view.bounds];
    UIImage *rendered = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [host.view.layer renderInContext:context.CGContext]; }];
    unsigned char pixel[4] = {0};
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef sample = CGBitmapContextCreate(pixel, 1, 1, 8, 4, space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGContextTranslateCTM(sample, -100*rendered.scale, -(rendered.size.height-400)*rendered.scale);
    CGContextDrawImage(sample, CGRectMake(0, 0, CGImageGetWidth(rendered.CGImage), CGImageGetHeight(rendered.CGImage)), rendered.CGImage);
    CGContextRelease(sample);
    CGColorSpaceRelease(space);
    BOOL contentVisible = pixel[0] > 200 && pixel[1] < 80 && pixel[2] < 80;
    host.selectedViewController.view.backgroundColor = UIColor.systemBackgroundColor;
    UILabel *previewLabel = [[UILabel alloc] initWithFrame:CGRectMake(24, 150, 300, 32)];
    previewLabel.text = @"悬浮液态底栏预览";
    previewLabel.font = [UIFont systemFontOfSize:20 weight:UIFontWeightSemibold];
    previewLabel.textColor = UIColor.labelColor;
    [host.selectedViewController.view addSubview:previewLabel];
    [host.view layoutIfNeeded]; [dock.view layoutIfNeeded];
    UIImage *dockPreview = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [host.view drawViewHierarchyInRect:host.view.bounds afterScreenUpdates:YES]; }];
    NSString *documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    [UIImagePNGRepresentation(dockPreview) writeToFile:[documents stringByAppendingPathComponent:@"dock.png"] atomically:YES];
    BOOL built = dock.viewControllers.count == 4 && dock.view.superview==host.customBar && !host.customBar.hidden && [dock.tabBar isKindOfClass:UITabBar.class];
    NSUInteger pluginGlassCount = 0;
    NSMutableArray<UIView *> *dockViews = [NSMutableArray arrayWithObject:dock.tabBar];
    for (NSUInteger index = 0; index < dockViews.count && index < 300; index++) {
        UIView *candidate = dockViews[index];
        if ([candidate.accessibilityIdentifier isEqualToString:@"NaiwaGuo.Dock.FloatingGlass"]) pluginGlassCount++;
        [dockViews addObjectsFromArray:candidate.subviews];
    }
    BOOL floatingCapsule = pluginGlassCount == 0 && dock.platter && dock.searchKey && dock.platter.bounds.size.width < host.customBar.bounds.size.width && dock.overrideUserInterfaceStyle == UIUserInterfaceStyleUnspecified;
    [dock tabBar:dock.tabBar didSelectItem:dock.viewControllers[2].tabBarItem];
    BOOL switched = host.taps == 1 && host.selectedIndex == 2;
    CGRect pageFrame = host.selectedViewController.view.frame;
    [NSUserDefaults.standardUserDefaults setObject:@[@"mine"] forKey:NGPreferenceHiddenDockItems];
    NGRefreshDock(host);
    BOOL hiddenItem = dock.buttons.count == 3 && dock.viewControllers.count == 3 && !host.customBar.hidden && dock.buttons.firstObject.layer.opacity==0;
    BOOL noResize = CGRectEqualToRect(pageFrame, host.selectedViewController.view.frame);
    [NSUserDefaults.standardUserDefaults setObject:@[] forKey:NGPreferenceHiddenDockItems];
    NGRefreshDock(host);
    BOOL restoredItems = dock.buttons.count == 4 && dock.viewControllers.count == 4;
    FQCShortVideoCommentPanelViewController *panel = [FQCShortVideoCommentPanelViewController new];
    UIViewController *panelOwner = host.selectedViewController;
    [panelOwner addChildViewController:panel];
    panel.view.frame = CGRectMake(0, 200, 402, 600);
    [panelOwner.view addSubview:panel.view];
    [panel didMoveToParentViewController:panelOwner];
    BOOL panelFound = NGDockHasPlaybackPanel(host);
    BOOL panelVisible = NGDockControllerIsVisible(panel.view, testWindow);
    NGRefreshDock(host);
    BOOL panelHidesDock = dock.view.hidden;
    BOOL dockStillAttached = objc_getAssociatedObject(host, &NGDockAssociation) == dock && dock.view.superview == host.customBar;
    [panel willMoveToParentViewController:nil];
    [panel.view removeFromSuperview];
    [panel removeFromParentViewController];
    BOOL panelGone = !NGDockHasPlaybackPanel(host);
    NGRefreshDock(host);
    BOOL panelDismissRestoresDock = !dock.view.hidden;
    SSVideoSharePanelViewController *sharePanel = [SSVideoSharePanelViewController new];
    [panelOwner addChildViewController:sharePanel];
    sharePanel.view.frame = CGRectMake(0, 200, 402, 600);
    [panelOwner.view addSubview:sharePanel.view];
    [sharePanel didMoveToParentViewController:panelOwner];
    BOOL sharePanelFound = NGDockHasPlaybackPanel(host);
    NGRefreshDock(host);
    BOOL sharePanelHidesDock = dock.view.hidden;
    [sharePanel willMoveToParentViewController:nil];
    [sharePanel.view removeFromSuperview];
    [sharePanel removeFromParentViewController];
    NGRefreshDock(host);
    BOOL sharePanelDismissRestoresDock = !dock.view.hidden;
    FQVShortVideoSettingsViewController *more = [FQVShortVideoSettingsViewController new];
    [host addChildViewController:more]; more.view.frame = CGRectMake(0, 262, 402, 612); [host.view addSubview:more.view]; [more didMoveToParentViewController:host];
    NGRefreshDock(host); BOOL moreHidden = dock.view.hidden;
    more.view.hidden = YES; NGRefreshDock(host); BOOL hiddenMoreIgnored = !dock.view.hidden;
    [more willMoveToParentViewController:nil]; [more.view removeFromSuperview]; [more removeFromParentViewController];
    NGRefreshDock(host); BOOL moreRestored = !dock.view.hidden;
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceDock];
    NGRefreshDock(host);
    BOOL restored = !host.customBar.hidden && !objc_getAssociatedObject(host, &NGDockAssociation);
    BOOL systemWasHidden = host.tabBar.hidden;
    [NSUserDefaults.standardUserDefaults setObject:@[@"home"] forKey:NGPreferenceHiddenDockItems];
    NGRefreshDock(host);
    BOOL hiddenNative = !host.customBar.hidden && NGDockOriginalButtons(host.customBar).firstObject.hidden;
    [NSUserDefaults.standardUserDefaults setObject:@[] forKey:NGPreferenceHiddenDockItems];
    NGRefreshDock(host);
    BOOL nativeRestored = !host.customBar.hidden && !NGDockOriginalButtons(host.customBar).firstObject.hidden && host.tabBar.hidden == systemWasHidden;
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceDock];
    BOOL dockIconPresets = YES; NSMutableSet *dockPresetImages = [NSMutableSet set];
    for (NSString *style in NGDockIconStyles()) { [NSUserDefaults.standardUserDefaults setObject:style forKey:NGPreferenceDockIconStyle]; UIImage *icon = NGDockIcon(0, NO); dockIconPresets &= icon != nil; if (icon) [dockPresetImages addObject:UIImagePNGRepresentation(icon)]; }
    [NSUserDefaults.standardUserDefaults setObject:@"sun.max.fill" forKey:@"com.naiwaguo.appearance.dockIcon.0"];
    dockIconPresets &= NGDockIcon(0, NO) != nil;
    [NSUserDefaults.standardUserDefaults removeObjectForKey:@"com.naiwaguo.appearance.dockIcon.0"];
    [NSUserDefaults.standardUserDefaults setObject:@"original" forKey:NGPreferenceDockIconStyle];
    testWindow.hidden = YES;
    return @{@"longPressPanelHidesAndRestoresDock": @(moreHidden && hiddenMoreIgnored && moreRestored), @"hidesSelectedItems": @(hiddenItem && restoredItems && hiddenNative && nativeRestored), @"panelHidesDock": @(panelHidesDock && panelDismissRestoresDock), @"commentPanelFound": @(panelFound), @"commentPanelVisible": @(panelVisible), @"commentPanelGoneAfterRemoval": @(panelGone), @"dockAttachedWhenPanelShown": @(dockStillAttached), @"dockHiddenWhenPanelShown": @(panelHidesDock), @"commentPanelDismissRestoresDock": @(panelDismissRestoresDock), @"sharePanelHidesAndRestoresDock": @(sharePanelFound && sharePanelHidesDock && sharePanelDismissRestoresDock), @"preservesPageFrame": @(noResize), @"built": @(built), @"floatingCapsule": @(floatingCapsule), @"noDuplicateGlass": @(pluginGlassCount == 0), @"switchesHost": @(switched), @"restores": @(restored), @"hostIcon": @(NGHostIcon() != nil), @"boundedToBottom": @(bounded), @"contentVisible": @(contentVisible), @"touchThrough": @(touchThrough), @"iconPresetsAndCustom": @(dockIconPresets && dockPresetImages.count == NGDockIconStyles().count)};
}

@interface SSSettingViewController : UITableViewController
@property(nonatomic) NSInteger originalSelections;
@end
@implementation SSSettingViewController
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 2; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { NSCAssert(section < 2, @"Invalid host section"); return section + 1; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSCAssert(path.section < 2, @"Invalid host cell");
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = @"原生设置";
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path { NSCAssert(path.section < 2, @"Invalid host selection"); self.originalSelections++; }
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { NSCAssert(path.section < 2, @"Invalid host height"); return 50; }
- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section { NSCAssert(section < 2, @"Invalid host header"); return nil; }
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section { NSCAssert(section < 2, @"Invalid host header height"); return 8; }
@end

static NSDictionary *NGTestNativeSettings(void) {
    SSSettingViewController *host = [SSSettingViewController new];
    [host loadViewIfNeeded];
    BOOL counts = [host numberOfSectionsInTableView:host.tableView] == 2 && [host tableView:host.tableView numberOfRowsInSection:1] == 2;
    [host tableView:host.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:1]];
    BOOL entry = [host.tableView.tableHeaderView.accessibilityIdentifier isEqual:@"NaiwaGuo.NativeEntry"] && host.tableView.tableHeaderView.frame.size.height == 62;
    NGSettingsViewController *settings = [NGSettingsViewController new];
    UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:settings];
    [settings loadViewIfNeeded];
    NGSettingsGroupViewController *detail = [NGSettingsGroupViewController new];
    detail.groupIndex = 0;
    [nav pushViewController:detail animated:NO];
    [detail loadViewIfNeeded];
    BOOL back = detail.navigationItem.leftBarButtonItem != nil && settings.navigationItem.leftBarButtonItem != nil;
    [detail goBack];
    UISearchController *search = settings.navigationItem.searchController; search.searchBar.text = @"作者头像"; [settings updateSearchResultsForSearchController:search];
    BOOL searchesNestedSettings = search && settings.searchResults.count > 0 && [settings.searchResults.firstObject[@"row"][@"action"] isEqualToString:@"playbackVisibility"];
    return @{@"counts": @(counts), @"entry": @(entry), @"originalSelection": @(host.originalSelections == 1), @"back": @(back), @"returns": @(nav.topViewController == settings), @"searchesNestedSettings": @(searchesNestedSettings)};
}

static void NGConfigureMockAction(UIView *view, NSString *symbol, NSString *text) {
    UIImageView *image = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:symbol withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:26 weight:UIImageSymbolWeightSemibold]]];
    image.tag = 100;
    image.tintColor = UIColor.whiteColor;
    image.contentMode = UIViewContentModeScaleAspectFit;
    [view addSubview:image];
    UILabel *label = [UILabel new];
    label.tag = 101;
    label.text = text;
    label.textColor = UIColor.whiteColor;
    label.font = [UIFont systemFontOfSize:10 weight:UIFontWeightSemibold];
    label.textAlignment = NSTextAlignmentCenter;
    [view addSubview:label];
}

@interface SSShortVideoRightInteractionCollectModel : NSObject
@property(nonatomic) BOOL isCollected;
@end
@implementation SSShortVideoRightInteractionCollectModel @end
@interface SSShortVideoRightInteractionLikeModel : NSObject
@property(nonatomic) BOOL isLiked;
@end
@implementation SSShortVideoRightInteractionLikeModel @end
@interface SSShortVideoRightInteractionCollectView : UIView
@property(nonatomic,strong) id model;
@end
@interface SSShortVideoRightInteractionCommentView : UIView @end
@interface SSShortVideoRightInteractionLikeView : UIView
@property(nonatomic,strong) id model;
@end
@interface SSShortVideoRightInteractionShareView : UIView @end
@implementation SSShortVideoRightInteractionCollectView
- (instancetype)initWithFrame:(CGRect)frame { if ((self = [super initWithFrame:frame])) NGConfigureMockAction(self, @"star.fill", @"27.6万"); return self; }
- (void)layoutSubviews { [super layoutSubviews]; [self viewWithTag:100].frame = CGRectMake(0, 0, 46, 46); [self viewWithTag:101].frame = CGRectMake(0, 46, 46, 16); }
@end
@implementation SSShortVideoRightInteractionCommentView
- (instancetype)initWithFrame:(CGRect)frame { if ((self = [super initWithFrame:frame])) NGConfigureMockAction(self, @"ellipsis.message.fill", @"223"); return self; }
- (void)layoutSubviews { [super layoutSubviews]; [self viewWithTag:100].frame = CGRectMake(0, 0, 46, 46); [self viewWithTag:101].frame = CGRectMake(0, 46, 46, 16); }
@end
@implementation SSShortVideoRightInteractionLikeView
- (instancetype)initWithFrame:(CGRect)frame { if ((self = [super initWithFrame:frame])) NGConfigureMockAction(self, @"heart.fill", @"2.2万"); return self; }
- (void)layoutSubviews { [super layoutSubviews]; [self viewWithTag:100].frame = CGRectMake(0, 0, 46, 46); [self viewWithTag:101].frame = CGRectMake(0, 46, 46, 16); }
@end
@implementation SSShortVideoRightInteractionShareView
- (instancetype)initWithFrame:(CGRect)frame { if ((self = [super initWithFrame:frame])) NGConfigureMockAction(self, @"paperplane.fill", @"分享"); return self; }
- (void)layoutSubviews { [super layoutSubviews]; [self viewWithTag:100].frame = CGRectMake(0, 0, 46, 46); [self viewWithTag:101].frame = CGRectMake(0, 46, 46, 16); }
@end

@interface SSShortVideoRightInteractionView : UIView
@end
@implementation SSShortVideoRightInteractionView
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        [self addSubview:[[SSShortVideoRightInteractionAvatarView alloc] initWithFrame:CGRectZero]];
        [self addSubview:[[SSShortVideoRightInteractionCollectView alloc] initWithFrame:CGRectZero]];
        [self addSubview:[[SSShortVideoRightInteractionCommentView alloc] initWithFrame:CGRectZero]];
        [self addSubview:[[SSShortVideoRightInteractionLikeView alloc] initWithFrame:CGRectZero]];
        [self addSubview:[[SSShortVideoRightInteractionShareView alloc] initWithFrame:CGRectZero]];
    }
    return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];
    NSUInteger index = 0;
    for (UIView *subview in self.subviews) {
        if ([subview isKindOfClass:NGPlaybackGlassView.class]) continue;
        subview.frame = CGRectMake(0, index * 74, 46, 62);
        index++;
    }
}
@end

@interface NGTestRootController : UIViewController
@property(nonatomic, strong) SSShortVideoRightInteractionView *rightRail;
@property(nonatomic, strong) CAGradientLayer *background;
@property(nonatomic, strong) FQVShortVideoPlayerCellView *playerView;
@property(nonatomic, strong) UIView *videoSurface;
@end

@implementation NGTestRootController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"红果短剧宿主";
    self.view.backgroundColor = UIColor.blackColor;
    self.playerView = [[FQVShortVideoPlayerCellView alloc] initWithFrame:self.view.bounds];
    self.playerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.playerView.backgroundColor = UIColor.blackColor;
    [self.view addSubview:self.playerView];

    // A synthetic landscape frame makes the simulator screenshot useful without implying real video playback.
    self.videoSurface = [[UIView alloc] initWithFrame:CGRectMake(0, 250, CGRectGetWidth(self.view.bounds), 226)];
    self.videoSurface.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    self.videoSurface.clipsToBounds = YES;
    self.videoSurface.layer.cornerRadius = 2;
    [self.playerView addSubview:self.videoSurface];
    self.background = [CAGradientLayer layer];
    self.background.colors = @[(__bridge id)[UIColor colorWithRed:0.12 green:0.28 blue:0.42 alpha:1].CGColor,
                               (__bridge id)[UIColor colorWithRed:0.68 green:0.46 blue:0.31 alpha:1].CGColor,
                               (__bridge id)[UIColor colorWithRed:0.17 green:0.31 blue:0.28 alpha:1].CGColor];
    self.background.startPoint = CGPointMake(0, 0);
    self.background.endPoint = CGPointMake(1, 1);
    [self.videoSurface.layer addSublayer:self.background];

    UIView *back = [[UIView alloc] initWithFrame:CGRectMake(16, 104, 44, 44)];
    back.backgroundColor = [UIColor colorWithWhite:0.12 alpha:0.95];
    back.layer.cornerRadius = 22;
    UIImageView *backIcon = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"arrow.left" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:18 weight:UIImageSymbolWeightSemibold]]];
    backIcon.tintColor = UIColor.whiteColor; backIcon.frame = CGRectMake(12, 12, 20, 20); backIcon.contentMode = UIViewContentModeScaleAspectFit;
    [back addSubview:backIcon]; [self.playerView addSubview:back];
    UIView *more = [[UIView alloc] initWithFrame:CGRectMake(CGRectGetWidth(self.view.bounds) - 60, 104, 44, 44)];
    more.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    more.backgroundColor = back.backgroundColor; more.layer.cornerRadius = 22;
    UIImageView *moreIcon = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"ellipsis" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:18 weight:UIImageSymbolWeightBold]]];
    moreIcon.tintColor = UIColor.whiteColor; moreIcon.frame = CGRectMake(10, 12, 24, 20); moreIcon.contentMode = UIViewContentModeScaleAspectFit;
    [more addSubview:moreIcon]; [self.playerView addSubview:more];

    UILabel *account = [[UILabel alloc] initWithFrame:CGRectMake(68, 632, 205, 30)];
    account.text = @"演示作者  ✓"; account.font = [UIFont boldSystemFontOfSize:21]; account.textColor = UIColor.whiteColor;
    [self.playerView addSubview:account];
    UILabel *handle = [[UILabel alloc] initWithFrame:CGRectMake(68, 658, 240, 24)];
    handle.text = @"@NaiwaGuoDemo"; handle.font = [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold]; handle.textColor = [UIColor colorWithWhite:0.72 alpha:1];
    [self.playerView addSubview:handle];
    UILabel *caption = [[UILabel alloc] initWithFrame:CGRectMake(16, 694, CGRectGetWidth(self.view.bounds) - 32, 44)];
    caption.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    caption.text = @"模拟器演示内容 · 非真机视频"; caption.font = [UIFont boldSystemFontOfSize:19]; caption.textColor = UIColor.whiteColor; caption.numberOfLines = 2;
    [self.playerView addSubview:caption];
    UIButton *follow = [UIButton buttonWithType:UIButtonTypeSystem];
    follow.frame = CGRectMake(CGRectGetWidth(self.view.bounds) - 105, 635, 88, 38); follow.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    follow.backgroundColor = [UIColor colorWithWhite:0.12 alpha:1]; follow.layer.cornerRadius = 19;
    [follow setTitle:@"关注" forState:UIControlStateNormal]; [follow setTitleColor:UIColor.whiteColor forState:UIControlStateNormal]; follow.titleLabel.font = [UIFont boldSystemFontOfSize:16];
    [self.playerView addSubview:follow];
    UIView *progressTrack = [[UIView alloc] initWithFrame:CGRectMake(0, 791, CGRectGetWidth(self.view.bounds), 2)];
    progressTrack.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    progressTrack.backgroundColor = [UIColor colorWithWhite:0.22 alpha:1];
    UIView *progressPlayed = [[UIView alloc] initWithFrame:CGRectMake(0, 0, CGRectGetWidth(self.view.bounds) * 0.22, 2)];
    progressPlayed.backgroundColor = [UIColor colorWithWhite:0.72 alpha:1]; [progressTrack addSubview:progressPlayed]; [self.playerView addSubview:progressTrack];
    UIView *controls = [[UIView alloc] initWithFrame:CGRectMake(22, 807, CGRectGetWidth(self.view.bounds) - 44, 44)];
    controls.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
    UIImageView *pause = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"pause.fill"]];
    pause.tintColor = UIColor.whiteColor; pause.contentMode = UIViewContentModeScaleAspectFit; pause.frame = CGRectMake(0, 10, 22, 24); [controls addSubview:pause];
    UILabel *remaining = [[UILabel alloc] initWithFrame:CGRectMake(36, 6, 90, 32)]; remaining.text = @"-2:26"; remaining.textColor = [UIColor colorWithWhite:0.65 alpha:1]; remaining.font = [UIFont systemFontOfSize:17 weight:UIFontWeightMedium]; [controls addSubview:remaining];
    UILabel *speed = [[UILabel alloc] initWithFrame:CGRectMake(CGRectGetWidth(controls.bounds) - 178, 5, 42, 34)]; speed.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin; speed.text = @"1x"; speed.textColor = UIColor.whiteColor; speed.font = [UIFont boldSystemFontOfSize:24]; [controls addSubview:speed];
    UIImageView *volume = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"speaker.wave.2.fill"]]; volume.tintColor = UIColor.whiteColor; volume.contentMode = UIViewContentModeScaleAspectFit; volume.frame = CGRectMake(CGRectGetWidth(controls.bounds) - 106, 10, 24, 24); volume.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin; [controls addSubview:volume];
    [self.playerView addSubview:controls];
    UILabel *demo = [[UILabel alloc] initWithFrame:CGRectMake(18, 269, 175, 22)];
    demo.frame = CGRectMake(16, 12, 175, 22);
    demo.text = @"模拟器画面 · 非真实视频"; demo.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold]; demo.textColor = [UIColor colorWithWhite:1 alpha:0.75];
    [self.videoSurface addSubview:demo];
    self.rightRail = [[SSShortVideoRightInteractionView alloc] initWithFrame:CGRectMake(CGRectGetWidth(self.view.bounds) - 58, 440, 46, 358)];
    self.rightRail.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleTopMargin;
    [self.playerView addSubview:self.rightRail];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    self.playerView.frame = self.view.bounds;
    self.videoSurface.frame = CGRectMake(0, CGRectGetHeight(self.view.bounds) * 0.29, CGRectGetWidth(self.view.bounds), CGRectGetWidth(self.view.bounds) * 9.0 / 16.0);
    self.background.frame = self.videoSurface.bounds;
    self.rightRail.frame = CGRectMake(CGRectGetWidth(self.view.bounds) - 58, 440, 46, 358);
}
@end

static NSDictionary *NGTestPlaybackPresets(SSShortVideoRightInteractionView *rail) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    id savedXStyle = [defaults objectForKey:@"com.naiwaguo.playback.xStyle"];
    id savedPreset = [defaults objectForKey:NGPreferencePlaybackPreset];
    BOOL savedGlass = [defaults boolForKey:NGPreferencePlaybackGlass];
    NSString *collectNormalKey = @"com.naiwaguo.playback.custom.collect.normal";
    NSString *collectSelectedKey = @"com.naiwaguo.playback.custom.collect.selected";
    NSString *collectNormalImageKey = @"com.naiwaguo.playback.customImage.collect.normal";
    NSString *collectSelectedImageKey = @"com.naiwaguo.playback.customImage.collect.selected";
    id savedCollectNormal = [defaults objectForKey:collectNormalKey], savedCollectSelected = [defaults objectForKey:collectSelectedKey];
    id savedCollectNormalImage = [defaults objectForKey:collectNormalImageKey], savedCollectSelectedImage = [defaults objectForKey:collectSelectedImageKey];
    [defaults removeObjectForKey:collectNormalKey]; [defaults removeObjectForKey:collectSelectedKey];
    [defaults removeObjectForKey:collectNormalImageKey]; [defaults removeObjectForKey:collectSelectedImageKey];
    [defaults setBool:YES forKey:@"com.naiwaguo.playback.xStyle"];
    NGDecoratePlaybackInteraction(rail);
    NSMutableArray *actions = [NSMutableArray array];
    for (UIView *view in rail.subviews) if (![view isKindOfClass:NGPlaybackGlassView.class] && ![NGPlaybackRailItemIdentifier(view) isEqualToString:@"avatar"]) [actions addObject:view];
    NSMutableArray *images = [NSMutableArray array], *frames = [NSMutableArray array], *labelFrames = [NSMutableArray array], *alphas = [NSMutableArray array];
    for (UIView *action in actions) {
        [action layoutIfNeeded];
        UIImageView *icon = (UIImageView *)[action viewWithTag:100];
        [images addObject:icon.image];
        [alphas addObject:@(icon.alpha)];
        [frames addObject:[NSValue valueWithCGRect:action.frame]];
        [labelFrames addObject:[NSValue valueWithCGRect:[action viewWithTag:101].frame]];
    }
    BOOL fourPresets = actions.count == 4, labelsPreserved = YES, hitPreserved = YES, noDuplicates = YES, imagesDistinct = YES, glassIndependent = YES;
    NSMutableSet *firstIconImages = [NSMutableSet set];
    for (NSString *preset in NGPlaybackPresets()) {
        [defaults setObject:preset forKey:NGPreferencePlaybackPreset];
        NGDecoratePlaybackInteraction(rail);
        NGDecoratePlaybackInteraction(rail);
        for (NSUInteger index = 0; index < actions.count; index++) {
            UIView *action = actions[index];
            [action setNeedsLayout];
            [action layoutIfNeeded];
            UIImageView *icon = (UIImageView *)[action viewWithTag:100];
            NGPlaybackPresetState *state = objc_getAssociatedObject(action, &NGPlaybackPresetAssociation);
            NSUInteger overlays = 0;
            for (UIView *child in action.subviews) if ([child.accessibilityIdentifier isEqualToString:@"NaiwaGuo.Playback.IconPreset"]) overlays++;
            noDuplicates &= overlays == 0;
            if ([preset isEqualToString:@"original"]) fourPresets &= !state && icon.image == images[index] && icon.alpha == [alphas[index] doubleValue];
            else {
                fourPresets &= state != nil && icon.image != images[index] && icon.alpha == [alphas[index] doubleValue];
                if (index == 0) [firstIconImages addObject:UIImagePNGRepresentation(icon.image)];
            }
            labelsPreserved &= CGRectEqualToRect(action.frame, [frames[index] CGRectValue]) && CGRectEqualToRect([action viewWithTag:101].frame, [labelFrames[index] CGRectValue]);
            CGPoint point = CGPointMake(CGRectGetMidX(action.frame), action.frame.origin.y + 23);
            hitPreserved &= [rail hitTest:point withEvent:nil] == action;
        }
    }
    imagesDistinct = firstIconImages.count == 4;
    [defaults setBool:NO forKey:NGPreferencePlaybackGlass];
    NGDecoratePlaybackInteraction(rail);
    for (UIView *action in actions) {
        glassIndependent &= objc_getAssociatedObject(action, &NGPlaybackPresetAssociation) != nil && !objc_getAssociatedObject(action, &NGPlaybackGlassAssociationKey);
    }
    UIView *firstAction = actions.firstObject;
    UIImageView *firstIcon = (UIImageView *)[firstAction viewWithTag:100];
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(30, 30)];
    firstIcon.image = [[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [UIColor.systemRedColor setFill]; CGContextFillRect(context.CGContext, CGRectMake(0, 0, 30, 30)); }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    [firstAction setNeedsLayout];
    [firstAction layoutIfNeeded];
    NGPlaybackPresetState *coloredState = objc_getAssociatedObject(firstAction, &NGPlaybackPresetAssociation);
    BOOL hostColorUpdates = coloredState != nil && firstIcon.image != images.firstObject;
    firstIcon.image = images.firstObject;
    [defaults setObject:@"original" forKey:NGPreferencePlaybackPreset];
    NGDecoratePlaybackInteraction(rail);
    BOOL restores = YES;
    for (NSUInteger index = 0; index < actions.count; index++) {
        UIView *action = actions[index];
        UIImageView *icon = (UIImageView *)[action viewWithTag:100];
        restores &= !objc_getAssociatedObject(action, &NGPlaybackPresetAssociation) && icon.image == images[index] && icon.alpha == [alphas[index] doubleValue];
    }
    NGPlaybackPresetSettingsController *settings = [NGPlaybackPresetSettingsController new];
    [settings loadViewIfNeeded];
    [settings tableView:settings.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];
    UITableViewCell *selected = [settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];
    NSMutableSet *previewImages = [NSMutableSet set];
    BOOL previewMatchesPreset = YES;
    for (NSUInteger p = 1; p < NGPlaybackPresets().count; p++) {
        [settings tableView:settings.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:p inSection:0]];
        UITableViewCell *previewCell = [settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:1]];
        [previewImages addObject:UIImagePNGRepresentation(previewCell.imageView.image)];
        previewMatchesPreset &= [previewCell.detailTextLabel.text isEqual:NGPlaybackPresetTitles()[p]];
    }
    [settings tableView:settings.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:2 inSection:0]];
    BOOL settingSelects = previewMatchesPreset && previewImages.count == 4 && [NGPlaybackCurrentPreset() isEqualToString:@"bold"] && selected.accessoryType == UITableViewCellAccessoryCheckmark && selected.accessoryView == nil;
    [defaults setObject:@"heart.fill" forKey:@"com.naiwaguo.playback.custom.collect.selected"];
    firstIcon.highlighted = YES; NGApplyPlaybackPreset(firstAction, 0);
    BOOL selectedCustomIcon = [NGPlaybackCustomSymbol(0, YES) isEqualToString:@"heart.fill"] && firstIcon.image == firstIcon.highlightedImage && CGSizeEqualToSize(firstIcon.image.size, CGSizeMake(46, 46));
    UIGraphicsImageRenderer *stateRenderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(30, 30)];
    UIImage *hostNormalImage = [stateRenderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [UIColor.systemGreenColor setFill]; UIRectFill(CGRectMake(0, 0, 30, 30)); }];
    UIImage *hostSelectedImage = [stateRenderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [UIColor.systemPurpleColor setFill]; UIRectFill(CGRectMake(0, 0, 30, 30)); }];
    [defaults setObject:@"heart.fill" forKey:@"com.naiwaguo.playback.custom.collect.selected"];
    [defaults setObject:@"original" forKey:NGPreferencePlaybackPreset];
    firstIcon.highlighted = NO; firstIcon.image = hostNormalImage; NGApplyPlaybackPreset(firstAction, 0);
    firstIcon.highlighted = YES; firstIcon.image = hostSelectedImage; NGApplyPlaybackPreset(firstAction, 0);
    NGPlaybackPresetState *interactionState = objc_getAssociatedObject(firstAction, &NGPlaybackPresetAssociation);
    BOOL capturedSelectedHostImage = interactionState.originalImages[@YES] == hostSelectedImage;
    UIImage *selectedOriginalExpected = interactionState.originalImages[@YES] == NSNull.null ? nil : interactionState.originalImages[@YES];
    [defaults removeObjectForKey:collectNormalKey]; [defaults removeObjectForKey:collectSelectedKey];
    [defaults removeObjectForKey:collectNormalImageKey]; [defaults removeObjectForKey:collectSelectedImageKey];
    NGApplyPlaybackPreset(firstAction, 0);
    BOOL cancelRestoresSelectedImage = firstIcon.image == hostSelectedImage;
    BOOL cancelClearsPresetState = !objc_getAssociatedObject(firstAction, &NGPlaybackPresetAssociation);
    firstIcon.highlighted = NO; firstIcon.image = hostNormalImage; NGApplyPlaybackPreset(firstAction, 0);
    BOOL tapStillUpdatesAfterCancel = firstIcon.image == hostNormalImage;
    if (savedCollectNormal) [defaults setObject:savedCollectNormal forKey:collectNormalKey];
    if (savedCollectSelected) [defaults setObject:savedCollectSelected forKey:collectSelectedKey];
    if (savedCollectNormalImage) [defaults setObject:savedCollectNormalImage forKey:collectNormalImageKey];
    if (savedCollectSelectedImage) [defaults setObject:savedCollectSelectedImage forKey:collectSelectedImageKey];
    firstIcon.highlighted = NO;
    if (savedPreset) [defaults setObject:savedPreset forKey:NGPreferencePlaybackPreset]; else [defaults removeObjectForKey:NGPreferencePlaybackPreset];
    [defaults setBool:savedGlass forKey:NGPreferencePlaybackGlass];
    if (savedXStyle) [defaults setObject:savedXStyle forKey:@"com.naiwaguo.playback.xStyle"]; else [defaults removeObjectForKey:@"com.naiwaguo.playback.xStyle"];
    NGDecoratePlaybackInteraction(rail);
    return @{@"fourPresets": @(fourPresets), @"distinctImages": @(imagesDistinct), @"originalRestores": @(restores), @"labelsPreserved": @(labelsPreserved), @"hitPreserved": @(hitPreserved), @"noDuplicates": @(noDuplicates), @"glassIndependent": @(glassIndependent), @"hostColorUpdates": @(hostColorUpdates), @"settingsSelects": @(settingSelects), @"selectedStateCustomIcon": @(selectedCustomIcon), @"capturedSelectedHostImage": @(capturedSelectedHostImage), @"selectedBaselineMatches": @(selectedOriginalExpected == hostSelectedImage), @"selectedImageRestored": @(cancelRestoresSelectedImage), @"presetStateCleared": @(cancelClearsPresetState), @"selectedHostRestoredAfterCancel": @(cancelRestoresSelectedImage && cancelClearsPresetState), @"toggleRestoresAfterCancel": @(cancelRestoresSelectedImage && tapStillUpdatesAfterCancel)};
}

static NSDictionary *NGTestCommentGlass(void) {
    UIView *host = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 390, 800)];
    UIView *root = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 390, 700)]; root.backgroundColor = UIColor.whiteColor;
    FQCShortVideoCommentPanelTitleView *title = [[FQCShortVideoCommentPanelTitleView alloc] initWithFrame:CGRectMake(0, 0, 390, 64)];
    UICollectionViewFlowLayout *layout = [UICollectionViewFlowLayout new];
    FQCShortVideoCommentListCollectionView *list = [[FQCShortVideoCommentListCollectionView alloc] initWithFrame:CGRectMake(0, 64, 390, 580) collectionViewLayout:layout]; list.backgroundColor = UIColor.whiteColor;
    UIView *listWrapper = [[UIView alloc] initWithFrame:CGRectMake(0, 64, 390, 580)];
    list.frame = listWrapper.bounds;
    [host addSubview:root]; [root addSubview:title]; [root addSubview:listWrapper]; [listWrapper addSubview:list];
    FQCShortVideoBottomToolBar *toolbar = [[FQCShortVideoBottomToolBar alloc] initWithFrame:CGRectMake(0, 644, 390, 56)];
    toolbar.backgroundColor = UIColor.whiteColor;
    UIVisualEffect *originalEffect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleRegular];
    UIVisualEffectView *nativeBlur = [[UIVisualEffectView alloc] initWithEffect:originalEffect]; nativeBlur.frame = toolbar.bounds; [toolbar addSubview:nativeBlur];
    UIView *field = [[UIView alloc] initWithFrame:CGRectMake(16, 10, 280, 36)]; field.backgroundColor = UIColor.lightGrayColor; [toolbar addSubview:field];
    [root addSubview:toolbar];
    UIColor *rootBefore = root.backgroundColor, *listBefore = list.backgroundColor;
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceCommentGlass];
    NGDecorateCommentSource(title);
    UIVisualEffectView *glass = objc_getAssociatedObject(root, &NGCommentGlassAssociation);
    BOOL materialApplied = glass && glass.superview == root && !glass.userInteractionEnabled && [root.backgroundColor isEqual:UIColor.clearColor] && [list.backgroundColor isEqual:UIColor.clearColor];
    BOOL touchThrough = glass && [[root hitTest:CGPointMake(10, 100) withEvent:nil] isDescendantOfView:list];
    BOOL inputMaterial = [toolbar.backgroundColor isEqual:UIColor.clearColor] && [field.backgroundColor isEqual:UIColor.clearColor] &&
        nativeBlur.effect == nil && CGRectEqualToRect(glass.frame, root.bounds);
    [NSUserDefaults.standardUserDefaults setFloat:0.6 forKey:NGPreferenceCommentGlassOpacity]; NGDecorateCommentSource(list);
    BOOL opacityUpdated = fabs(CGColorGetAlpha(glass.contentView.backgroundColor.CGColor) - 0.6) < 0.01;
    [NSUserDefaults.standardUserDefaults removeObjectForKey:NGPreferenceCommentGlassOpacity];
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceCommentGlass]; NGUpdateCommentGlass();
    BOOL restored = !objc_getAssociatedObject(root, &NGCommentGlassAssociation) && [root.backgroundColor isEqual:rootBefore] &&
        [list.backgroundColor isEqual:listBefore] && nativeBlur.effect == originalEffect &&
        [toolbar.backgroundColor isEqual:UIColor.whiteColor] && [field.backgroundColor isEqual:UIColor.lightGrayColor];
    return @{@"materialApplied": @(materialApplied), @"touchThrough": @(touchThrough), @"restoresBackgrounds": @(restored), @"inputMaterial": @(inputMaterial), @"opacityUpdated": @(opacityUpdated)};
}

static NSDictionary *NGTestShareGlass(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    BOOL saved = [defaults boolForKey:NGPreferenceShareGlass];
    [defaults setBool:YES forKey:NGPreferenceShareGlass];
    SSUGKmpSharePanelContainerViewController *share = [SSUGKmpSharePanelContainerViewController new];
    [share loadViewIfNeeded];
    share.view.backgroundColor = UIColor.whiteColor;
    UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(20, 20, 220, 30)];
    label.text = @"分享至";
    label.textColor = UIColor.blackColor;
    [share.view addSubview:label];
    NGDecorateShareController(share);
    UIVisualEffectView *glass = objc_getAssociatedObject(share.view, &NGCommentGlassAssociation);
    BOOL systemGlass = glass && glass.overrideUserInterfaceStyle == UIUserInterfaceStyleUnspecified &&
        [share.view.backgroundColor isEqual:UIColor.clearColor];
    BOOL systemTextColorPreserved = [label.textColor isEqual:UIColor.blackColor];
    [defaults setBool:NO forKey:NGPreferenceShareGlass];
    NGUpdateCommentGlass();
    BOOL restored = !objc_getAssociatedObject(share.view, &NGCommentGlassAssociation) &&
        [share.view.backgroundColor isEqual:UIColor.whiteColor] && [label.textColor isEqual:UIColor.blackColor];
    [defaults setBool:saved forKey:NGPreferenceShareGlass];
    return @{@"followsSystemAppearance": @(systemGlass && systemTextColorPreserved), @"restores": @(restored)};
}

static NSDictionary *NGTestHeaderRestore(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSArray *legacy = @[@"com.naiwaguo.playback.hidden.header.follow", @"com.naiwaguo.playback.hidden.header.manju", @"com.naiwaguo.playback.hidden.header.live", @"com.naiwaguo.playback.hidden.header.recommended"];
    id savedMigration = [defaults objectForKey:NGPreferenceChannelRestoreMigration];
    NSMutableDictionary *savedValues = [NSMutableDictionary dictionary];
    for (NSString *key in legacy) { savedValues[key] = [defaults objectForKey:key] ?: NSNull.null; [defaults setBool:YES forKey:key]; }
    [defaults removeObjectForKey:NGPreferenceChannelRestoreMigration];
    NGRestoreLegacyTopChannels(defaults);
    BOOL oldPreferencesCleared = YES;
    for (NSString *key in legacy) oldPreferencesCleared &= [defaults objectForKey:key] == nil;
    [defaults setBool:YES forKey:legacy.lastObject];
    NGRestoreLegacyTopChannels(defaults);
    BOOL oneTimeMigration = [defaults boolForKey:legacy.lastObject];

    SSVideoSeriesFeedHeaderView *header = [[SSVideoSeriesFeedHeaderView alloc] initWithFrame:CGRectMake(0, 0, 360, 44)];
    SSHGBookStoreCategoryTitleView *channels = [[SSHGBookStoreCategoryTitleView alloc] initWithFrame:CGRectMake(68, 0, 266, 44)];
    [header addSubview:channels];
    UIView *collection = [[UIView alloc] initWithFrame:channels.bounds];
    [channels addSubview:collection];
    FQRJXCategoryIndicatorLineView *indicator = [[FQRJXCategoryIndicatorLineView alloc] initWithFrame:CGRectMake(130, 40, 16, 2)];
    [collection addSubview:indicator];
    NSMutableArray<UIView *> *cells = [NSMutableArray array];
    for (NSUInteger index = 0; index < 5; index++) {
        SSHGBookStoreCategoryTitleCell *cell = [[SSHGBookStoreCategoryTitleCell alloc] initWithFrame:CGRectMake(index * 50, 0, 44, 44)];
        UILabel *title = [[UILabel alloc] initWithFrame:cell.bounds];
        title.text = @[@"关注", @"漫剧", @"真人剧", @"推荐", @"其他"][index];
        [cell addSubview:title]; [collection addSubview:cell]; [cells addObject:cell];
    }
    CGRect sourceFrame = channels.frame;
    [defaults setBool:NO forKey:NGPreferencePlaybackHeaderGlass];
    NGDecoratePlaybackHeader(header);
    BOOL categoryViewsPreserved = !indicator.hidden && CGRectEqualToRect(channels.frame, sourceFrame);
    for (UIView *cell in cells) categoryViewsPreserved &= !cell.hidden;
    BOOL channelHidingRemoved = YES;
    for (NSArray *section in NGPlaybackVisibilityItems()) for (NSDictionary *item in section) channelHidingRemoved &= ![item[@"key"] hasPrefix:@"header."] || [item[@"key"] isEqual:@"header.sidebar"] || [item[@"key"] isEqual:@"header.search"];

    if (savedMigration) [defaults setObject:savedMigration forKey:NGPreferenceChannelRestoreMigration]; else [defaults removeObjectForKey:NGPreferenceChannelRestoreMigration];
    for (NSString *key in legacy) { id saved = savedValues[key]; if (saved == NSNull.null) [defaults removeObjectForKey:key]; else [defaults setObject:saved forKey:key]; }
    return @{@"restoresOriginalChannelLayout": @(categoryViewsPreserved), @"removesChannelHideOptions": @(channelHidingRemoved), @"clearsOldHiddenChannelsOnce": @(oldPreferencesCleared && oneTimeMigration)};
}

static NSDictionary *NGTestFullscreenAndPreview(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    BOOL savedLegacyFullscreen = [defaults boolForKey:@"com.naiwaguo.playback.fullscreen"];
    id savedPanelOrigin = [defaults objectForKey:@"com.naiwaguo.preview.panelOrigin"];
    UIViewController *owner = [UIViewController new];
    owner.view.frame = CGRectMake(0, 0, 402, 874);
    FQVCollectionView *collection = [[FQVCollectionView alloc] initWithFrame:CGRectMake(0, 0, 402, 784) collectionViewLayout:[UICollectionViewFlowLayout new]];
    FQVShortVideoCell *cell = [[FQVShortVideoCell alloc] initWithFrame:collection.bounds];
    FQVShortVideoPlayerCellView *player = [[FQVShortVideoPlayerCellView alloc] initWithFrame:cell.bounds];
    [owner.view addSubview:collection]; [collection addSubview:cell]; [cell addSubview:player];
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:NGBaseWindow().windowScene];
    window.rootViewController = owner;
    window.frame = owner.view.bounds;
    [window makeKeyAndVisible];
    [owner.view layoutIfNeeded];
    CGRect collectionFrame = collection.frame, cellFrame = cell.frame, playerFrame = player.frame;
    [defaults setBool:YES forKey:@"com.naiwaguo.playback.fullscreen"];
    BOOL geometryUnchanged = CGRectEqualToRect(collection.frame, collectionFrame) && CGRectEqualToRect(cell.frame, cellFrame) && CGRectEqualToRect(player.frame, playerFrame);
    BOOL playbackDetected = NGVisiblePlaybackPlayer() != nil;
    NGPresentLivePreview();
    BOOL presented = NGLivePreviewWindow != nil && NGLivePreviewWindow.rootViewController != nil;
    NSUInteger sliders = 0;
    NSMutableArray *queue = [NSMutableArray arrayWithObject:NGLivePreviewWindow.rootViewController.view];
    for (NSUInteger index = 0; index < queue.count && index < 300; index++) {
        UIView *view = queue[index];
        if ([view isKindOfClass:UISlider.class]) sliders++;
        [queue addObjectsFromArray:view.subviews];
    }
    NGLivePreviewController *preview = (NGLivePreviewController *)NGLivePreviewWindow.rootViewController;
    preview.view.frame = window.bounds;
    [preview.view setNeedsLayout]; [preview.view layoutIfNeeded];
    BOOL hasDragHandle = preview.dragHandle.gestureRecognizers.count > 0;
    CGPoint startOrigin = preview.panel.frame.origin;
    NGTestPanGestureRecognizer *pan = [NGTestPanGestureRecognizer new];
    pan.testState = UIGestureRecognizerStateBegan;
    [preview panelDragged:pan];
    pan.testState = UIGestureRecognizerStateChanged;
    pan.testTranslation = CGPointMake(17.5, -11.25);
    [preview panelDragged:pan];
    CGPoint firstDragOrigin = preview.panel.frame.origin;
    [preview.view setNeedsLayout]; [preview.view layoutIfNeeded];
    BOOL dragContinuesThroughLayout = fabs(preview.panel.frame.origin.x - firstDragOrigin.x) < 0.1 && fabs(preview.panel.frame.origin.y - firstDragOrigin.y) < 0.1;
    pan.testTranslation = CGPointMake(46.25, 23.5);
    [preview panelDragged:pan];
    CGPoint movedOrigin = preview.panel.frame.origin;
    [preview.view setNeedsLayout]; [preview.view layoutIfNeeded];
    BOOL secondDragContinues = fabs(preview.panel.frame.origin.x - movedOrigin.x) < 0.1 && fabs(preview.panel.frame.origin.y - movedOrigin.y) < 0.1 &&
        movedOrigin.x > firstDragOrigin.x && movedOrigin.y > firstDragOrigin.y;
    pan.testState = UIGestureRecognizerStateEnded;
    [preview panelDragged:pan];
    [preview.view setNeedsLayout]; [preview.view layoutIfNeeded];
    BOOL panelMovesAndPersists = dragContinuesThroughLayout && secondDragContinues &&
        fabs(preview.panel.frame.origin.x - movedOrigin.x) < 0.1 && fabs(preview.panel.frame.origin.y - movedOrigin.y) < 0.1 &&
        fabs(CGPointFromString([defaults stringForKey:@"com.naiwaguo.preview.panelOrigin"]).x - movedOrigin.x) < 0.1 &&
        fabs(CGPointFromString([defaults stringForKey:@"com.naiwaguo.preview.panelOrigin"]).y - movedOrigin.y) < 0.1;
    NGDismissLivePreview();
    BOOL dismissed = NGLivePreviewWindow == nil;
    window.hidden = YES;
    if (savedLegacyFullscreen) [defaults setBool:YES forKey:@"com.naiwaguo.playback.fullscreen"]; else [defaults removeObjectForKey:@"com.naiwaguo.playback.fullscreen"];
    if (savedPanelOrigin) [defaults setObject:savedPanelOrigin forKey:@"com.naiwaguo.preview.panelOrigin"];
    else [defaults removeObjectForKey:@"com.naiwaguo.preview.panelOrigin"];
    BOOL fullscreenSettingPresent = NO;
    for (NSDictionary *group in NGSettingsGroups()) for (NSDictionary *row in group[@"rows"])
        fullscreenSettingPresent |= [row[@"title"] isEqual:@"全屏播放"] && [row[@"key"] isEqual:NGPreferencePlaybackFullscreen];
    return @{@"fullscreenSettingPresent": @(fullscreenSettingPresent), @"geometryUnchanged": @(geometryUnchanged), @"playbackDetected": @(playbackDetected),
             @"previewPresented": @(presented), @"previewSliderCount": @(sliders), @"previewDismissed": @(dismissed), @"previewDragHandle": @(hasDragHandle), @"previewMovesAndPersists": @(panelMovesAndPersists), @"dragSurvivesLayout": @(dragContinuesThroughLayout && secondDragContinues), @"panelStartOrigin": NSStringFromCGPoint(startOrigin), @"panelFrameAfterMove": NSStringFromCGRect(preview.panel.frame), @"expectedPanelOrigin": NSStringFromCGPoint(movedOrigin)};
}

static NSDictionary *NGTestPlaybackRailLayout(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    id savedXStyle = [defaults objectForKey:@"com.naiwaguo.playback.xStyle"];
    [defaults setBool:NO forKey:@"com.naiwaguo.playback.xStyle"];
    UIView *rail = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 46, 260)];
    NSArray *classes = @[[SSShortVideoRightInteractionAvatarView class],[SSShortVideoRightInteractionCollectView class],[SSShortVideoRightInteractionCommentView class],[SSShortVideoRightInteractionLikeView class],[SSShortVideoRightInteractionShareView class]];
    NSMutableArray *items = [NSMutableArray array];
    for (NSUInteger index = 0; index < classes.count; index++) { UIView *item = [[classes[index] alloc] initWithFrame:CGRectMake(0, index * 50, 46, 40)]; [rail addSubview:item]; [items addObject:item]; }
    CGFloat oldAvatarY = ((UIView *)items[0]).frame.origin.y;
    [defaults setBool:YES forKey:@"com.naiwaguo.playback.hidden.collect"];
    [defaults setFloat:17 forKey:NGPreferencePlaybackRailX]; [defaults setFloat:9 forKey:NGPreferencePlaybackRailY]; [defaults setFloat:1.2 forKey:NGPreferencePlaybackRailScale];
    NGApplyPlaybackRailLayout(rail);
    BOOL reflows = ((UIView *)items[1]).hidden && ((UIView *)items[0]).frame.origin.y > oldAvatarY;
    BOOL wholeRailMoves = fabs(rail.transform.tx - 17) < 0.1 && fabs(rail.transform.ty - 9) < 0.1 && fabs(rail.transform.a - 1.2) < 0.1;
    [defaults setBool:NO forKey:@"com.naiwaguo.playback.hidden.collect"];
    [defaults removeObjectForKey:NGPreferencePlaybackRailX]; [defaults removeObjectForKey:NGPreferencePlaybackRailY]; [defaults removeObjectForKey:NGPreferencePlaybackRailScale];
    NGApplyPlaybackRailLayout(rail);
    BOOL restores = !((UIView *)items[1]).hidden && fabs(rail.transform.a - 1) < 0.01 && CGRectGetMinY(((UIView *)items[0]).frame) < CGRectGetMinY(((UIView *)items[1]).frame);
    [defaults setBool:YES forKey:NGPreferencePlaybackGlass]; NGDecoratePlaybackInteraction(rail);
    BOOL avatarGlass = objc_getAssociatedObject(items[0], &NGPlaybackGlassAssociationKey) != nil;
    if (savedXStyle) [defaults setObject:savedXStyle forKey:@"com.naiwaguo.playback.xStyle"]; else [defaults removeObjectForKey:@"com.naiwaguo.playback.xStyle"];
    return @{@"hiddenRowsCollapse":@(reflows),@"movesAndScalesAsOne":@(wholeRailMoves),@"restores":@(restores),@"avatarGlass":@(avatarGlass)};
}

static NSDictionary *NGTestPlaybackXStyle(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSString *legacyKey = @"com.naiwaguo.playback.xStyle";
    id saved = [defaults objectForKey:legacyKey];
    [defaults setBool:YES forKey:legacyKey];
    UIView *rail = [[UIView alloc] initWithFrame:CGRectMake(330, 400, 46, 284)];
    UIView *parent = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 402, 874)]; [parent addSubview:rail];
    NSArray *classes = @[[SSShortVideoRightInteractionCollectView class], [SSShortVideoRightInteractionCommentView class], [SSShortVideoRightInteractionLikeView class], [SSShortVideoRightInteractionShareView class]];
    for (NSUInteger i = 0; i < classes.count; i++) [rail addSubview:[[classes[i] alloc] initWithFrame:CGRectMake(0, i * 70, 46, 62)]];
    CGRect original = rail.frame;
    NGDecoratePlaybackInteraction(rail);
    BOOL untouched = CGRectEqualToRect(rail.frame, original);
    BOOL absent = YES;
    for (NSDictionary *group in NGSettingsGroups()) for (NSDictionary *row in group[@"rows"]) absent &= ![row[@"key"] isEqualToString:legacyKey];
    if (saved) [defaults setObject:saved forKey:legacyKey]; else [defaults removeObjectForKey:legacyKey];
    return @{@"legacyPreferenceIgnored": @(untouched), @"settingRemoved": @(absent)};
}

static NSDictionary *NGTestPhotoIcons(void) {
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(900, 450)];
    UIImage *photo = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [UIColor.systemGreenColor setFill]; UIRectFill(CGRectMake(0, 0, 900, 450)); }];
    NSData *data = UIImagePNGRepresentation(NGIconImageOnCanvas(photo, 128, 128));
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSString *dockKey = NGDockIconImageKey(0), *playbackKey = NGPlaybackCustomImageKey(0, NO);
    id savedDock = [defaults objectForKey:dockKey], savedPlayback = [defaults objectForKey:playbackKey];
    [defaults setObject:data forKey:dockKey]; [defaults setObject:data forKey:playbackKey];
    UIImage *dockImage = NGDockIcon(0, NO);
    SSShortVideoRightInteractionCollectView *action = [[SSShortVideoRightInteractionCollectView alloc] initWithFrame:CGRectMake(0, 0, 46, 62)]; [action layoutIfNeeded]; NGApplyPlaybackPreset(action, 0);
    UIImageView *icon = (UIImageView *)[action viewWithTag:100];
    BOOL playbackLoaded = CGSizeEqualToSize(icon.image.size, CGSizeMake(46, 46)) && icon.image.renderingMode == UIImageRenderingModeAlwaysOriginal;
    if (savedDock) [defaults setObject:savedDock forKey:dockKey]; else [defaults removeObjectForKey:dockKey];
    if (savedPlayback) [defaults setObject:savedPlayback forKey:playbackKey]; else [defaults removeObjectForKey:playbackKey];
    return @{@"boundedStorage": @(data.length > 0 && data.length < 100000), @"dockImageSize": @(CGSizeEqualToSize(dockImage.size, CGSizeMake(28, 28))), @"playbackImageLoaded": @(playbackLoaded)};
}

static NSDictionary *NGTestPlaybackTextLayout(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSString *prefix = @"com.naiwaguo.playback.layout.author";
    [defaults setFloat:12 forKey:[prefix stringByAppendingString:@".x"]]; [defaults setFloat:1.2 forKey:[prefix stringByAppendingString:@".scale"]];
    SSShortVideoOutterSeriesInfoView *info = [SSShortVideoOutterSeriesInfoView new];
    UIView *row = [UIView new]; UILabel *author = [UILabel new]; author.text = @"@作者"; [row addSubview:author]; [info addSubview:row];
    NGDecoratePlaybackTextView(info); NGDecoratePlaybackTextView(info);
    BOOL singleTransform = fabs(row.transform.tx - 12) < 0.01 && fabs(row.transform.a - 1.2) < 0.01 && CGAffineTransformIsIdentity(author.transform);
    [defaults removeObjectForKey:[prefix stringByAppendingString:@".x"]]; [defaults removeObjectForKey:[prefix stringByAppendingString:@".scale"]]; NGDecoratePlaybackTextView(info);
    BOOL reset = CGAffineTransformIsIdentity(row.transform);
    SSShortVideoAiUsageView *notice = [SSShortVideoAiUsageView new]; notice.hidden = YES; NGDecoratePlaybackTextView(notice);
    return @{@"singleTransform": @(singleTransform), @"reset": @(reset), @"preservesHostVisibility": @(notice.hidden)};
}

static NSDictionary *NGTestNativeCommentGeometry(void) {
    // Even an old persisted YES must not intercept the host's comment transition.
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:@"com.naiwaguo.playback.commentVideoFreeze"];
    FQVShortVideoPlayerCellView *video = [[FQVShortVideoPlayerCellView alloc] initWithFrame:CGRectMake(0, 0, 402, 784)];
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:NGBaseWindow().windowScene]; window.rootViewController = [UIViewController new]; window.hidden = NO;
    [window.rootViewController.view addSubview:video];
    FQCShortVideoCommentPanelTitleView *panel = [[FQCShortVideoCommentPanelTitleView alloc] initWithFrame:CGRectMake(0, 300, 402, 400)]; [window.rootViewController.view addSubview:panel];
    video.frame = CGRectMake(20, 0, 362, 300); [video layoutIfNeeded];
    BOOL native = CGRectEqualToRect(video.frame, CGRectMake(20, 0, 362, 300));
    video.transform = CGAffineTransformMakeScale(0.6, 0.6);
    BOOL transform = CGAffineTransformEqualToTransform(video.transform, CGAffineTransformMakeScale(0.6, 0.6));
    window.hidden = YES; [NSUserDefaults.standardUserDefaults removeObjectForKey:@"com.naiwaguo.playback.commentVideoFreeze"];
    return @{@"nativeCommentResizeAllowed": @(native), @"nativeCommentTransformAllowed": @(transform)};
}

static NSDictionary *NGTestPlaybackHeaderGlass(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    id saved = [defaults objectForKey:NGPreferencePlaybackHeaderGlass];
    SSVideoSeriesFeedHeaderView *header = [[SSVideoSeriesFeedHeaderView alloc] initWithFrame:CGRectMake(0, 0, 402, 44)];
    UIView *content = [[UIView alloc] initWithFrame:header.bounds];
    [header addSubview:content];
    SSVideoSeriesFeedHeaderSearchView *search = [[SSVideoSeriesFeedHeaderSearchView alloc] initWithFrame:content.bounds];
    [content addSubview:search];
    SSHGBookStoreCategoryTitleView *channels = [[SSHGBookStoreCategoryTitleView alloc] initWithFrame:CGRectMake(68, 0, 266, 44)];
    channels.clipsToBounds = YES;
    [content addSubview:channels];
    FQRJXCategoryCollectionView *channelItems = [[FQRJXCategoryCollectionView alloc] initWithFrame:channels.bounds];
    [channels addSubview:channelItems];
    NSArray *channelTitles = @[@"关注", @"漫剧", @"真人剧", @"推荐"];
    for (NSUInteger index = 0; index < channelTitles.count; index++) {
        UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(index * 66.5, 0, 66.5, 44)];
        label.text = channelTitles[index]; label.textColor = UIColor.whiteColor;
        label.font = [UIFont systemFontOfSize:17 weight:index == 3 ? UIFontWeightBold : UIFontWeightRegular];
        label.textAlignment = NSTextAlignmentCenter;
        [channelItems addSubview:label];
    }
    FQReaderSaaSBaseImageView *sidebar = [[FQReaderSaaSBaseImageView alloc] initWithFrame:CGRectMake(16, 10, 24, 24)];
    sidebar.image = [UIImage systemImageNamed:@"line.3.horizontal"]; sidebar.tintColor = UIColor.whiteColor;
    sidebar.userInteractionEnabled = YES;
    [content addSubview:sidebar];
    FQReaderSaaSBaseImageView *searchIcon = [[FQReaderSaaSBaseImageView alloc] initWithFrame:CGRectMake(362, 10, 24, 24)];
    searchIcon.image = [UIImage systemImageNamed:@"magnifyingglass"]; searchIcon.tintColor = UIColor.whiteColor;
    searchIcon.userInteractionEnabled = YES;
    [search addSubview:searchIcon];

    [defaults setBool:YES forKey:NGPreferencePlaybackHeaderGlass];
    NGDecoratePlaybackHeader(header);
    NGDecoratePlaybackHeader(header);
    UIView *channelGlass = objc_getAssociatedObject(channelItems, &NGPlaybackHeaderGlassAssociation) ? ((NGPlaybackHeaderGlassState *)objc_getAssociatedObject(channelItems, &NGPlaybackHeaderGlassAssociation)).glass : nil;
    UIView *sidebarGlass = objc_getAssociatedObject(sidebar, &NGPlaybackHeaderGlassAssociation) ? ((NGPlaybackHeaderGlassState *)objc_getAssociatedObject(sidebar, &NGPlaybackHeaderGlassAssociation)).glass : nil;
    UIView *searchGlass = objc_getAssociatedObject(searchIcon, &NGPlaybackHeaderGlassAssociation) ? ((NGPlaybackHeaderGlassState *)objc_getAssociatedObject(searchIcon, &NGPlaybackHeaderGlassAssociation)).glass : nil;
    CGPoint channelPoint = [channelItems convertPoint:CGPointMake(CGRectGetMidX(channelItems.bounds), CGRectGetMidY(channelItems.bounds)) toView:content];
    CGPoint sidebarPoint = CGPointMake(CGRectGetMidX(sidebar.frame), CGRectGetMidY(sidebar.frame));
    CGPoint searchPoint = [searchIcon convertPoint:CGPointMake(CGRectGetMidX(searchIcon.bounds), CGRectGetMidY(searchIcon.bounds)) toView:search];
    BOOL eachHasGlass = channelGlass && sidebarGlass && searchGlass &&
        [channelGlass.accessibilityIdentifier hasSuffix:@"Channels"] &&
        [sidebarGlass.accessibilityIdentifier hasSuffix:@"Sidebar"] &&
        [searchGlass.accessibilityIdentifier hasSuffix:@"Search"];
    BOOL originalTouches = [content hitTest:channelPoint withEvent:nil] == channelItems &&
        [content hitTest:sidebarPoint withEvent:nil] == sidebar && [search hitTest:searchPoint withEvent:nil] == searchIcon;
    NSUInteger glassCount = 0;
    NSMutableArray<UIView *> *views = [NSMutableArray arrayWithObject:header];
    for (NSUInteger index = 0; index < views.count && index < 100; index++) {
        UIView *view = views[index];
        if ([view.accessibilityIdentifier hasPrefix:@"NaiwaGuo.Playback.HeaderGlass."]) glassCount++;
        [views addObjectsFromArray:view.subviews];
    }
    BOOL unique = glassCount == 3 && search.subviews.count == 2 && channels.subviews.count == 1;
    BOOL matchingHeight = fabs(channelGlass.bounds.size.height - sidebarGlass.bounds.size.height) < 0.01 && fabs(channelGlass.bounds.size.height - searchGlass.bounds.size.height) < 0.01 && fabs(channelGlass.layer.cornerRadius - channelGlass.bounds.size.height / 2) < 0.01;
    [defaults setBool:NO forKey:NGPreferencePlaybackHeaderGlass];
    NGDecoratePlaybackHeader(header);
    BOOL removesWhenDisabled = !objc_getAssociatedObject(channelItems, &NGPlaybackHeaderGlassAssociation) &&
        !objc_getAssociatedObject(sidebar, &NGPlaybackHeaderGlassAssociation) && !objc_getAssociatedObject(searchIcon, &NGPlaybackHeaderGlassAssociation);
    [defaults setBool:YES forKey:NGPreferencePlaybackHeaderGlass];
    NGDecoratePlaybackHeader(header);
    BOOL restoresWhenEnabled = objc_getAssociatedObject(channelItems, &NGPlaybackHeaderGlassAssociation) &&
        objc_getAssociatedObject(sidebar, &NGPlaybackHeaderGlassAssociation) && objc_getAssociatedObject(searchIcon, &NGPlaybackHeaderGlassAssociation);
    UIWindow *previewWindow = [[UIWindow alloc] initWithWindowScene:NGBaseWindow().windowScene];
    previewWindow.frame = CGRectMake(0, 0, 402, 112);
    previewWindow.rootViewController = [UIViewController new];
    UIView *preview = previewWindow.rootViewController.view;
    preview.backgroundColor = [UIColor colorWithRed:0.22 green:0.45 blue:0.38 alpha:1];
    header.frame = CGRectMake(0, 34, 402, 44);
    [preview addSubview:header]; previewWindow.hidden = NO;
    [preview layoutIfNeeded]; NGDecoratePlaybackHeader(header);
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithBounds:preview.bounds];
    UIImage *headerPreview = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [preview drawViewHierarchyInRect:preview.bounds afterScreenUpdates:YES]; }];
    NSString *documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    [UIImagePNGRepresentation(headerPreview) writeToFile:[documents stringByAppendingPathComponent:@"header.png"] atomically:YES];
    previewWindow.hidden = YES;
    if (saved) [defaults setObject:saved forKey:NGPreferencePlaybackHeaderGlass];
    else [defaults removeObjectForKey:NGPreferencePlaybackHeaderGlass];
    return @{@"matchesSideButtonHeight": @(matchingHeight), @"threeSeparateContainers": @(eachHasGlass), @"originalTouches": @(originalTouches), @"noDuplicates": @(unique), @"toggleRestores": @(removesWhenDisabled && restoresWhenEnabled)};
}

#include "NGRegression073Tests.inc"
#include "NGSettings001Tests.inc"
#include "NGRegression002Tests.inc"
#include "NGRegression003Tests.inc"
#include "NGDebug003Tests.inc"
#include "NGRegression005Tests.inc"
#include "NGContinuation005Tests.inc"
#include "NGRegression006Tests.inc"
#include "NGRegression007Tests.inc"
#include "NGRegression009Tests.inc"

static void NGWriteTestResult(NSDictionary *result) {
    NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSData *data = [NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:nil];
    [data writeToFile:[documents stringByAppendingPathComponent:@"result.json"] atomically:YES];
}

@interface NGTestAppDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

@implementation NGTestAppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    return YES;
}
@end

@interface NGTestSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

@implementation NGTestSceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    self.window.rootViewController = [NGTestRootController new];
    [self.window makeKeyAndVisible];

    NSArray *arguments = NSProcessInfo.processInfo.arguments;
    NGDockForceLegacyTest=[arguments containsObject:@"--legacy-dock006"];
    for (NSString *mode in @[@"settings-preview009",@"native-entry009",@"sponsor-never-seed009",@"sponsor-never-check009",@"unrestricted-restart009"]) if ([arguments containsObject:[@"--" stringByAppendingString:mode]]) { NGRunSettingsUI009(self.window,mode);return; }
    if ([arguments containsObject:@"--fresh-defaults005"] || [arguments containsObject:@"--retained-defaults005"]) {
        NGStart();
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.1*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NGRunDefaults005(self.window,[arguments containsObject:@"--retained-defaults005"]); });
        return;
    }
    for (NSString *mode in @[@"debug-page-003",@"debug-audio-003",@"debug-ui-003",@"debug-late-003"]) if ([arguments containsObject:[@"--" stringByAppendingString:mode]]) { NGRunDebug003(self.window,mode); return; }
    if ([arguments containsObject:@"--stress-font003"]) {
        NGStart(); [NSUserDefaults.standardUserDefaults setDouble:0.8 forKey:NGPreferenceFontScale]; [NSUserDefaults.standardUserDefaults setObject:@"Georgia" forKey:NGPreferenceFontName]; NGRefreshTypography();
        NGFontStressController *profile=[NGFontStressController new]; profile.title=@"我的"; profile.tableView.rowHeight=UITableViewAutomaticDimension; profile.tableView.estimatedRowHeight=70;
        UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:profile]; self.window.rootViewController=nav;
        __block NSUInteger beats=0; __block CFTimeInterval previous=CACurrentMediaTime(),maxGap=0; CFTimeInterval started=previous;
        [NSTimer scheduledTimerWithTimeInterval:0.12 repeats:YES block:^(NSTimer *timer){
            CFTimeInterval now=CACurrentMediaTime(); maxGap=MAX(maxGap,now-previous); previous=now;
            switch (beats%4) { case 0:[nav popToRootViewControllerAnimated:NO]; [profile.tableView reloadData]; break; case 1:[nav pushViewController:[NGSettingsViewController new] animated:NO]; break; case 2:[nav pushViewController:[NGTypographyController new] animated:NO]; break; default:[nav popToRootViewControllerAnimated:NO]; break; }
            beats++;
            if (beats==40) { [timer invalidate]; NGSmokeImage(self.window,@"font-stress-003.png"); NGSmokeWrite(@"font-stress-003.json",@{@"passed":@(maxGap<1.5 && now-started<15),@"mainThreadHeartbeats":@(beats),@"profileSettingsFontNavigationCycles":@10,@"maxHeartbeatGapSeconds":@(maxGap),@"durationSeconds":@(now-started)}); }
        }]; return;
    }
    if ([arguments containsObject:@"--preview-dock003"]) {
        NGStart(); NSUserDefaults *d=NSUserDefaults.standardUserDefaults; [d setBool:YES forKey:NGPreferenceDock]; [d setBool:[arguments containsObject:@"--clear"] forKey:NGPreferenceDockClear];
        NSInteger collapse=[arguments containsObject:@"--collapse-left"] ? 1 : ([arguments containsObject:@"--collapse-right"] ? 2 : ([arguments containsObject:@"--collapse-center"] ? 3 : 0)); [d setInteger:collapse forKey:NGPreferenceDockCollapse];
        [d setBool:[arguments containsObject:@"--account-avatar"] forKey:NGPreferenceDockAvatar]; [d setObject:@"search" forKey:NGPreferenceDockAction];
        SSTabBarController *host=[SSTabBarController new]; self.window.rootViewController=host; self.window.overrideUserInterfaceStyle=[arguments containsObject:@"--dark"] ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight; [host loadViewIfNeeded];
        SSVideoSeriesFeedViewController *feed=[SSVideoSeriesFeedViewController new]; NSMutableArray *pages=[host.viewControllers mutableCopy]; pages[0]=feed; host.viewControllers=pages;
        if ([arguments containsObject:@"--account-avatar"]) {
            double avatarSize=[arguments containsObject:@"--avatar-scale-60"] ? 60 : ([arguments containsObject:@"--avatar-scale-100"] ? 100 : 90); [d setDouble:avatarSize forKey:NGPreferenceDockHeight]; [d setDouble:avatarSize forKey:NGPreferenceDockSearchSize];
            SSVideoSeriesFeedSideBarUserInfoCell *account=[SSVideoSeriesFeedSideBarUserInfoCell new]; account.avatarView=[SSAvatarView new]; account.avatarView.avatarView=[SSBaseReaderImageView new]; [account.contentView addSubview:account.avatarView]; [account.avatarView addSubview:account.avatarView.avatarView]; [feed.view addSubview:account];
            account.avatarView.avatarView.image=[[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(80,80)] imageWithActions:^(UIGraphicsImageRendererContext *ctx){ [UIColor.systemOrangeColor setFill]; UIRectFill(CGRectMake(0,0,80,80)); [@"N" drawAtPoint:CGPointMake(20,12) withAttributes:@{NSFontAttributeName:[UIFont boldSystemFontOfSize:48],NSForegroundColorAttributeName:UIColor.whiteColor}]; }]; [account updateUserInfo];
        }
        CAGradientLayer *gradient=[CAGradientLayer layer]; gradient.colors=@[(__bridge id)[UIColor colorWithRed:0.16 green:0.25 blue:0.34 alpha:1].CGColor,(__bridge id)[UIColor colorWithRed:0.7 green:0.45 blue:0.25 alpha:1].CGColor]; gradient.frame=self.window.bounds; [feed.view.layer insertSublayer:gradient atIndex:0];
        for (NSUInteger i=0;i<18;i++) { UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(20,40+i*47,350,35)]; label.text=[NSString stringWithFormat:@"短剧内容 %02lu    NaiwaGuo",(unsigned long)i]; label.font=[UIFont systemFontOfSize:20 weight:UIFontWeightMedium]; label.textColor=UIColor.whiteColor; [feed.view addSubview:label]; }
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.4*NSEC_PER_SEC),dispatch_get_main_queue(),^{ CGFloat h=49+self.window.safeAreaInsets.bottom; host.customBar.frame=CGRectMake(0,self.window.bounds.size.height-h,self.window.bounds.size.width,h); NGRefreshDock(host); });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.8*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); [dock layoutGlass]; if (collapse) [dock setCollapsed:YES]; });
        __block BOOL collapsedOK=YES; __block CGFloat collapsedAvatarWidth=0;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.6*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); collapsedOK=!collapse || (dock.collapsed && !dock.collapsePlatter.hidden && dock.searchKey.alpha<0.01 && fabs(dock.viewControllers[dock.selectedIndex].tabBarItem.image.size.width-(NGDockAvatarEnabled() && collapse==3 ? dock.metrics.height-14 : 26))<0.01);
            if ([arguments containsObject:@"--account-avatar"]) collapsedAvatarWidth=NGColorBounds006(NGSnapshot005(dock.collapsePlatter),UIColor.systemOrangeColor).size.width*NGDockSize(NGPreferenceDockHeight);
            NGSmokeImage(self.window,@"dock-native-003.png");
            if (collapse) [dock setCollapsed:NO];
            if ([arguments containsObject:@"--search-native"]) { host.selectedIndex=3; [dock searchTapped]; }
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2.5*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            NGSystemDock *dock=objc_getAssociatedObject(host,&NGDockAssociation); CGRect capsule=[dock.platter convertRect:dock.platter.bounds toView:dock.view];
            BOOL clear=[d boolForKey:NGPreferenceDockClear]; BOOL material=clear ? objc_getAssociatedObject(dock.platter,&NGDockAppliedGlassKey)!=nil : objc_getAssociatedObject(dock.platter,&NGDockAppliedGlassKey)==nil;
            BOOL search=![arguments containsObject:@"--search-native"] || (feed.searchInvocations==1 && host.selectedIndex==0);
            BOOL expanded=!dock.collapsed && !dock.platter.hidden && dock.searchKey.alpha>0.99;
            CGFloat expandedAvatarWidth=[arguments containsObject:@"--account-avatar"] ? NGColorBounds006(NGSnapshot005(dock.searchKey),UIColor.systemOrangeColor).size.width : 0;
            BOOL avatarEqual=![arguments containsObject:@"--account-avatar"] || (collapsedAvatarWidth>0 && fabs(collapsedAvatarWidth-expandedAvatarWidth)<1.2);
            NGSmokeImage(self.window,@"dock-expanded-003.png");
            NGSmokeWrite(@"dock-native-003.json",@{@"passed":@(collapsedOK && expanded && material && search && avatarEqual),@"avatarEqualVisibleSize":@(avatarEqual),@"expandedAvatarWidth":@(expandedAvatarWidth),@"collapsedAvatarWidth":@(collapsedAvatarWidth),@"clear":@(clear),@"collapseSide":@(collapse),@"collapsedNativePlateVisible":@(collapsedOK),@"expandedNativePlateVisible":@(expanded),@"nativeMaterialApplied":@(material),@"nativeSearchDispatchedOnce":@(search),@"capsule":NSStringFromCGRect(capsule),@"searchCircle":NSStringFromCGRect(dock.searchKey.frame),@"collapsedFrame":NSStringFromCGRect(dock.collapseFrame)});
        }); return;
    }
    if ([arguments containsObject:@"--preview-search"]) {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGStart();
        NSUInteger arg=[arguments indexOfObject:@"--search-case"];
        NSString *kind=arg!=NSNotFound && arg+1<arguments.count ? arguments[arg+1] : @"fonts";
        NSDictionary *cases=@{
            @"fonts":@[@"字体",@"typography",@"NGTypographyController"],
            @"icons":@[@"右侧动作图标",@"playbackIconPresets",@"NGPlaybackPresetSettingsController"],
            @"visibility":@[@"头像",@"playbackVisibility",@"NGPlaybackVisibilitySettingsController"],
            @"fullscreen":@[@"全屏播放",NGPreferencePlaybackFullscreen,@"NGSettingsGroupViewController"],
            @"clear":@[@"清除记录",@"clearEvents",@"NGSettingsGroupViewController"],
            @"qq":@[@"QQ",@"community",@"NGCommunityViewController"],
            @"cancel":@[@"字体",@"",@"NGSettingsViewController"],
            @"empty":@[@"no-such-setting-002",@"",@"NGSettingsViewController"]};
        NSArray *config=cases[kind] ?: cases[@"fonts"];
        NGSettingsViewController *home=[NGSettingsViewController new]; UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home]; self.window.rootViewController=nav; self.window.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
        BOOL originalFullscreen=[NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackFullscreen];
        __block NSDictionary *tapState=nil; __block BOOL activeAtTap=NO,matched=NO; __block NSUInteger selectedGroup=NSNotFound,externalOpens=0;
        NGTestCommunityOpener=^(NSURL *url,void (^completion)(BOOL)){ externalOpens++; completion(YES); };
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.4*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            UISearchController *search=home.navigationItem.searchController; search.active=YES; search.searchBar.text=config[0]; [home updateSearchResultsForSearchController:search]; [search.searchBar becomeFirstResponder];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.2*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            UISearchController *search=home.navigationItem.searchController; activeAtTap=search.active && [home isSearching];
            if ([kind isEqualToString:@"cancel"]) { matched=home.searchResults.count>0; search.active=NO; return; }
            if ([kind isEqualToString:@"empty"]) { matched=home.searchResults.count==0; return; }
            for (NSUInteger i=0;i<home.searchResults.count;i++) {
                NSDictionary *match=home.searchResults[i],*row=match[@"row"];
                if ([(row[@"key"] ?: row[@"action"]) isEqualToString:config[1]]) {
                    matched=YES; selectedGroup=[match[@"group"] unsignedIntegerValue];
                    NSIndexPath *path=[NSIndexPath indexPathForRow:i inSection:0];
                    [home tableView:home.tableView didSelectRowAtIndexPath:path];
                    tapState=@{@"pendingAfterTap":@(home.pendingSearchMatch!=nil),@"homeNavPresent":@(home.navigationController!=nil),@"homeNavTop":NSStringFromClass(home.navigationController.topViewController.class) ?: @"nil",@"realNavTop":NSStringFromClass(nav.topViewController.class),@"searchPresented":@(search.presentingViewController!=nil)};
                    [home tableView:home.tableView didSelectRowAtIndexPath:path]; // rapid repeated selection
                    break;
                }
            }
        });
        NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:9];
        [NSTimer scheduledTimerWithTimeInterval:0.1 repeats:YES block:^(NSTimer *timer) {
            BOOL stays=[kind isEqualToString:@"cancel"] || [kind isEqualToString:@"empty"];
            UIViewController *top=nav.topViewController;
            BOOL ready=matched && activeAtTap && [NSStringFromClass(top.class) isEqualToString:config[2]] && home.pendingSearchMatch==nil && !nav.transitionCoordinator && (home.navigationItem.searchController.active==[kind isEqualToString:@"empty"]);
            if (!ready && deadline.timeIntervalSinceNow>0) return;
            [timer invalidate]; BOOL located=YES;
            if ([top isKindOfClass:NGSettingsGroupViewController.class]) {
                NGSettingsGroupViewController *group=(id)top; located=group.groupIndex==selectedGroup && [group.highlightSetting isEqualToString:config[1]];
                NSArray *rows=group.group[@"rows"]; for (NSUInteger i=0;i<rows.count;i++) if ([(rows[i][@"key"] ?: rows[i][@"action"]) isEqualToString:config[1]]) {
                    UITableViewCell *cell=[group.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:0]];
                    located &= cell!=nil && cell.backgroundConfiguration.strokeWidth==1;
                }
            }
            BOOL active=home.navigationItem.searchController.active;
            BOOL emptyFooter=![kind isEqualToString:@"empty"] || [home.tableView rectForFooterInSection:0].size.height>20;
            BOOL passed=activeAtTap && matched && [NSStringFromClass(top.class) isEqualToString:config[2]] && nav.viewControllers.count==(stays?1:2) && home.pendingSearchMatch==nil && located && externalOpens==0 && emptyFooter && originalFullscreen==[NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePlaybackFullscreen] && (active==[kind isEqualToString:@"empty"]);
            NSDictionary *report=@{@"passed":@(passed),@"case":kind,@"tapState":tapState ?: @{},@"pendingAtEnd":@(home.pendingSearchMatch!=nil),@"activeSearchAtSelection":@(activeAtTap),@"matched":@(matched),@"topClass":NSStringFromClass(top.class),@"title":top.title ?: @"",@"navigationDepth":@(nav.viewControllers.count),@"searchActiveAfter":@(active),@"correctGroupAndVisibleHighlight":@(located),@"externalOpens":@(externalOpens),@"emptyFooterFits":@(emptyFooter)};
            NSString *documents=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
            NSData *json=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil]; [json writeToFile:[documents stringByAppendingPathComponent:@"search-navigation.json"] atomically:YES];
            UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithBounds:self.window.bounds]; UIImage *preview=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){ [self.window drawViewHierarchyInRect:self.window.bounds afterScreenUpdates:YES]; }];
            [UIImagePNGRepresentation(preview) writeToFile:[documents stringByAppendingPathComponent:@"search-navigation.png"] atomically:YES]; NGTestCommunityOpener=nil;
        }];
        return;
    }
    if ([arguments containsObject:@"--preview-motion"]) {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGStart();
        UIViewController *stage = [UIViewController new]; stage.view.backgroundColor = NGSettingsCanvas(); self.window.rootViewController = stage; self.window.overrideUserInterfaceStyle = UIUserInterfaceStyleLight;
        NGSettingsViewController *home = [NGSettingsViewController new]; UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:home]; nav.modalPresentationStyle = UIModalPresentationFullScreen;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.6*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [stage presentViewController:nav animated:YES completion:nil]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [home tableView:home.tableView didHighlightRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:5]]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2.2*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NSIndexPath *path=[NSIndexPath indexPathForRow:0 inSection:5]; [home tableView:home.tableView didUnhighlightRowAtIndexPath:path]; [home tableView:home.tableView didSelectRowAtIndexPath:path]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,3.5*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NGSettingsGroupViewController *group=(id)nav.topViewController; [group tableView:group.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [nav popViewControllerAnimated:YES]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,6*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [nav popToRootViewControllerAnimated:YES]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,7.2*NSEC_PER_SEC),dispatch_get_main_queue(),^{ self.window.overrideUserInterfaceStyle = UIUserInterfaceStyleDark; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,8.2*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [home tableView:home.tableView didHighlightRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:2]]; });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,8.4*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [home tableView:home.tableView didUnhighlightRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:2]]; });
        return;
    }
    if ([arguments containsObject:@"--preview-fonts"] || [arguments containsObject:@"--preview-share"] || [arguments containsObject:@"--preview-link-failure"] || [arguments containsObject:@"--preview-support"] || [arguments containsObject:@"--preview-community"] || [arguments containsObject:@"--preview-settings"] || [arguments containsObject:@"--preview-icons"] || [arguments containsObject:@"--preview-colors"] || [arguments containsObject:@"--preview-dock-icons"] || [arguments containsObject:@"--preview-layout"]) {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGStart();
        UIViewController *settings = [arguments containsObject:@"--preview-icons"] ? [NGPlaybackPresetSettingsController new] : ([arguments containsObject:@"--preview-colors"] ? [NGDockColorSettingsController new] : ([arguments containsObject:@"--preview-dock-icons"] ? [NGDockIconSettingsController new] : ([arguments containsObject:@"--preview-layout"] ? [NGPlaybackLayoutSettingsController new] : [NGSettingsViewController new])));
        if ([arguments containsObject:@"--preview-fonts"]) settings = [NGTypographyController new];
        if ([arguments containsObject:@"--dock-settings"]) settings=[NGDockAppearanceSettingsController new];
        if ([arguments containsObject:@"--font-demo"]) { [NSUserDefaults.standardUserDefaults setDouble:0.8 forKey:NGPreferenceFontScale]; [NSUserDefaults.standardUserDefaults setObject:@"Georgia" forKey:NGPreferenceFontName]; NGRefreshTypography(); }

        if ([arguments containsObject:@"--preview-support"] || [arguments containsObject:@"--preview-share"]) settings = [NGSupportViewController new];
        if ([arguments containsObject:@"--preview-community"] || [arguments containsObject:@"--preview-link-failure"]) { settings = [NGCommunityViewController new]; }
        [NSUserDefaults.standardUserDefaults setObject:@"badge" forKey:NGPreferencePlaybackPreset];
        self.window.rootViewController = [[UINavigationController alloc] initWithRootViewController:settings];
        self.window.overrideUserInterfaceStyle = [arguments containsObject:@"--dark"] ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
        if ([arguments containsObject:@"--preview-share"]) dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.6*NSEC_PER_SEC),dispatch_get_main_queue(),^{ [(NGSupportViewController *)settings shareCode]; });
        if ([arguments containsObject:@"--preview-link-failure"]) { NGTestCommunityOpener = ^(NSURL *url,void (^completion)(BOOL)){ completion(NO); }; dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.6*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NGOpenCommunity(@"qqGroup",settings); }); }
        UIView *previewRenderView = self.window;
        if ([arguments containsObject:@"--compact"]) {
            UIViewController *navigation = self.window.rootViewController;
            UIViewController *viewport = [UIViewController new]; self.window.rootViewController = viewport; [viewport loadViewIfNeeded];
            [viewport addChildViewController:navigation]; [viewport.view addSubview:navigation.view];
            navigation.view.frame = CGRectMake(0,0,320,568); [navigation didMoveToParentViewController:viewport];
            previewRenderView = navigation.view;
        }
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            [previewRenderView setNeedsLayout]; [previewRenderView layoutIfNeeded];
            if ([arguments containsObject:@"--scroll-bottom"] && [settings isKindOfClass:UITableViewController.class]) {
                UITableView *table=((UITableViewController *)settings).tableView;
                [table setContentOffset:CGPointMake(0,MAX(-table.adjustedContentInset.top,table.contentSize.height-table.bounds.size.height+table.adjustedContentInset.bottom)) animated:NO];
                [table layoutIfNeeded];
            }
            UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithBounds:previewRenderView.bounds];
            UIImage *preview = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) { [previewRenderView drawViewHierarchyInRect:previewRenderView.bounds afterScreenUpdates:YES]; }];
            NSString *documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
            [UIImagePNGRepresentation(preview) writeToFile:[documents stringByAppendingPathComponent:@"preview.png"] atomically:YES];
            if ([arguments containsObject:@"--preview-icons"]) {
                UIGraphicsImageRenderer *sheetRenderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(390,336)];
                UIImage *sheet=[sheetRenderer imageWithActions:^(UIGraphicsImageRendererContext *context){
                    [[UIColor colorWithWhite:0.08 alpha:1] setFill]; UIRectFill(CGRectMake(0,0,390,336));
                    NSDictionary *small=@{NSFontAttributeName:[UIFont systemFontOfSize:11],NSForegroundColorAttributeName:UIColor.lightGrayColor};
                    for (NSUInteger j=0;j<4;j++) [@[@"收藏",@"评论",@"点赞",@"分享"][j] drawAtPoint:CGPointMake(138+j*60,16) withAttributes:small];
                    for (NSUInteger i=1;i<NGPlaybackPresets().count;i++) {
                        [NGPlaybackPresetTitles()[i] drawAtPoint:CGPointMake(18,53+(i-1)*72) withAttributes:@{NSFontAttributeName:[UIFont systemFontOfSize:13],NSForegroundColorAttributeName:UIColor.whiteColor}];
                        for (NSUInteger j=0;j<4;j++) { UIImage *icon=NGPlaybackVectorIcon(j,NO,NGPlaybackPresets()[i]); if (icon.renderingMode!=UIImageRenderingModeAlwaysOriginal) icon=[icon imageWithTintColor:UIColor.whiteColor renderingMode:UIImageRenderingModeAlwaysOriginal]; [icon drawInRect:CGRectMake(126+j*60,38+(i-1)*72,46,46)]; }
                    }
                }];
                [UIImagePNGRepresentation(sheet) writeToFile:[documents stringByAppendingPathComponent:@"presets-overview.png"] atomically:YES];
            }

        });
        return;
    }

    if ([NSProcessInfo.processInfo.arguments containsObject:@"--preview-picker"]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ NGPresentIconPhotoPicker(self.window.rootViewController, NGPlaybackCustomImageKey(0, NO)); });
        return;
    }

    if ([NSProcessInfo.processInfo.arguments containsObject:@"--preview-dock"]) {
        SSTabBarController *host = [SSTabBarController new]; self.window.rootViewController = host; [host loadViewIfNeeded];
        NSMutableArray *pages = [host.viewControllers mutableCopy]; pages[0] = [NGTestRootController new]; host.viewControllers = pages;
        [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceDock];
        [NSUserDefaults.standardUserDefaults setObject:@"soft" forKey:NGPreferencePlaybackPreset];
        NGStart(); NGRefreshDock(host);
        return;
    }

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.25 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        // Feature tests opt in explicitly; startup defaults are tested separately.
        for (NSString *key in @[NGPreferenceHUD,NGPreferenceDock,NGPreferencePlaybackGlass,NGPreferencePlaybackHeaderGlass,NGPreferenceCollectionGlass,NGPreferenceShareGlass,NGPreferenceShareGlassClear]) [NSUserDefaults.standardUserDefaults setBool:YES forKey:key];
        NGStart();
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.25 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NGSetPerformanceEnabled(YES);
        NGUpdatePlaybackGlass();
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NSData *playback = NGCaptureScreenshot();
        NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        [playback writeToFile:[documents stringByAppendingPathComponent:@"playback.png"] atomically:YES];
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.35 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NSURL *archive = NGCreateCaptureArchive();
        NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSURL *destination = [NSURL fileURLWithPath:[documents stringByAppendingPathComponent:@"NaiwaGuo-capture-test.zip"]];
        if (archive) [[NSFileManager defaultManager] copyItemAtURL:archive toURL:destination error:nil];
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.65 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [NGPerformanceRecorder.shared stopWithReason:@"test"];
        NGOpenSettings();
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.05 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [(NGSettingsViewController *)NGTopController() close];
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        NGOpenSettings();
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIWindow *overlay = NGCurrentOverlay();
        UIViewController *top = NGTopController();
        BOOL settingsVisible = [top isKindOfClass:NGSettingsViewController.class] ||
            ([top isKindOfClass:UINavigationController.class] && [((UINavigationController *)top).topViewController isKindOfClass:NGSettingsViewController.class]);
        NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSURL *destination = [NSURL fileURLWithPath:[documents stringByAppendingPathComponent:@"NaiwaGuo-capture-test.zip"]];
        BOOL copied = [[NSFileManager defaultManager] fileExistsAtPath:destination.path];
        NSDictionary *snapshot = NGCaptureSnapshot();
        NSUInteger glassCount = 0;
        NGTestRootController *root = (NGTestRootController *)self.window.rootViewController;
        for (UIView *view in root.rightRail.subviews) if ([view isKindOfClass:NGPlaybackGlassView.class]) glassCount++;
        UIView *originalAction = nil;
        for (UIView *view in root.rightRail.subviews) if ([view isKindOfClass:SSShortVideoRightInteractionCollectView.class]) originalAction = view;
        CGRect originalFrame = originalAction.frame;
        BOOL labelsOutside = YES;
        NSDictionary *diskInfo = [NSDictionary dictionaryWithContentsOfFile:[NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"Info.plist"]];
        NSDictionary *runtimeStatus = NGRuntimeGlassStatus();
        NSBundle *frameworkBundle = [NSBundle bundleForClass:UIView.class];
        for (UIView *action in root.rightRail.subviews) {
            if ([action isKindOfClass:NGPlaybackGlassView.class]) continue;
            if ([NGPlaybackRailItemIdentifier(action) isEqualToString:@"avatar"]) continue;
            NGPlaybackGlassView *glass = objc_getAssociatedObject(action, &NGPlaybackGlassAssociationKey);
            CGRect labelFrame = [[action viewWithTag:101] convertRect:[action viewWithTag:101].bounds toView:root.rightRail];
            if (!glass || CGRectGetMaxY(glass.frame) > CGRectGetMinY(labelFrame)) labelsOutside = NO;
        }
        BOOL hitPreserved = [root.rightRail hitTest:CGPointMake(CGRectGetMidX(originalAction.frame), CGRectGetMidY(originalAction.frame)) withEvent:nil] == originalAction;
        NGSetPlaybackGlassEnabled(NO);
        BOOL removed = YES;
        for (UIView *view in root.rightRail.subviews) if ([view isKindOfClass:NGPlaybackGlassView.class]) removed = NO;
        NGSetPlaybackGlassEnabled(YES);
        NGDecoratePlaybackInteraction(root.rightRail);
        NSUInteger recreatedCount = 0;
        for (UIView *view in root.rightRail.subviews) if ([view isKindOfClass:NGPlaybackGlassView.class]) recreatedCount++;
        NGWriteTestResult(@{
            @"fullscreenPlayback": NGTestFullscreenPlayback(),
            @"collection075": NGTestCollection075(),
            @"review075": NGTestReview075(),
            @"underlay075": NGTestUnderlay075(),
            @"settings075": NGTestSettings075(),
            @"backdrop075": NGTestBackdrop075(),
            @"dramaTitle": NGTestDramaTitleLayout(),
            @"shareIconReuse": NGTestShareIconReuse(),
            @"thumbnailRepaint": NGTestThumbnailRepaint(),
            @"dockAspectAndPreview": NGTestDockAspectAndPreview(),
            @"openingAnimation": NGTestOpeningAnimation(),
            @"settings001": NGTestSettings001(),
            @"corners002": NGTestPlaybackCorners002(),
            @"shareRepaint002": NGTestShareRepaint002(),
            @"typography002": NGTestTypography002(),
            @"search002": NGTestSearch002(),
            @"typography003": NGTestTypography003(),
            @"corners003": NGTestCorners003(),
            @"iconStates003": NGTestIconStates003(),
            @"dock003": NGTestDock003(),
            @"dockColors005": NGTestDockColors005(),
            @"dockActions005": NGTestDockActions005(),
            @"fullscreenTransitions005": NGTestFullscreenTransitions005(),
            @"speedGlass005": NGTestSpeedGlass005(),
            @"dockSurface006": NGTestDockSurface006(),
            @"dockAvatarAndInk006": NGTestDockAvatarAndInk006(),
            @"legacyDock006": NGTestLegacyDock006(),
            @"danmaku007": NGTestDanmaku007(),
            @"settings009": NGTestSettings009(),
            @"sponsorPrompt007": NGTestSponsorPrompt007(),
            @"dockHaptics005": NGTestDockHaptics005(),
            @"episodeToolbar": NGTestEpisodeToolbar(),
            @"dockColors": NGTestDockColors(),
            @"statusBarRouting": NGTestStatusBarRouting(),
            @"morePanelGlass": NGTestMorePanelGlass(),
            @"dock": NGTestDock(),
            @"playbackPresets": NGTestPlaybackPresets(root.rightRail),
            @"commentGlass": NGTestCommentGlass(),
            @"shareGlass": NGTestShareGlass(),
            @"headerRestore": NGTestHeaderRestore(),
            @"fullscreenPreview": NGTestFullscreenAndPreview(),
            @"playbackRail": NGTestPlaybackRailLayout(),
            @"legacyXStyleRemoved": NGTestPlaybackXStyle(),
            @"photoIcons": NGTestPhotoIcons(),
            @"commentNativeGeometry": NGTestNativeCommentGeometry(),
            @"playbackText": NGTestPlaybackTextLayout(),
            @"playbackHeaderGlass": NGTestPlaybackHeaderGlass(),
            @"nativeSettings": NGTestNativeSettings(),
            @"overlayCreated": @(overlay != nil),
            @"overlayVisible": @(overlay && !overlay.hidden),
            @"settingsVisible": @(settingsVisible),
            @"archiveCopied": @(copied),
            @"viewCount": snapshot[@"viewsCount"] ?: @0,
            @"windowCount": @([snapshot[@"windows"] count]),
            @"eventCount": @([NGEventLog.shared events].count),
            @"performanceSamples": @(NGPerformanceRecorder.shared.seconds.count),
            @"performanceState": NGPerformanceRecorder.shared.state ?: @"idle",
            @"settingsGroups": @(NGSettingsGroups().count),
            @"playbackGlassCount": @(glassCount),
            @"labelsOutsideGlass": @(labelsOutside),
            @"compatibilityDiskUnchanged": @([diskInfo[@"UIDesignRequiresCompatibility"] isEqual:NGOriginalHostInfo[@"UIDesignRequiresCompatibility"]]),
            @"runtimeCompatibilityFalse": @([runtimeStatus[@"bundleHooksInstalled"] boolValue] && ![NSBundle.mainBundle.infoDictionary[@"UIDesignRequiresCompatibility"] boolValue] && ![[NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] boolValue]),
            @"otherBundleUnchanged": @([frameworkBundle.infoDictionary isEqual:NGTestFrameworkOriginalInfo] || (!frameworkBundle.infoDictionary && !NGTestFrameworkOriginalInfo)),
            @"otherHostKeysUnchanged": @([diskInfo[@"CFBundleIdentifier"] isEqual:NSBundle.mainBundle.infoDictionary[@"CFBundleIdentifier"]]),
            @"toggleRestores": @(removed && recreatedCount == 5),
            @"originalHitPreserved": @(hitPreserved),
            @"originalFramePreserved": @(CGRectEqualToRect(originalFrame, originalAction.frame))
        });
    });
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        BOOL retained=[NSProcessInfo.processInfo.arguments containsObject:@"--retained-defaults005"] || [NSProcessInfo.processInfo.arguments containsObject:@"--sponsor-never-check009"] || [NSProcessInfo.processInfo.arguments containsObject:@"--unrestricted-restart009"];
        if (!retained) [NSUserDefaults.standardUserDefaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];
        NGTestFrameworkOriginalInfo = [[NSBundle bundleForClass:UIView.class].infoDictionary copy];
        [NSUserDefaults.standardUserDefaults setObject:@"original" forKey:NGPreferencePlaybackPreset];
        NGPrepareRuntimeGlass();
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(NGTestAppDelegate.class));
    }
}
