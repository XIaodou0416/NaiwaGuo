// Settings-only adaptive design. No global UI appearance or forced color scheme.
static UIColor *NGSettingsColor(CGFloat r, CGFloat g, CGFloat b, CGFloat dr, CGFloat dg, CGFloat db) {
    return [UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
        BOOL dark = traits.userInterfaceStyle == UIUserInterfaceStyleDark;
        return [UIColor colorWithRed:(dark ? dr : r)/255.0 green:(dark ? dg : g)/255.0 blue:(dark ? db : b)/255.0 alpha:1];
    }];
}
static UIColor *NGSettingsAccent(void) { return UIColor.labelColor; }
static UIColor *NGSettingsCanvas(void) { return NGSettingsColor(246,247,247,18,19,21); }
static UIColor *NGSettingsSurface(void) { return NGSettingsColor(255,255,255,29,30,33); }
static UIColor *NGSettingsInk(void) { return NGSettingsColor(31,33,34,242,242,242); }
static UIColor *NGSettingsMuted(void) { return NGSettingsColor(117,120,122,162,165,168); }
static UIColor *NGSettingsLine(void) { return UIColor.separatorColor; }
static UIFont *NGSettingsFont(CGFloat size, UIFontWeight weight) {
    return [[UIFontMetrics metricsForTextStyle:UIFontTextStyleBody] scaledFontForFont:[UIFont systemFontOfSize:size weight:weight] maximumPointSize:size * 1.45];
}
static void NGSettingsFeedback(void) { [[[UISelectionFeedbackGenerator alloc] init] selectionChanged]; }

static UIImage *NGSettingsIcon(UIImage *symbol,CGFloat side) {
    if (!symbol) return nil;
    UIImageSymbolConfiguration *configuration=[UIImageSymbolConfiguration configurationWithPointSize:MIN(side,23) weight:UIImageSymbolWeightRegular];
    return [[symbol imageByApplyingSymbolConfiguration:configuration] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}

#if NG_TEST
static BOOL NGTestSettingsReduceMotion = NO;
#endif
static BOOL NGSettingsMotionEnabled(void) {
#if NG_TEST
    if (NGTestSettingsReduceMotion) return NO;
#endif
    return !UIAccessibilityIsReduceMotionEnabled();
}

@interface NGSettingsSectionIntro : UIView
@property(nonatomic, strong) UILabel *eyebrow;
@property(nonatomic, strong) UILabel *heading;
@property(nonatomic, strong) UILabel *caption;
@end
@implementation NGSettingsSectionIntro
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        _eyebrow = [UILabel new]; _eyebrow.hidden = YES;
        _eyebrow.font = [UIFont monospacedSystemFontOfSize:10 weight:UIFontWeightMedium]; _eyebrow.textColor = NGSettingsAccent(); [self addSubview:_eyebrow];
        _heading = [UILabel new]; _heading.font = NGSettingsFont(24, UIFontWeightSemibold); _heading.textColor = NGSettingsInk(); _heading.numberOfLines = 2; [self addSubview:_heading];
        _caption = [UILabel new]; _caption.font = NGSettingsFont(12, UIFontWeightRegular); _caption.textColor = NGSettingsMuted(); _caption.numberOfLines = 2; [self addSubview:_caption];
        self.isAccessibilityElement = YES; self.accessibilityTraits = UIAccessibilityTraitHeader;
    } return self;
}
- (void)layoutSubviews {
    [super layoutSubviews]; CGFloat w = MAX(0,self.bounds.size.width-48);
    _eyebrow.frame = CGRectMake(20,6,w,14);
    CGSize title = [_heading sizeThatFits:CGSizeMake(w,100)];
    _heading.frame = CGRectMake(20,10,w,title.height);
    _caption.frame = CGRectMake(20,CGRectGetMaxY(_heading.frame)+6,w,34);
    self.accessibilityLabel = [NSString stringWithFormat:@"%@，%@",_heading.text ?: @"",_caption.text ?: @""];
}
@end

