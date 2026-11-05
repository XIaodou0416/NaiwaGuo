@interface NGDockAppearanceSettingsController : NGDockPreviewSettingsController @end
@implementation NGDockAppearanceSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title=@"底栏外观与收缩"; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 8; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return [@[@2,@2,@4,@3,@3,@6,@2,@1][section] integerValue]; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return @[@"底栏",@"液态材质",@"自动收缩",@"底栏内容",@"尺寸",@"独立按钮功能",@"按钮图案",@""][section]; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section==0) return @"按键触动反馈默认关闭，开启后点击底栏和独立按钮时轻震。";
    if (section==1) return @"普通为系统磨砂，清透保留更多背景细节；两块玻璃同步，跟随系统浅深色。";
    if (section==2) return @"上滑收为当前页圆钮，可选左侧、右侧或居中；下滑或点击圆钮展开。收缩后独立按钮隐藏，空余位置可点击。仅 iPhone / iOS 26 及以上。";
    if (section==4) return @"100% 为默认比例。胶囊高度等比缩放、长度独立调节、独立圆钮单独缩放；底边位置保持。";
    if (section==6) return @"红果头像用于独立按钮及居中收缩圆钮。首次未显示时，打开一次红果侧栏同步；未获取到时显示默认头像。";
    return nil;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { return path.section==4 ? 74 : UITableViewAutomaticDimension; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil]; NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    if (path.section==0) {
        BOOL haptic=path.row==1; NSString *key=haptic ? NGPreferenceDockHaptics : NGPreferenceDock;
        cell.textLabel.text=haptic ? @"按键触动反馈" : @"悬浮液态底栏";
        UISwitch *toggle=[UISwitch new]; toggle.on=[d boolForKey:key]; toggle.accessibilityIdentifier=key;
        [toggle addTarget:self action:haptic ? @selector(hapticsChanged:) : @selector(enabledChanged:) forControlEvents:UIControlEventValueChanged]; cell.accessoryView=toggle;
    }
    else if (path.section==1) { cell.textLabel.text=@[@"普通液态",@"清透液态"][path.row]; cell.accessoryType=path.row==[d boolForKey:NGPreferenceDockClear] ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==2) { cell.textLabel.text=@[@"关闭",@"收缩到左侧",@"收缩到右侧",@"收缩到中间"][path.row]; cell.accessoryType=path.row==NGDockCollapseSide() ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==3) { cell.textLabel.text=@[@"文字",@"仅图标",@"图标与文字"][path.row]; cell.accessoryType=path.row==NGDockContentMode() ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==4) {
        NSString *key=@[NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize][path.row];
        UILabel *label=[UILabel new]; label.text=[NSString stringWithFormat:@"%@  %.0f%%",@[@"胶囊高度",@"胶囊长度",@"独立圆钮"][path.row],NGDockSize(key)*100]; label.font=NGSettingsFont(13,UIFontWeightMedium); label.tag=9;
        UISlider *slider=[UISlider new]; slider.minimumValue=path.row==1 ? 80 : 60; slider.maximumValue=100; slider.value=NGDockSize(key)*100; slider.accessibilityIdentifier=key; slider.accessibilityLabel=@[@"胶囊高度",@"胶囊长度",@"独立圆钮大小"][path.row]; [slider addTarget:self action:@selector(sizeChanged:) forControlEvents:UIControlEventValueChanged];
        for (UIView *view in @[label,slider]) { view.translatesAutoresizingMaskIntoConstraints=NO; [cell.contentView addSubview:view]; }
        [NSLayoutConstraint activateConstraints:@[[label.leadingAnchor constraintEqualToAnchor:cell.contentView.leadingAnchor constant:16],[label.topAnchor constraintEqualToAnchor:cell.contentView.topAnchor constant:8],[slider.leadingAnchor constraintEqualToAnchor:label.leadingAnchor],[slider.trailingAnchor constraintEqualToAnchor:cell.contentView.trailingAnchor constant:-16],[slider.topAnchor constraintEqualToAnchor:label.bottomAnchor constant:3],[slider.bottomAnchor constraintEqualToAnchor:cell.contentView.bottomAnchor constant:-7]]];
    } else if (path.section==5) { cell.textLabel.text=NGDockActionTitles()[path.row]; cell.accessoryType=[NGDockActions()[path.row] isEqual:NGDockAction()] ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==6) { cell.textLabel.text=@[@"功能图标",@"红果头像"][path.row]; cell.accessoryType=(path.row==1)==NGDockAvatarEnabled() ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else cell.textLabel.text=@"恢复底栏默认外观";
    return cell;
}
- (void)refreshDockPreview { NGUpdateDock(); [self.preview refreshIcons]; [self.preview layoutGlass]; }
- (void)enabledChanged:(UISwitch *)sender { [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:NGPreferenceDock]; NGPrepareRuntimeGlass(); [self refreshDockPreview]; }
- (void)hapticsChanged:(UISwitch *)sender { [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:NGPreferenceDockHaptics]; }
- (void)sizeChanged:(UISlider *)sender {
    [NSUserDefaults.standardUserDefaults setDouble:round(sender.value) forKey:sender.accessibilityIdentifier]; UILabel *label=[sender.superview viewWithTag:9];
    NSString *title=[sender.accessibilityIdentifier isEqualToString:NGPreferenceDockHeight] ? @"胶囊高度" : ([sender.accessibilityIdentifier isEqualToString:NGPreferenceDockLength] ? @"胶囊长度" : @"独立圆钮"); label.text=[NSString stringWithFormat:@"%@  %.0f%%",title,round(sender.value)]; [self refreshDockPreview];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES]; NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    if (path.section==1) [d setBool:path.row==1 forKey:NGPreferenceDockClear];
    else if (path.section==2) [d setInteger:path.row forKey:NGPreferenceDockCollapse];
    else if (path.section==3) [d setInteger:path.row forKey:NGPreferenceDockContent];
    else if (path.section==5) [d setObject:NGDockActions()[path.row] forKey:NGPreferenceDockAction];
    else if (path.section==6) [d setBool:path.row==1 forKey:NGPreferenceDockAvatar];
    else if (path.section==7) for (NSString *key in @[NGPreferenceDockAction,NGPreferenceDockAvatar,NGPreferenceDockHaptics,NGPreferenceDockClear,NGPreferenceDockCollapse,NGPreferenceDockContent,NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize]) [d removeObjectForKey:key];
    else return;
    [self refreshDockPreview]; [self.tableView reloadData];
}
@end
