@interface SSShortVideoOutterSeriesLeftContainerView : UIView @end
@implementation SSShortVideoOutterSeriesLeftContainerView @end
@interface YYLabel : UILabel @end
@implementation YYLabel @end
@interface FQVShortVideoSettingsThumbnailCell : UITableViewCell @end
@implementation FQVShortVideoSettingsThumbnailCell @end

static NSDictionary *NGTestDramaTitleLayout(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    SSShortVideoOutterSeriesLeftContainerView *container = [[SSShortVideoOutterSeriesLeftContainerView alloc] initWithFrame:CGRectMake(16, 620, 274, 80)];
    UIView *row = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 274, 50)];
    UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 224, 22)]; title.text = @"听建议的校长，垫底高中变神校";
    UIImageView *arrow = [[UIImageView alloc] initWithFrame:CGRectMake(228, 1, 10, 20)]; [row addSubview:title]; [row addSubview:arrow]; [container addSubview:row];
    UIScrollView *scroll = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 58, 274, 22)];
    YYLabel *caption = [[YYLabel alloc] initWithFrame:scroll.bounds]; caption.text = @"这条简介应当与剧名独立调节"; [scroll addSubview:caption]; [container addSubview:scroll];
    NSString *prefix = @"com.naiwaguo.playback.layout.title";
    [defaults setFloat:26 forKey:[prefix stringByAppendingString:@".x"]]; [defaults setFloat:-14 forKey:[prefix stringByAppendingString:@".y"]]; [defaults setFloat:1.25 forKey:[prefix stringByAppendingString:@".scale"]];
    [defaults setFloat:-9 forKey:@"com.naiwaguo.playback.layout.description.x"];
    NGInstallPlaybackTextHooks(); [container setNeedsLayout]; [container layoutIfNeeded];
    BOOL independent = fabs(row.transform.tx - 26) < 0.01 && fabs(row.transform.ty + 14) < 0.01 && fabs(row.transform.a - 1.25) < 0.01 && fabs(caption.transform.tx + 9) < 0.01 && CGAffineTransformIsIdentity(title.transform) && arrow.superview == row;
    for (NSUInteger i = 0; i < 50; i++) NGDecoratePlaybackTextView(container);
    BOOL stable = fabs(row.transform.tx - 26) < 0.01 && fabs(row.transform.a - 1.25) < 0.01;
    title.text = @"下一部短剧"; NGDecoratePlaybackTextView(container);
    BOOL reused = fabs(row.transform.tx - 26) < 0.01;
    NGPlaybackLayoutSettingsController *settings = [NGPlaybackLayoutSettingsController new]; [settings loadViewIfNeeded];
    BOOL controls = settings.controls.count == 5 && [settings.controls[2][0][@"key"] isEqual:[prefix stringByAppendingString:@".x"]];
    [settings resetLayout]; NGDecoratePlaybackTextView(container);
    return @{@"independentTitleAndDescription": @(independent), @"noAccumulation": @(stable), @"reusedTitleKeepsLayout": @(reused), @"fiveControlGroups": @(controls), @"resetRestoresBoth": @(CGAffineTransformIsIdentity(row.transform) && CGAffineTransformIsIdentity(caption.transform))};
}