@interface NGSettingsTableController : UITableViewController
@property(nonatomic) BOOL entranceRunning;
@property(nonatomic,strong) id accessObserver;
- (void)applyFeatureAccess;
@end
@implementation NGSettingsTableController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.tintColor = NGSettingsAccent(); self.tableView.backgroundColor = NGSettingsCanvas();
    self.tableView.backgroundView = nil;
    self.tableView.separatorColor = NGSettingsLine(); self.tableView.separatorInset = UIEdgeInsetsMake(0,20,0,20);
    self.tableView.cellLayoutMarginsFollowReadableWidth = YES;
    self.tableView.rowHeight = UITableViewAutomaticDimension; self.tableView.estimatedRowHeight = 62;
    self.tableView.sectionHeaderHeight = 28; self.tableView.sectionFooterHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedSectionFooterHeight = 24;
    self.tableView.contentInset = UIEdgeInsetsMake(4,0,28,0);
    __weak typeof(self) weakSelf=self;
    self.accessObserver=[NSNotificationCenter.defaultCenter addObserverForName:NGAccessChanged object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note){[weakSelf applyFeatureAccess];}];
}
- (void)dealloc { if(self.accessObserver)[NSNotificationCenter.defaultCenter removeObserver:self.accessObserver]; }
- (void)applyFeatureAccess { self.tableView.userInteractionEnabled=!NGBlocked;self.tableView.alpha=NGBlocked ? .42 : 1; }
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self applyFeatureAccess];
    UINavigationBarAppearance *appearance = [UINavigationBarAppearance new];
    [appearance configureWithTransparentBackground];
    appearance.backgroundEffect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterial];
    appearance.backgroundColor = [NGSettingsCanvas() colorWithAlphaComponent:0.74]; appearance.shadowColor = UIColor.clearColor;
    appearance.titleTextAttributes = @{NSFontAttributeName: NGSettingsFont(15,UIFontWeightSemibold), NSForegroundColorAttributeName:NGSettingsInk()};
    self.navigationItem.standardAppearance = appearance; self.navigationItem.scrollEdgeAppearance = appearance;
    self.navigationController.navigationBar.tintColor = NGSettingsInk();
    self.navigationItem.backButtonDisplayMode = UINavigationItemBackButtonDisplayModeMinimal;
    if (!self.tableView.tableHeaderView && self.title.length) {
        NGSettingsSectionIntro *intro = [[NGSettingsSectionIntro alloc] initWithFrame:CGRectMake(0,0,self.tableView.bounds.size.width,144)];
        intro.heading.text = self.title; intro.caption.text = @"更改会自动保存。"; self.tableView.tableHeaderView = intro;
    }
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    if ([self.tableView.tableHeaderView isKindOfClass:NGSettingsSectionIntro.class]) {
        NGSettingsSectionIntro *intro = (id)self.tableView.tableHeaderView;
        CGFloat width = self.tableView.bounds.size.width;
        CGFloat height = 52 + [intro.heading sizeThatFits:CGSizeMake(MAX(1,width-48),200)].height;
        if (fabs(intro.frame.size.width-width)>0.5 || fabs(intro.frame.size.height-height)>0.5) {
            intro.frame = CGRectMake(0,0,width,height); self.tableView.tableHeaderView = intro;
        }
    }
}
- (void)viewIsAppearing:(BOOL)animated {
    [super viewIsAppearing:animated];
    id<UIViewControllerTransitionCoordinator> transition = self.transitionCoordinator;
    if (NGBlocked || !animated || !transition || transition.initiallyInteractive || self.entranceRunning || !NGSettingsMotionEnabled()) return;
    self.entranceRunning = YES; [self.tableView layoutIfNeeded];
    NSArray<UITableViewCell *> *cells = self.tableView.visibleCells;
    UIView *header = self.tableView.tableHeaderView;
    header.alpha = 0; header.transform = CGAffineTransformMakeTranslation(0,14);
    for (UITableViewCell *cell in cells) {
        cell.contentView.alpha = 0;
        cell.contentView.transform = CGAffineTransformScale(CGAffineTransformMakeTranslation(0,8),0.995,0.995);
    }
    void (^restore)(void) = ^{
        header.alpha = 1; header.transform = CGAffineTransformIdentity;
        for (UITableViewCell *cell in cells) { cell.contentView.alpha = 1; cell.contentView.transform = CGAffineTransformIdentity; }
    };
    BOOL scheduled = [transition animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        [UIView animateWithDuration:0.48 delay:0 usingSpringWithDamping:0.9 initialSpringVelocity:0.25 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction animations:restore completion:nil];
    } completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        [UIView performWithoutAnimation:restore]; self.entranceRunning = NO;
    }];
    if (!scheduled) { restore(); self.entranceRunning = NO; }
}
- (void)viewDidAppear:(BOOL)animated { [super viewDidAppear:animated]; self.entranceRunning = NO; }
- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)path {
    if ([cell.accessibilityIdentifier hasPrefix:@"NaiwaGuo.Settings.Group."]) { cell.backgroundConfiguration = [UIBackgroundConfiguration clearConfiguration]; cell.imageView.tintColor=NGSettingsInk(); return; }
    cell.textLabel.font = NGSettingsFont(15,UIFontWeightMedium); cell.textLabel.adjustsFontForContentSizeCategory = YES; cell.textLabel.numberOfLines = 0;
    cell.detailTextLabel.font = NGSettingsFont(12,UIFontWeightRegular); cell.detailTextLabel.textColor = NGSettingsMuted(); cell.detailTextLabel.numberOfLines = 0;
    cell.imageView.tintColor=NGSettingsInk();
    cell.layoutMargins = UIEdgeInsetsMake(12,16,12,16); cell.tintColor = NGSettingsAccent();
    UIBackgroundConfiguration *background = [UIBackgroundConfiguration listGroupedCellConfiguration];
    background.backgroundColor = NGSettingsSurface(); background.cornerRadius = 16;
    cell.backgroundConfiguration = background;
    if ([cell.accessoryView isKindOfClass:UISwitch.class]) { ((UISwitch *)cell.accessoryView).onTintColor = NGSettingsAccent(); ((UISwitch *)cell.accessoryView).thumbTintColor=UIColor.systemBackgroundColor; }
}
- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section {
    if (![view isKindOfClass:UITableViewHeaderFooterView.class]) return;
    UILabel *label = ((UITableViewHeaderFooterView *)view).textLabel;
    label.font = NGSettingsFont(12,UIFontWeightSemibold); label.textColor = NGSettingsMuted();
}
- (void)tableView:(UITableView *)tableView didHighlightRowAtIndexPath:(NSIndexPath *)path {
    if (!NGSettingsMotionEnabled()) return;
    UITableViewCell *cell = [tableView cellForRowAtIndexPath:path];
    [UIView animateWithDuration:0.14 delay:0 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction animations:^{ cell.transform = CGAffineTransformMakeScale(0.99,0.99); } completion:nil];
}
- (void)tableView:(UITableView *)tableView didUnhighlightRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [tableView cellForRowAtIndexPath:path];
    [UIView animateWithDuration:!NGSettingsMotionEnabled() ? 0 : 0.38 delay:0 usingSpringWithDamping:0.72 initialSpringVelocity:0 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction animations:^{ cell.transform = CGAffineTransformIdentity; } completion:nil];
}
@end
