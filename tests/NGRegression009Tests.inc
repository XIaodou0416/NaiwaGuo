static NSDictionary *NGTestSettings009(void) {
    NSDictionary *saved=NGSavePreferences005(@[NGPreferenceDock,NGPreferenceFontScale]);
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    [defaults setBool:YES forKey:NGPreferenceDock]; [defaults setDouble:.7 forKey:NGPreferenceFontScale];
    NGSettingsViewController *home=[NGSettingsViewController new];
    UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home]; [home loadViewIfNeeded];
    BOOL available=home.tableView.userInteractionEnabled && home.navigationItem.searchController.searchBar.userInteractionEnabled;
    NSMutableSet *icons=[NSMutableSet set]; BOOL tiles=YES;
    for (NSUInteger index=0;index<home.groups.count;index++) {
        NGSettingsCategoryCell *cell=(id)[home tableView:home.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:index]];
        available &= cell.userInteractionEnabled && cell.contentView.alpha==1;
        tiles &= cell.tile.image.renderingMode==UIImageRenderingModeAlwaysOriginal && CGSizeEqualToSize(cell.tile.image.size,CGSizeMake(36,36));
        if (cell.tile.image) [icons addObject:UIImagePNGRepresentation(cell.tile.image)];
    }
    [home tableView:home.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    NGSettingsGroupViewController *detail=(id)nav.topViewController; [detail loadViewIfNeeded];
    BOOL navigation=[detail isKindOfClass:NGSettingsGroupViewController.class];
    UITableViewCell *row=[detail tableView:detail.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    [detail tableView:detail.tableView willDisplayCell:row forRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    BOOL detailIcon=row.imageView.image.renderingMode==UIImageRenderingModeAlwaysOriginal && CGSizeEqualToSize(row.imageView.image.size,CGSizeMake(30,30));
    NGInstallNativeSettings(); SSSettingViewController *host=[SSSettingViewController new]; [host loadViewIfNeeded];
    NGNativeEntry *entry=nil; for (UIView *view in host.tableView.tableHeaderView.subviews) if ([view isKindOfClass:NGNativeEntry.class]) entry=(id)view;
    BOOL name=[NGPluginName isEqual:@"奶蛙"] && [home.brandHeader.titleLabel.text isEqual:@"奶蛙"];
    BOOL native=entry && [entry.configuration.title isEqual:@"奶蛙"] && !entry.configuration.image && !entry.currentImage;
    BOOL preferences=[defaults boolForKey:NGPreferenceDock] && fabs([defaults doubleForKey:NGPreferenceFontScale]-.7)<.001;
    NGRestorePreferences005(saved);
    return @{@"allCategoriesAvailableWithoutHostLogin":@(available),@"categoryIconsHaveFixedColoredTiles":@(tiles),@"eachCategoryIconIsDistinct":@(icons.count==home.groups.count),@"detailRowIconUsesNewStyle":@(detailIcon),@"featureNavigationAvailable":@(navigation),@"chineseDisplayName":@(name),@"nativeEntryHasTextOnly":@(native),@"savedPreferencesReadWithoutMasking":@(preferences),@"noAuthorizationBanner":@(home.tableView.tableHeaderView==home.brandHeader)};
}

static void NGRunSettingsUI009(UIWindow *window,NSString *mode) {
    NGStart(); NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    BOOL preview=[mode isEqual:@"settings-preview009"],seed=[mode isEqual:@"sponsor-never-seed009"];
    NGSponsorTestPrompts=YES;
    if ([mode isEqual:@"native-entry009"]) {
        NGSponsorTestPrompts=NO;
        SSSettingViewController *host=[SSSettingViewController new]; host.title=@"设置";
        window.rootViewController=[[UINavigationController alloc] initWithRootViewController:host];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            NGNativeEntry *entry=nil; for(UIView *view in host.tableView.tableHeaderView.subviews)if([view isKindOfClass:NGNativeEntry.class])entry=(id)view;
            BOOL text=entry && [entry.configuration.title isEqual:@"奶蛙"] && !entry.configuration.image;
            NGSmokeImage(window,@"native-entry009.png"); [entry sendActionsForControlEvents:UIControlEventTouchUpInside];
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1*NSEC_PER_SEC),dispatch_get_main_queue(),^{
                UINavigationController *presented=(id)host.presentedViewController;
                BOOL opened=[presented isKindOfClass:UINavigationController.class] && [presented.topViewController isKindOfClass:NGSettingsViewController.class];
                NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(text && opened),@"textOnlyChineseEntry":@(text),@"entryOpensSettings":@(opened)});
            });
        }); return;
    }
    if (seed || preview) { [defaults setBool:YES forKey:NGPreferenceDock]; [defaults setDouble:.7 forKey:NGPreferenceFontScale]; }
    NGSettingsViewController *home=[NGSettingsViewController new];
    UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:home]; window.rootViewController=nav;
    if (preview || seed) {
        __block BOOL sponsor=NO;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            NGSponsorPromptController *prompt=(id)home.presentedViewController;
            sponsor=[prompt isKindOfClass:NGSponsorPromptController.class] && [prompt.messageLabel.text isEqual:NGSponsorPromptMessage];
            if(preview)NGSmokeImage(window,@"sponsor-prompt009.png");
            if(sponsor) { if(seed)[prompt neverPrompt]; else [prompt closePrompt]; }
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC),dispatch_get_main_queue(),^{
            BOOL available=!home.presentedViewController && home.tableView.visibleCells.count>0;
            for(UITableViewCell *cell in home.tableView.visibleCells)available &= cell.userInteractionEnabled && cell.contentView.alpha>.99;
            if(preview)NGSmokeImage(window,@"settings-unrestricted009.png");
            BOOL suppression=seed ? [defaults boolForKey:NGPreferenceSponsorNever] : ![defaults boolForKey:NGPreferenceSponsorNever];
            [home tableView:home.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
            BOOL navigates=[nav.topViewController isKindOfClass:NGSettingsGroupViewController.class];
            NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(sponsor && available && suppression && navigates),@"sponsorHasRequestedContent":@(sponsor),@"allVisibleFeaturesEnabledWithoutLogin":@(available),@"sponsorPreferenceCorrect":@(suppression),@"featureNavigationAvailable":@(navigates)});
        }); return;
    }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.6*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        BOOL hidden=!home.presentedViewController,preferences=[defaults boolForKey:NGPreferenceDock] && fabs([defaults doubleForKey:NGPreferenceFontScale]-.7)<.001;
        BOOL enabled=home.tableView.visibleCells.count>0; for(UITableViewCell *cell in home.tableView.visibleCells)enabled &= cell.contentView.alpha>.99 && cell.userInteractionEnabled;
        BOOL never=[defaults boolForKey:NGPreferenceSponsorNever];
        NGSmokeWrite([mode stringByAppendingString:@".json"],@{@"passed":@(hidden && preferences && enabled && never),@"noSponsorAfterNeverAgain":@(hidden && never),@"savedSettingsRetained":@(preferences),@"featuresAvailableAfterRestartWithoutLogin":@(enabled)});
    });
}
