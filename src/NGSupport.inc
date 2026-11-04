static NSData *NGSponsorCodeData(void) {
    static NSData *data; static dispatch_once_t once;
    dispatch_once(&once, ^{ data = [[NSData alloc] initWithBase64EncodedString:
#include "NGSponsorImage.inc"
        options:0]; });
    return data;
}
static NSURL *NGCommunityURL(NSString *action) {
    if (!action.length) return nil;
    NSDictionary *links = @{
        @"qqGroup": @"https://qm.qq.com/q/w9rb2DnAoE",
        @"tgNews": @"https://t.me/beans0416",
        @"tgChat": @"https://t.me/beans1011"
    };
    NSString *value = links[action]; return value ? [NSURL URLWithString:value] : nil;
}
#if NG_TEST
static void (^NGTestCommunityOpener)(NSURL *, void (^)(BOOL));
#endif
static void NGOpenCommunity(NSString *action, UIViewController *presenter) {
    NSURL *url = NGCommunityURL(action); if (!url) return;
    NGSettingsFeedback();
    void (^completion)(BOOL) = ^(BOOL opened) {
        if (opened) return;
        void (^showFailure)(void) = ^{
            if (!presenter.view.window || presenter.presentedViewController) return;
            UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"暂时无法打开链接" message:@"可复制链接后，在浏览器或对应应用中打开。" preferredStyle:UIAlertControllerStyleAlert];
            [alert addAction:[UIAlertAction actionWithTitle:@"复制链接" style:UIAlertActionStyleDefault handler:^(UIAlertAction *item) { UIPasteboard.generalPasteboard.URL = url; }]];
            [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
            [presenter presentViewController:alert animated:YES completion:nil];
        };
        if (NSThread.isMainThread) showFailure(); else dispatch_async(dispatch_get_main_queue(),showFailure);
    };
#if NG_TEST
    if (NGTestCommunityOpener) { NGTestCommunityOpener(url,completion); return; }
#endif
    [UIApplication.sharedApplication openURL:url options:@{} completionHandler:completion];
}

// Invitation and support layouts follow the author's NaiwaEase handoff UI.
// Account/authorization features and payment assets are not shared between apps.
static UIVisualEffectView *NGSupportGlass(BOOL interactive, CGFloat radius) {
    UIVisualEffect *effect;
    if (@available(iOS 26.0, *)) {
        UIGlassEffect *glass = [UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
        glass.interactive = interactive; effect = glass;
    } else { effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial]; }
    UIVisualEffectView *view = [[UIVisualEffectView alloc] initWithEffect:effect];
    view.layer.cornerRadius = radius; view.layer.cornerCurve = kCACornerCurveContinuous;
    view.clipsToBounds = YES; view.translatesAutoresizingMaskIntoConstraints = NO;
    return view;
}