static NSDictionary *NGTestShareIconReuse(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    [defaults setObject:@"original" forKey:NGPreferencePlaybackPreset]; NGInstallPlaybackPresetHooks();
    SSShortVideoRightInteractionShareView *action = [[SSShortVideoRightInteractionShareView alloc] initWithFrame:CGRectMake(0, 0, 46, 62)];
    UIImageView *icon = (UIImageView *)[action viewWithTag:100]; UIImage *firstHost = icon.image;
    [defaults setObject:@"soft" forKey:NGPreferencePlaybackPreset]; NGUpdatePlaybackGlass();
    UIImage *soft = icon.image; BOOL immediate = soft && soft != firstHost;
    UIImage *nextHost = [UIImage systemImageNamed:@"square.and.arrow.up"];
    for (NSUInteger i = 0; i < 100; i++) { icon.image = nextHost; icon.highlightedImage = nextHost; }
    BOOL noTap = [UIImagePNGRepresentation(icon.image) isEqual:UIImagePNGRepresentation(soft)] && [UIImagePNGRepresentation(icon.highlightedImage) isEqual:UIImagePNGRepresentation(soft)];
    [defaults setObject:@"bold" forKey:NGPreferencePlaybackPreset]; NGUpdatePlaybackGlass();
    BOOL switchAgain = ![UIImagePNGRepresentation(icon.image) isEqual:UIImagePNGRepresentation(soft)];
    SSShortVideoRightInteractionShareView *nextVideo = [[SSShortVideoRightInteractionShareView alloc] initWithFrame:action.frame];
    UIImageView *nextIcon = (UIImageView *)[nextVideo viewWithTag:100]; nextIcon.image = nextHost;
    BOOL newVideo = [UIImagePNGRepresentation(nextIcon.image) isEqual:UIImagePNGRepresentation(icon.image)];
    UIImageView *replacement = [[UIImageView alloc] initWithImage:nextHost]; replacement.frame = icon.frame;
    [icon removeFromSuperview]; [action insertSubview:replacement atIndex:0];
    BOOL replacedView = replacement.image != nextHost && objc_getAssociatedObject(replacement, &NGPlaybackPresetIconAssociation) != nil;
    [defaults setObject:@"original" forKey:NGPreferencePlaybackPreset]; NGUpdatePlaybackGlass();
    BOOL restored = replacement.image == nextHost && !objc_getAssociatedObject(replacement, &NGPlaybackPresetIconAssociation);
    replacement.image = firstHost;
    return @{@"immediateWithoutTapOrLayout": @(immediate), @"hostImageWritesCannotUndoPreset": @(noTap), @"switchPresetAgain": @(switchAgain), @"newVideoInheritsPreset": @(newVideo), @"replacementImageView": @(replacedView), @"originalRestoresLatestHostImage": @(restored && replacement.image == firstHost && nextIcon.image == nextHost)};
}

static NSDictionary *NGTestThumbnailRepaint(void) {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults; [defaults setBool:YES forKey:NGPreferenceShareGlass];
    FQVShortVideoSettingsViewController *controller = [FQVShortVideoSettingsViewController new]; controller.view.frame = CGRectMake(0, 0, 402, 612);
    NGInstallCommentGlassHooks(); NGDecorateShareController(controller);
    FQVShortVideoSettingsThumbnailCell *cell = [[FQVShortVideoSettingsThumbnailCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.frame = CGRectMake(20, 520, 362, 54); cell.backgroundColor = UIColor.whiteColor; cell.contentView.backgroundColor = UIColor.whiteColor;
    UIImageView *thumbnail = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"photo"]]; thumbnail.frame = CGRectMake(320, 15, 24, 24); [cell.contentView addSubview:thumbnail];
    [controller.view addSubview:cell]; [cell setNeedsLayout]; [cell layoutIfNeeded];
    BOOL lateCell = [cell.backgroundColor isEqual:UIColor.clearColor] && [cell.contentView.backgroundColor isEqual:UIColor.clearColor];
    UIColor *latest = [UIColor colorWithWhite:0.96 alpha:1];
    for (NSUInteger i = 0; i < 100; i++) { cell.backgroundColor = latest; cell.contentView.backgroundColor = latest; }
    BOOL repaint = [cell.backgroundColor isEqual:UIColor.clearColor] && [cell.contentView.backgroundColor isEqual:UIColor.clearColor];
    BOOL content = thumbnail.image != nil && !thumbnail.hidden && thumbnail.superview == cell.contentView;
    [defaults setBool:NO forKey:NGPreferenceShareGlass]; NGDecorateShareController(controller);
    BOOL restored = [cell.backgroundColor isEqual:latest] && [cell.contentView.backgroundColor isEqual:latest];
    UIView *unrelated = [UIView new]; unrelated.backgroundColor = UIColor.whiteColor; [defaults setBool:YES forKey:NGPreferenceShareGlass];
    return @{@"lateThumbnailCellGetsGlass": @(lateCell), @"whiteRepaintSuppressedWithoutScroll": @(repaint), @"thumbnailPreserved": @(content), @"restoresLatestHostColor": @(restored), @"unrelatedViewsUnchanged": @([unrelated.backgroundColor isEqual:UIColor.whiteColor])};
}

