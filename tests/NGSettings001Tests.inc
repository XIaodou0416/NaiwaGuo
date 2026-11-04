// Capture presentation arguments synchronously. UIKit's remote share service is
// exercised separately by --preview-share after the main test callback returns.
@interface NGTestSupportPresenter : NGSupportViewController
@property(nonatomic,strong) UIViewController *capturedPresentation;
@end
@implementation NGTestSupportPresenter
- (void)presentViewController:(UIViewController *)controller animated:(BOOL)animated completion:(void (^)(void))completion { self.capturedPresentation=controller; if (completion) completion(); }
@end
@interface NGTestFailurePresenter : UIViewController
@property(nonatomic,strong) UIViewController *capturedPresentation;
@end
@implementation NGTestFailurePresenter
- (void)presentViewController:(UIViewController *)controller animated:(BOOL)animated completion:(void (^)(void))completion { self.capturedPresentation=controller; if (completion) completion(); }
@end
static NSDictionary *NGTestSettings001(void) {
    NSMutableDictionary *checks = [NSMutableDictionary dictionary];
    NSArray *groups = NGSettingsGroups(); NSArray *rows = groups.lastObject[@"rows"];
    checks[@"versionStartsAt001"] = @([NGVersion isEqualToString:@"0.0.4"]);
    checks[@"fiveGroupsWithoutAuthorCategory"] = @(groups.count == 5 && rows.count == 1 && ![[groups valueForKey:@"title"] containsObject:@"作者与交流"]);
    NSArray *actions = @[@"qqGroup",@"tgNews",@"tgChat"];
    NSArray *links = @[@"https://qm.qq.com/q/w9rb2DnAoE",@"https://t.me/beans0416",@"https://t.me/beans1011"];
    NSMutableArray *opened = [NSMutableArray array];
    NGTestCommunityOpener = ^(NSURL *url, void (^completion)(BOOL)) { [opened addObject:url.absoluteString]; completion(YES); };
    NGSettingsViewController *community = [NGSettingsViewController new];
    UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:community];
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:NGBaseWindow().windowScene]; window.rootViewController = nav; window.hidden = NO;
    [community loadViewIfNeeded]; [nav.view layoutIfNeeded];
    UIStackView *footerStack = (id)community.supportFooter.subviews.firstObject;
    NSArray<UIButton *> *footerButtons = (id)footerStack.arrangedSubviews;
    checks[@"twoVerticalGlassFooterButtons"] = @(footerButtons.count == 2 && footerStack.axis == UILayoutConstraintAxisVertical && [footerButtons[0].configuration.title isEqual:@"赞赏"] && [footerButtons[1].configuration.title isEqual:@"交流群"] && community.tableView.tableFooterView == community.supportFooter);
    [footerButtons[1] sendActionsForControlEvents:UIControlEventTouchUpInside];
    [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.45]];
    NGCommunityViewController *invitations = (id)nav.topViewController; [invitations loadViewIfNeeded]; [nav.view layoutIfNeeded];
    checks[@"communityEntryOpensInvitationPage"] = @([invitations isKindOfClass:NGCommunityViewController.class] && invitations.cards.arrangedSubviews.count == 3 && opened.count == 0);
    BOOL exact = YES, glassCards = YES;
    for (NSUInteger i=0;i<3;i++) {
        exact &= [NGCommunityURL(actions[i]).absoluteString isEqualToString:links[i]];
        NGCommunityCard *card = (id)invitations.cards.arrangedSubviews[i];
        glassCards &= card.glass.layer.cornerRadius == 24 && !card.glass.userInteractionEnabled && card.isAccessibilityElement;
        if (@available(iOS 26.0, *)) glassCards &= [card.glass.effect isKindOfClass:UIGlassEffect.class] && ((UIGlassEffect *)card.glass.effect).interactive;
        card.highlighted = YES; glassCards &= fabs(card.alpha-0.65)<0.01; card.highlighted = NO; glassCards &= card.alpha == 1;
        [card sendActionsForControlEvents:UIControlEventTouchUpInside];
    }
    checks[@"allThreeOriginalURLsExact"] = @(exact);
    checks[@"eachInvitationCardOpensItsURL"] = @([opened isEqualToArray:links]);
    checks[@"invitationGlassAndPressFeedback"] = @(glassCards);
    [nav popViewControllerAnimated:NO];
    [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.02]];
    NGOpenCommunity(@"unknown",community); NGOpenCommunity(nil,community);
    checks[@"unknownActionDoesNotOpen"] = @(opened.count == 3 && NGCommunityURL(nil) == nil);
    NGTestCommunityOpener = nil;
    [footerButtons[0] sendActionsForControlEvents:UIControlEventTouchUpInside];
    checks[@"sponsorFooterOpensSupportPage"] = @([nav.topViewController isKindOfClass:NGSupportViewController.class]);
    [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.45]];
    NGSupportViewController *support = (id)nav.topViewController; [support loadViewIfNeeded];
    UIImage *image = support.codeView.image;
    checks[@"embeddedCodeDecodesAtOriginalSize"] = @(image && CGImageGetWidth(image.CGImage)==534 && CGImageGetHeight(image.CGImage)==505 && NGSponsorCodeData().length > 1000);
    checks[@"codePreservesAspectAndWhiteQuietZone"] = @(support.codeView.contentMode == UIViewContentModeScaleAspectFit && [support.codeView.backgroundColor isEqual:UIColor.whiteColor]);
    BOOL sponsorGlass = support.glassCard.layer.cornerRadius == 28 && support.stack.spacing == 16;
    if (@available(iOS 26.0, *)) sponsorGlass &= [support.glassCard.effect isKindOfClass:UIGlassEffect.class] && !((UIGlassEffect *)support.glassCard.effect).interactive;
    checks[@"sponsorUsesRegularGlassCard"] = @(sponsorGlass);
    UIViewController *viewport = [UIViewController new]; window.rootViewController = viewport; [viewport loadViewIfNeeded];
    BOOL layoutOK = YES, communityLayoutOK = YES, footerLayoutOK = YES; NSMutableArray *supportLayouts = [NSMutableArray new];
    // A fresh contained navigation controller models each viewport. Reparenting
    // an already visible stack leaves UIKit's old transition container attached.
    for (NSValue *sizeValue in @[[NSValue valueWithCGSize:CGSizeMake(320,568)],[NSValue valueWithCGSize:CGSizeMake(844,390)]]) {
        CGSize size = sizeValue.CGSizeValue;
        for (NSUInteger kind=0;kind<3;kind++) {
            UIViewController *page = kind == 2 ? [NGSettingsViewController new] : kind ? [NGCommunityViewController new] : [NGSupportViewController new];
            UINavigationController *sizedNav = [[UINavigationController alloc] initWithRootViewController:page];
            [viewport addChildViewController:sizedNav]; sizedNav.view.frame = (CGRect){CGPointZero,size};
            [viewport.view addSubview:sizedNav.view]; [sizedNav didMoveToParentViewController:viewport];
            [sizedNav.view setNeedsLayout]; [sizedNav.view layoutIfNeeded]; [page.view layoutIfNeeded];
            if (!kind) {
                NGSupportViewController *sized = (id)page; [sized.scrollView layoutIfNeeded]; [sized.stack layoutIfNeeded]; CGRect code = sized.codeView.bounds;
                [supportLayouts addObject:@{@"requested":NSStringFromCGSize(size), @"page":NSStringFromCGRect(page.view.bounds), @"card":NSStringFromCGRect(sized.glassCard.bounds), @"code":NSStringFromCGRect(code)}];
                layoutOK &= fabs(page.view.bounds.size.width-size.width)<0.5 && sized.glassCard.bounds.size.width<=430.5 && sized.shareButton.bounds.size.height>=47.9 && code.size.width>0 && fabs(code.size.width/code.size.height-534.0/505.0)<0.01 && !sized.stack.hasAmbiguousLayout && !sized.codeView.hasAmbiguousLayout;
            } else if (kind == 1) {
                NGCommunityViewController *sized = (id)page; [sized.scrollView layoutIfNeeded]; [sized.cards layoutIfNeeded];
                communityLayoutOK &= !sized.cards.hasAmbiguousLayout && fabs(sized.cards.bounds.size.width-(size.width-32))<0.5;
                for (NGCommunityCard *card in sized.cards.arrangedSubviews) communityLayoutOK &= card.bounds.size.height>=120 && !card.hasAmbiguousLayout && card.bounds.size.width<=size.width;
            }
            if (kind == 2) {
                NGSettingsViewController *sized = (id)page; [sized.supportFooter layoutIfNeeded];
                UIStackView *stack = (id)sized.supportFooter.subviews.firstObject; [stack layoutIfNeeded];
                for (UIButton *button in stack.arrangedSubviews) footerLayoutOK &= button.bounds.size.height>=48 && CGRectGetMaxX(button.frame)<=size.width && !button.hasAmbiguousLayout;
                footerLayoutOK &= stack.arrangedSubviews.count==2 && !stack.hasAmbiguousLayout;
            }
            [sizedNav willMoveToParentViewController:nil]; [sizedNav.view removeFromSuperview]; [sizedNav removeFromParentViewController];
        }
    }
    checks[@"compactAndLandscapeSupportLayout"] = @(layoutOK);
    checks[@"compactAndLandscapeInvitationLayout"] = @(communityLayoutOK);
    checks[@"compactAndLandscapeFooterLayout"] = @(footerLayoutOK);
    [[NSJSONSerialization dataWithJSONObject:supportLayouts options:NSJSONWritingPrettyPrinted error:nil] writeToFile:[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:@"support-layout.json"] atomically:YES];
    window.rootViewController = nav;
    [support goBack]; [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.45]]; checks[@"supportBackReturnsToSettings"] = @(nav.topViewController == community);
    NGSettingsViewController *home = [NGSettingsViewController new]; nav = [[UINavigationController alloc] initWithRootViewController:home]; window.rootViewController = nav; window.frame = NGBaseWindow().bounds; [nav.view layoutIfNeeded]; [home loadViewIfNeeded];
    checks[@"themeNeverForcedBySettings"] = @(home.overrideUserInterfaceStyle == UIUserInterfaceStyleUnspecified && community.overrideUserInterfaceStyle == UIUserInterfaceStyleUnspecified && invitations.overrideUserInterfaceStyle == UIUserInterfaceStyleUnspecified && support.overrideUserInterfaceStyle == UIUserInterfaceStyleUnspecified);
    window.overrideUserInterfaceStyle = UIUserInterfaceStyleLight; [window setNeedsLayout]; [window layoutIfNeeded]; [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.06]]; [nav.view layoutIfNeeded]; UIColor *light = [home.tableView.backgroundColor resolvedColorWithTraitCollection:home.traitCollection];
    window.overrideUserInterfaceStyle = UIUserInterfaceStyleDark; [window setNeedsLayout]; [window layoutIfNeeded]; [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.06]]; [nav.view layoutIfNeeded]; UIColor *dark = [home.tableView.backgroundColor resolvedColorWithTraitCollection:home.traitCollection];
    checks[@"sameOpenPageFollowsSystemTheme"] = @(![light isEqual:dark] && home.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark);
    window.overrideUserInterfaceStyle = UIUserInterfaceStyleLight; [window setNeedsLayout]; [window layoutIfNeeded]; [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.06]]; [nav.view layoutIfNeeded];
    checks[@"switchBackToLightWorks"] = @([[home.tableView.backgroundColor resolvedColorWithTraitCollection:home.traitCollection] isEqual:light]);
    checks[@"newAdaptiveHeroAndVersionBadge"] = @([home.brandHeader.badgeLabel.text isEqualToString:@"v0.0.4"] && home.tableView.backgroundView == nil && home.brandHeader.bounds.size.height <= 100);
    BOOL cardLayout = YES;
    for (NSUInteger i=0;i<5;i++) {
        NSIndexPath *path = [NSIndexPath indexPathForRow:0 inSection:i]; NGSettingsCategoryCell *cell = (id)[home tableView:home.tableView cellForRowAtIndexPath:path];
        cell.frame = CGRectMake(0,0,280,64); [cell setNeedsLayout]; [cell layoutIfNeeded];
        cardLayout &= CGRectGetMaxX(cell.summary.frame)<=CGRectGetMinX(cell.chevron.frame) && CGRectGetMaxY(cell.summary.frame)<=cell.surface.bounds.size.height && cell.tile.image != nil && cell.surface.layer.cornerRadius == 14;
    }
    checks[@"allFiveCardsFitNarrowScreen"] = @(cardLayout);
    UISearchController *search = home.navigationItem.searchController; BOOL searchable = YES;
    for (NSString *query in @[@"奶茶",@"QQ",@"TG 通知",@"TG 交流",@"字体"]) { search.searchBar.text = query; [home updateSearchResultsForSearchController:search]; searchable &= home.searchResults.count>0; }
    checks[@"sponsorAndAllGroupsAreSearchable"] = @(searchable);
    search.searchBar.text = @""; [home updateSearchResultsForSearchController:search];
    checks[@"compactHomeRowAndHero"] = @([home tableView:home.tableView heightForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]]<=80 && home.brandHeader.bounds.size.height<=200);
    NGTestSettingsReduceMotion = YES;
    NGTestEntranceController *reduced = [NGTestEntranceController new]; [reduced loadViewIfNeeded]; reduced.testTransition = [NGTestEntranceTransition new]; [reduced viewIsAppearing:YES];
    checks[@"reduceMotionSkipsSheenAndEntrance"] = @(reduced.testTransition.animation==nil);
    NGTestSettingsReduceMotion = NO;
    NGTestSupportPresenter *sharePage = [NGTestSupportPresenter new]; window.rootViewController = [[UINavigationController alloc] initWithRootViewController:sharePage]; [window.rootViewController loadViewIfNeeded]; [window.rootViewController.view layoutIfNeeded]; [window makeKeyAndVisible]; [window layoutIfNeeded]; [sharePage loadViewIfNeeded];
    [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.08]];
    [sharePage shareCode]; [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.45]]; UIActivityViewController *sheet = (id)sharePage.capturedPresentation;
    checks[@"shareSheetExcludesPhotoLibraryWrite"] = @([sheet isKindOfClass:UIActivityViewController.class] && [sheet.excludedActivityTypes containsObject:UIActivityTypeSaveToCameraRoll]);
    [sharePage dismissViewControllerAnimated:NO completion:nil];
    NGTestFailurePresenter *failurePage = [NGTestFailurePresenter new]; window.rootViewController = failurePage; [failurePage loadViewIfNeeded]; [window makeKeyAndVisible]; [window layoutIfNeeded]; [failurePage.view layoutIfNeeded]; [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    NGTestCommunityOpener = ^(NSURL *url, void (^completion)(BOOL)) { completion(NO); };
    NGOpenCommunity(@"qqGroup",failurePage); [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.08]];
    UIAlertController *failureAlert = (id)failurePage.capturedPresentation;
    checks[@"openFailureOffersCopyLink"] = @([failureAlert isKindOfClass:UIAlertController.class] && [failureAlert.actions.firstObject.title isEqualToString:@"复制链接"]);
    [failurePage dismissViewControllerAnimated:NO completion:nil]; NGTestCommunityOpener = nil;
    window.hidden = YES; [NGBaseWindow() makeKeyAndVisible]; return checks;
}
