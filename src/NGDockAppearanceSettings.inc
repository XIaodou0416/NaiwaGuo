@interface NGDockAppearanceSettingsController : NGDockPreviewSettingsController @end
@implementation NGDockAppearanceSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title=@"底栏外观与收缩"; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 6; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section==0 || section==5 ? 1 : (section==1 ? 2 : 3); }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return @[@"底栏",@"液态材质",@"自动收缩",@"底栏内容",@"尺寸",@""][section]; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section==1) return @"普通为系统磨砂，清透保留更多背景细节；两块玻璃同步，跟随系统浅深色。";
    if (section==2) return @"上滑收为当前页圆钮，稍微下滑或点击圆钮展开。收缩后搜索圆钮隐藏，空余位置可点击。仅 iPhone / iOS 26 及以上。";
    if (section==4) return @"100% 为默认比例。胶囊高度等比缩放、长度独立调节、搜索圆钮单独缩放；底边位置保持。";
    return nil;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { return path.section==4 ? 74 : UITableViewAutomaticDimension; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil]; NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    if (path.section==0) { cell.textLabel.text=@"悬浮液态底栏"; UISwitch *toggle=[UISwitch new]; toggle.on=[d boolForKey:NGPreferenceDock]; [toggle addTarget:self action:@selector(enabledChanged:) forControlEvents:UIControlEventValueChanged]; cell.accessoryView=toggle; }
    else if (path.section==1) { cell.textLabel.text=@[@"普通液态",@"清透液态"][path.row]; cell.accessoryType=path.row==[d boolForKey:NGPreferenceDockClear] ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==2) { cell.textLabel.text=@[@"关闭",@"收缩到左侧",@"收缩到右侧"][path.row]; cell.accessoryType=path.row==NGDockCollapseSide() ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==3) { cell.textLabel.text=@[@"文字",@"仅图标",@"图标与文字"][path.row]; cell.accessoryType=path.row==NGDockContentMode() ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone; }
    else if (path.section==4) {
        NSString *key=@[NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize][path.row];
        UILabel *label=[UILabel new]; label.text=[NSString stringWithFormat:@"%@  %.0f%%",@[@"胶囊高度",@"胶囊长度",@"搜索圆钮"][path.row],NGDockSize(key)*100]; label.font=NGSettingsFont(13,UIFontWeightMedium); label.tag=9;
        UISlider *slider=[UISlider new]; slider.minimumValue=path.row==1 ? 80 : 60; slider.maximumValue=100; slider.value=NGDockSize(key)*100; slider.accessibilityIdentifier=key; slider.accessibilityLabel=@[@"胶囊高度",@"胶囊长度",@"搜索圆钮大小"][path.row]; [slider addTarget:self action:@selector(sizeChanged:) forControlEvents:UIControlEventValueChanged];
        for (UIView *view in @[label,slider]) { view.translatesAutoresizingMaskIntoConstraints=NO; [cell.contentView addSubview:view]; }
        [NSLayoutConstraint activateConstraints:@[[label.leadingAnchor constraintEqualToAnchor:cell.contentView.leadingAnchor constant:16],[label.topAnchor constraintEqualToAnchor:cell.contentView.topAnchor constant:8],[slider.leadingAnchor constraintEqualToAnchor:label.leadingAnchor],[slider.trailingAnchor constraintEqualToAnchor:cell.contentView.trailingAnchor constant:-16],[slider.topAnchor constraintEqualToAnchor:label.bottomAnchor constant:3],[slider.bottomAnchor constraintEqualToAnchor:cell.contentView.bottomAnchor constant:-7]]];
    } else cell.textLabel.text=@"恢复底栏默认外观";
    return cell;
}
- (void)refreshDockPreview { NGUpdateDock(); [self.preview refreshIcons]; [self.preview layoutGlass]; }
- (void)enabledChanged:(UISwitch *)sender { [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:NGPreferenceDock]; [self refreshDockPreview]; }
- (void)sizeChanged:(UISlider *)sender {
    [NSUserDefaults.standardUserDefaults setDouble:round(sender.value) forKey:sender.accessibilityIdentifier]; UILabel *label=[sender.superview viewWithTag:9];
    NSString *title=[sender.accessibilityIdentifier isEqualToString:NGPreferenceDockHeight] ? @"胶囊高度" : ([sender.accessibilityIdentifier isEqualToString:NGPreferenceDockLength] ? @"胶囊长度" : @"搜索圆钮"); label.text=[NSString stringWithFormat:@"%@  %.0f%%",title,round(sender.value)]; [self refreshDockPreview];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES]; NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    if (path.section==1) [d setBool:path.row==1 forKey:NGPreferenceDockClear];
    else if (path.section==2) [d setInteger:path.row forKey:NGPreferenceDockCollapse];
    else if (path.section==3) [d setInteger:path.row forKey:NGPreferenceDockContent];
    else if (path.section==5) for (NSString *key in @[NGPreferenceDockClear,NGPreferenceDockCollapse,NGPreferenceDockContent,NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize]) [d removeObjectForKey:key];
    else return;
    [self refreshDockPreview]; [self.tableView reloadData];
}
@end