@interface NGCommunityCard : UIControl
@property(nonatomic, strong) UIVisualEffectView *glass;
@property(nonatomic, copy) NSString *action;
- (instancetype)initWithTitle:(NSString *)title detail:(NSString *)detail symbol:(NSString *)symbol color:(UIColor *)color action:(NSString *)action;
@end
@implementation NGCommunityCard
- (instancetype)initWithTitle:(NSString *)title detail:(NSString *)detail symbol:(NSString *)symbol color:(UIColor *)color action:(NSString *)action {
    if ((self = [super initWithFrame:CGRectZero])) {
        _action = [action copy]; self.accessibilityIdentifier = [@"NaiwaGuo.Community." stringByAppendingString:action];
        self.isAccessibilityElement = YES; self.accessibilityTraits = UIAccessibilityTraitButton;
        NSString *verb = [action isEqualToString:@"tgNews"] ? @"查看通知" : @"加入群聊";
        self.accessibilityLabel = [NSString stringWithFormat:@"%@，%@，%@", title, detail, verb];
        _glass = NGSupportGlass(YES,24); _glass.userInteractionEnabled = NO; [self addSubview:_glass];
        UIStackView *content = [UIStackView new]; content.axis = UILayoutConstraintAxisVertical;
        content.spacing = 12; content.translatesAutoresizingMaskIntoConstraints = NO; [_glass.contentView addSubview:content];
        UILabel *invitation = [UILabel new]; invitation.text = @"邀请你加入";
        invitation.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote]; invitation.adjustsFontForContentSizeCategory = YES;
        invitation.textColor = UIColor.secondaryLabelColor; [content addArrangedSubview:invitation];
        UIStackView *row = [UIStackView new]; row.axis = UILayoutConstraintAxisHorizontal; row.spacing = 14; row.alignment = UIStackViewAlignmentCenter;
        UIImageView *icon = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:symbol]];
        icon.contentMode = UIViewContentModeScaleAspectFit; icon.tintColor = color;
        [icon.widthAnchor constraintEqualToConstant:42].active = YES; [icon.heightAnchor constraintEqualToConstant:42].active = YES; [row addArrangedSubview:icon];
        UIStackView *words = [UIStackView new]; words.axis = UILayoutConstraintAxisVertical; words.spacing = 5;
        UILabel *name = [UILabel new]; name.text = title; name.font = [UIFont preferredFontForTextStyle:UIFontTextStyleTitle3];
        name.numberOfLines = 0; name.adjustsFontForContentSizeCategory = YES; [words addArrangedSubview:name];
        UILabel *subtitle = [UILabel new]; subtitle.text = detail; subtitle.font = [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
        subtitle.numberOfLines = 0; subtitle.adjustsFontForContentSizeCategory = YES; subtitle.textColor = UIColor.secondaryLabelColor; [words addArrangedSubview:subtitle];
        [row addArrangedSubview:words]; [content addArrangedSubview:row];
        UILabel *join = [UILabel new]; join.text = [verb stringByAppendingString:@"  ›"]; join.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
        join.adjustsFontForContentSizeCategory = YES; join.textColor = color; join.textAlignment = NSTextAlignmentRight; [content addArrangedSubview:join];
        [NSLayoutConstraint activateConstraints:@[
            [_glass.leadingAnchor constraintEqualToAnchor:self.leadingAnchor], [_glass.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
            [_glass.topAnchor constraintEqualToAnchor:self.topAnchor], [_glass.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
            [content.leadingAnchor constraintEqualToAnchor:_glass.contentView.leadingAnchor constant:20], [content.trailingAnchor constraintEqualToAnchor:_glass.contentView.trailingAnchor constant:-20],
            [content.topAnchor constraintEqualToAnchor:_glass.contentView.topAnchor constant:18], [content.bottomAnchor constraintEqualToAnchor:_glass.contentView.bottomAnchor constant:-18]
        ]];
    } return self;
}
- (void)setHighlighted:(BOOL)highlighted { [super setHighlighted:highlighted]; self.alpha = highlighted ? 0.65 : 1; }
@end

@interface NGCommunityViewController : UIViewController
@property(nonatomic, strong) UIScrollView *scrollView;
@property(nonatomic, strong) UIStackView *cards;
@end
@implementation NGCommunityViewController
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"交流群"; self.view.backgroundColor = UIColor.systemGroupedBackgroundColor;
    self.view.accessibilityIdentifier = @"NaiwaGuo.Community";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
    self.navigationItem.leftBarButtonItem.accessibilityLabel = @"返回";
    _scrollView = [UIScrollView new]; _scrollView.alwaysBounceVertical = YES; _scrollView.translatesAutoresizingMaskIntoConstraints = NO; [self.view addSubview:_scrollView];
    _cards = [UIStackView new]; _cards.axis = UILayoutConstraintAxisVertical; _cards.spacing = 16; _cards.translatesAutoresizingMaskIntoConstraints = NO; [_scrollView addSubview:_cards];
    NSArray *groups = @[
        @[@"QQ 交流群", @"所有用户均可加入", @"person.2.fill", UIColor.systemBlueColor, @"qqGroup"],
        @[@"TG 通知群", @"接收公告与更新消息", @"megaphone.fill", UIColor.systemOrangeColor, @"tgNews"],
        @[@"TG 交流群", @"交流体验与使用建议", @"bubble.left.and.bubble.right.fill", UIColor.systemTealColor, @"tgChat"]
    ];
    for (NSArray *group in groups) {
        NGCommunityCard *card = [[NGCommunityCard alloc] initWithTitle:group[0] detail:group[1] symbol:group[2] color:group[3] action:group[4]];
        [card addTarget:self action:@selector(join:) forControlEvents:UIControlEventTouchUpInside]; [_cards addArrangedSubview:card];
    }
    [NSLayoutConstraint activateConstraints:@[
        [_scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor], [_scrollView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [_scrollView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor], [_scrollView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
        [_cards.leadingAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.leadingAnchor constant:16], [_cards.trailingAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.trailingAnchor constant:-16],
        [_cards.topAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.topAnchor constant:20], [_cards.bottomAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.bottomAnchor constant:-24],
        [_cards.widthAnchor constraintEqualToAnchor:_scrollView.frameLayoutGuide.widthAnchor constant:-32]
    ]];
}
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (void)join:(NGCommunityCard *)card { NGOpenCommunity(card.action,self); }
@end

@interface NGSupportViewController : UIViewController
@property(nonatomic, strong) UIScrollView *scrollView;
@property(nonatomic, strong) UIVisualEffectView *glassCard;
@property(nonatomic, strong) UIStackView *stack;
@property(nonatomic, strong) UIImageView *codeView;
@property(nonatomic, strong) UIButton *shareButton;
@end
@implementation NGSupportViewController
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"请作者喝杯奶茶吧";
    self.view.backgroundColor = UIColor.systemGroupedBackgroundColor; self.view.tintColor = NGSettingsAccent();
    self.view.accessibilityIdentifier = @"NaiwaGuo.Support";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
    self.navigationItem.leftBarButtonItem.accessibilityLabel = @"返回";
    _scrollView = [UIScrollView new]; _scrollView.alwaysBounceVertical = YES; _scrollView.translatesAutoresizingMaskIntoConstraints = NO; [self.view addSubview:_scrollView];
    _glassCard = NGSupportGlass(NO,28); _glassCard.accessibilityIdentifier = @"NaiwaGuo.Support.Glass"; [_scrollView addSubview:_glassCard];
    _stack = [UIStackView new]; _stack.axis = UILayoutConstraintAxisVertical; _stack.spacing = 16;
    _stack.translatesAutoresizingMaskIntoConstraints = NO; [_glassCard.contentView addSubview:_stack];
    _codeView = [[UIImageView alloc] initWithImage:[UIImage imageWithData:NGSponsorCodeData()]];
    _codeView.contentMode = UIViewContentModeScaleAspectFit; _codeView.backgroundColor = UIColor.whiteColor;
    _codeView.layer.cornerRadius = 16; _codeView.clipsToBounds = YES;
    _codeView.accessibilityIdentifier = @"NaiwaGuo.Support.Code"; _codeView.isAccessibilityElement = YES; _codeView.accessibilityLabel = @"作者赞赏码，原图完整展示";
    [_stack addArrangedSubview:_codeView]; [_codeView.heightAnchor constraintEqualToAnchor:_codeView.widthAnchor multiplier:505.0/534.0].active = YES;
    UILabel *hint = [UILabel new]; hint.text = @"自愿支持，感谢每一份心意。"; hint.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    hint.adjustsFontForContentSizeCategory = YES; hint.numberOfLines = 0; hint.textAlignment = NSTextAlignmentCenter; [_stack addArrangedSubview:hint];
    _shareButton = [UIButton buttonWithType:UIButtonTypeSystem]; _shareButton.accessibilityIdentifier = @"NaiwaGuo.Support.Share";
    UIButtonConfiguration *configuration;
    if (@available(iOS 26.0, *)) configuration = [UIButtonConfiguration glassButtonConfiguration];
    else configuration = [UIButtonConfiguration tintedButtonConfiguration];
    configuration.title = @"分享赞赏码"; configuration.image = [UIImage systemImageNamed:@"square.and.arrow.up"]; configuration.imagePadding = 8;
    _shareButton.configuration = configuration; [_shareButton addTarget:self action:@selector(shareCode) forControlEvents:UIControlEventTouchUpInside];
    [_stack addArrangedSubview:_shareButton]; [_shareButton.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;
    NSLayoutConstraint *preferredWidth = [_glassCard.widthAnchor constraintEqualToAnchor:_scrollView.frameLayoutGuide.widthAnchor constant:-32]; preferredWidth.priority = 999;
    [NSLayoutConstraint activateConstraints:@[
        [_scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor], [_scrollView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [_scrollView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor], [_scrollView.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
        [_scrollView.contentLayoutGuide.widthAnchor constraintEqualToAnchor:_scrollView.frameLayoutGuide.widthAnchor], preferredWidth,
        [_glassCard.widthAnchor constraintLessThanOrEqualToConstant:430], [_glassCard.centerXAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.centerXAnchor],
        [_glassCard.topAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.topAnchor constant:20], [_glassCard.bottomAnchor constraintEqualToAnchor:_scrollView.contentLayoutGuide.bottomAnchor constant:-24],
        [_stack.leadingAnchor constraintEqualToAnchor:_glassCard.contentView.leadingAnchor constant:20], [_stack.trailingAnchor constraintEqualToAnchor:_glassCard.contentView.trailingAnchor constant:-20],
        [_stack.topAnchor constraintEqualToAnchor:_glassCard.contentView.topAnchor constant:20], [_stack.bottomAnchor constraintEqualToAnchor:_glassCard.contentView.bottomAnchor constant:-20]
    ]];
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    UINavigationBarAppearance *appearance = [UINavigationBarAppearance new]; [appearance configureWithTransparentBackground];
    appearance.backgroundEffect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterial]; appearance.backgroundColor = [NGSettingsCanvas() colorWithAlphaComponent:0.74]; appearance.shadowColor = UIColor.clearColor;
    appearance.titleTextAttributes = @{NSFontAttributeName: NGSettingsFont(15,UIFontWeightSemibold),NSForegroundColorAttributeName:NGSettingsInk()};
    self.navigationItem.standardAppearance = appearance; self.navigationItem.scrollEdgeAppearance = appearance; self.navigationController.navigationBar.tintColor = NGSettingsInk();
}
- (void)viewIsAppearing:(BOOL)animated {
    [super viewIsAppearing:animated]; id<UIViewControllerTransitionCoordinator> transition = self.transitionCoordinator;
    if (!animated || !transition || transition.initiallyInteractive || !NGSettingsMotionEnabled()) return;
    self.stack.alpha = 0; self.stack.transform = CGAffineTransformMakeTranslation(0,20);
    void (^restore)(void) = ^{ self.stack.alpha = 1; self.stack.transform = CGAffineTransformIdentity; };
    BOOL scheduled = [transition animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) { restore(); } completion:^(id<UIViewControllerTransitionCoordinatorContext> context) { restore(); }];
    if (!scheduled) restore();
}
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (void)shareCode {
    NGSettingsFeedback(); UIImage *image = [UIImage imageWithData:NGSponsorCodeData()]; if (!image || self.presentedViewController || self.navigationController.presentedViewController) return;
    UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[image] applicationActivities:nil];
    // Do not ask the host app for Photos write permission just to display/share a code.
    share.excludedActivityTypes = @[UIActivityTypeSaveToCameraRoll];
    share.popoverPresentationController.sourceView = self.shareButton; share.popoverPresentationController.sourceRect = self.shareButton.bounds;
    [self presentViewController:share animated:YES completion:nil];
}
@end