static NSDictionary *NGTestDockAspectAndPreview(void) {
    UIImage *source = [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(120, 30)] imageWithActions:^(UIGraphicsImageRendererContext *context) { [UIColor.redColor setFill]; UIRectFill(CGRectMake(0, 0, 120, 30)); }];
    UIImage *image = NGDockIconCaptionImage([source imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal], @"首页", UIColor.blackColor);
    size_t width = CGImageGetWidth(image.CGImage), height = CGImageGetHeight(image.CGImage); unsigned char *pixels = calloc(width * height, 4);
    CGColorSpaceRef colors = CGColorSpaceCreateDeviceRGB(); CGContextRef context = CGBitmapContextCreate(pixels, width, height, 8, width * 4, colors, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image.CGImage);
    size_t left = width, top = height, right = 0, bottom = 0;
    for (size_t y = 0; y < height; y++) for (size_t x = 0; x < width; x++) { unsigned char *p = pixels + (y * width + x) * 4; if (p[0] > 200 && p[1] < 30 && p[2] < 30 && p[3] > 200) { left = MIN(left, x); right = MAX(right, x); top = MIN(top, y); bottom = MAX(bottom, y); } }
    CGFloat ratio = (CGFloat)(right - left + 1) / MAX(1, bottom - top + 1); free(pixels); CGContextRelease(context); CGColorSpaceRelease(colors);
    NGDockIconSettingsController *settings = [NGDockIconSettingsController new]; [settings loadViewIfNeeded]; BOOL match = settings.preview.viewControllers.count == 4;
    for (NSUInteger i = 0; i < 4; i++) {
        UITabBarItem *actual = [[UITabBarItem alloc] initWithTitle:NGDockItemTitles()[i] image:nil tag:i]; NGDockApplyRenderedImages(actual, i, settings.preview.traitCollection.userInterfaceStyle);
        UITabBarItem *preview = settings.preview.viewControllers[i].tabBarItem;
        match &= [UIImagePNGRepresentation(actual.image) isEqual:UIImagePNGRepresentation(preview.image)] && [UIImagePNGRepresentation(actual.selectedImage) isEqual:UIImagePNGRepresentation(preview.selectedImage)];
    }
    UITableViewCell *row = [settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:1]];
    return @{@"wideImagePreservesAspect": @(fabs(ratio - 4) < 0.25), @"previewMatchesBothActualStates": @(match), @"previewUsesNativeTabBar": @([settings.preview.tabBar isKindOfClass:UITabBar.class]), @"symbolRowRemainsTintable": @(row.imageView.image.renderingMode == UIImageRenderingModeAlwaysTemplate)};
}

@interface NGTestEntranceTransition : NSObject
@property(nonatomic) BOOL initiallyInteractive;
@property(nonatomic, copy) void (^animation)(id);
@property(nonatomic, copy) void (^completion)(id);
@end
@implementation NGTestEntranceTransition
- (BOOL)animateAlongsideTransition:(void (^)(id))animation completion:(void (^)(id))completion { self.animation = animation; self.completion = completion; return YES; }
@end
@interface NGTestEntranceController : NGSettingsTableController
@property(nonatomic, strong) NGTestEntranceTransition *testTransition;
@end
@implementation NGTestEntranceController
- (id<UIViewControllerTransitionCoordinator>)transitionCoordinator { return (id)self.testTransition; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 4; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path { UITableViewCell *cell = [UITableViewCell new]; cell.textLabel.text = @"设置项目"; return cell; }
@end
static NSDictionary *NGTestOpeningAnimation(void) {
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:NGBaseWindow().windowScene]; NGTestEntranceController *controller = [NGTestEntranceController new]; window.rootViewController = controller; window.hidden = NO; [controller.view layoutIfNeeded];
    controller.testTransition = [NGTestEntranceTransition new]; [controller viewIsAppearing:YES]; UITableViewCell *cell = controller.tableView.visibleCells.firstObject;
    BOOL starts = controller.testTransition.animation && cell && cell.contentView.alpha == 0;
    if (controller.testTransition.animation) controller.testTransition.animation(nil); if (controller.testTransition.completion) controller.testTransition.completion(nil);
    BOOL restored = cell.contentView.alpha == 1 && CGAffineTransformIsIdentity(cell.contentView.transform);
    controller.testTransition.animation = nil; [controller viewDidAppear:YES]; BOOL noLateStart = !controller.testTransition.animation && cell.contentView.alpha == 1;
    controller.testTransition.initiallyInteractive = YES; [controller viewIsAppearing:YES]; BOOL interactive = !controller.testTransition.animation && cell.contentView.alpha == 1; window.hidden = YES;
    return @{@"startsInsideOpeningTransition": @(starts), @"completionRestoresContent": @(restored), @"noAnimationStartsAfterOpened": @(noLateStart), @"interactiveNavigationPreserved": @(interactive)};
}
