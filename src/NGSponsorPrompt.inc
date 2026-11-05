static NSString *const NGPreferenceSponsorNever = @"com.naiwaguo.prompts.sponsorNever";
static NSString *const NGSponsorPromptMessage = @"可以请我喝杯奶茶吗，为了以后的更新和体验🥹如果你不想看见点击下面弹窗以后不会再弹出来🙏抱歉<(_ _)>";
@interface NGSponsorPromptController : UIViewController
@property(nonatomic,strong) UIVisualEffectView *card;
@property(nonatomic,strong) UIScrollView *scroll;
@property(nonatomic,strong) UIImageView *codeView;
@property(nonatomic,strong) UILabel *messageLabel;
@property(nonatomic,strong) UIButton *closeButton;
@property(nonatomic,strong) UIButton *neverButton;
@end
@implementation NGSponsorPromptController
- (instancetype)init { if ((self=[super init])) { self.modalPresentationStyle=UIModalPresentationOverFullScreen; self.modalTransitionStyle=UIModalTransitionStyleCrossDissolve; } return self; }
- (void)viewDidLoad {
    [super viewDidLoad]; self.view.backgroundColor=[UIColor.blackColor colorWithAlphaComponent:.35]; self.view.accessibilityIdentifier=@"NaiwaGuo.SponsorPrompt";
    self.card=NGSupportGlass(NO,28); self.card.translatesAutoresizingMaskIntoConstraints=NO; [self.view addSubview:self.card];
    self.scroll=[UIScrollView new]; self.scroll.translatesAutoresizingMaskIntoConstraints=NO; [self.card.contentView addSubview:self.scroll];
    UIStackView *body=[UIStackView new]; body.axis=UILayoutConstraintAxisVertical; body.spacing=16; body.translatesAutoresizingMaskIntoConstraints=NO; [self.scroll addSubview:body];
    self.messageLabel=[UILabel new]; self.messageLabel.text=NGSponsorPromptMessage; self.messageLabel.numberOfLines=0; self.messageLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody]; self.messageLabel.adjustsFontForContentSizeCategory=YES; self.messageLabel.textColor=UIColor.labelColor; [body addArrangedSubview:self.messageLabel];
    self.codeView=[[UIImageView alloc] initWithImage:[UIImage imageWithData:NGSponsorCodeData()]]; self.codeView.contentMode=UIViewContentModeScaleAspectFit; self.codeView.backgroundColor=UIColor.whiteColor; self.codeView.accessibilityLabel=@"作者赞赏码"; self.codeView.isAccessibilityElement=YES; [body addArrangedSubview:self.codeView];
    [self.codeView.heightAnchor constraintEqualToAnchor:self.codeView.widthAnchor multiplier:505.0/534.0].active=YES;
    UIStackView *buttons=[UIStackView new]; buttons.axis=UILayoutConstraintAxisVertical; buttons.spacing=8; buttons.translatesAutoresizingMaskIntoConstraints=NO; [self.card.contentView addSubview:buttons];
    self.closeButton=[UIButton buttonWithType:UIButtonTypeSystem]; self.neverButton=[UIButton buttonWithType:UIButtonTypeSystem];
    for (UIButton *button in @[self.closeButton,self.neverButton]) { button.configuration=[UIButtonConfiguration tintedButtonConfiguration]; [buttons addArrangedSubview:button]; [button.heightAnchor constraintGreaterThanOrEqualToConstant:44].active=YES; }
    [self.closeButton setTitle:@"直接关闭" forState:UIControlStateNormal]; [self.closeButton addTarget:self action:@selector(closePrompt) forControlEvents:UIControlEventTouchUpInside]; self.closeButton.accessibilityIdentifier=@"NaiwaGuo.SponsorPrompt.Close";
    [self.neverButton setTitle:@"以后再也不弹" forState:UIControlStateNormal]; [self.neverButton addTarget:self action:@selector(neverPrompt) forControlEvents:UIControlEventTouchUpInside]; self.neverButton.accessibilityIdentifier=@"NaiwaGuo.SponsorPrompt.Never";
    NSLayoutConstraint *width=[self.card.widthAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.widthAnchor constant:-32]; width.priority=999;
    NSLayoutConstraint *height=[self.card.heightAnchor constraintEqualToConstant:560]; height.priority=750;
    [NSLayoutConstraint activateConstraints:@[width,height,[self.card.widthAnchor constraintLessThanOrEqualToConstant:390],[self.card.heightAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.heightAnchor constant:-32],[self.card.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],[self.card.centerYAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.centerYAnchor],
        [self.scroll.topAnchor constraintEqualToAnchor:self.card.contentView.topAnchor constant:20],[self.scroll.leadingAnchor constraintEqualToAnchor:self.card.contentView.leadingAnchor constant:20],[self.scroll.trailingAnchor constraintEqualToAnchor:self.card.contentView.trailingAnchor constant:-20],[self.scroll.bottomAnchor constraintEqualToAnchor:buttons.topAnchor constant:-16],
        [body.leadingAnchor constraintEqualToAnchor:self.scroll.contentLayoutGuide.leadingAnchor],[body.trailingAnchor constraintEqualToAnchor:self.scroll.contentLayoutGuide.trailingAnchor],[body.topAnchor constraintEqualToAnchor:self.scroll.contentLayoutGuide.topAnchor],[body.bottomAnchor constraintEqualToAnchor:self.scroll.contentLayoutGuide.bottomAnchor],[body.widthAnchor constraintEqualToAnchor:self.scroll.frameLayoutGuide.widthAnchor],
        [buttons.leadingAnchor constraintEqualToAnchor:self.card.contentView.leadingAnchor constant:20],[buttons.trailingAnchor constraintEqualToAnchor:self.card.contentView.trailingAnchor constant:-20],[buttons.bottomAnchor constraintEqualToAnchor:self.card.contentView.bottomAnchor constant:-16]]];
}
- (void)closePrompt { [self dismissViewControllerAnimated:YES completion:nil]; }
- (void)neverPrompt { [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceSponsorNever]; [self closePrompt]; }
@end
