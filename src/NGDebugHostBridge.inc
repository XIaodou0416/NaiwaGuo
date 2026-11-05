// Hongguo-specific data for the DYKiller-derived read-only diagnostic exporter.
static NGSystemDock *NGDebugCurrentDock(void) {
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithArray:NGDDebugActiveWindows()];
    for (NSUInteger i=0;i<queue.count && i<12000;i++) {
        UIView *view=queue[i];
        NGSystemDock *dock=objc_getAssociatedObject(view,&NGDockNativeOwnerKey);
        if (dock && !view.hidden) return dock;
        if ([view isKindOfClass:NGDockSurface.class] && ((NGDockSurface *)view).dock.host) return ((NGDockSurface *)view).dock;
        [queue addObjectsFromArray:view.subviews];
    }
    return nil;
}
UITabBar *NGDGlassTabBarCurrent(void) { return NGDebugCurrentDock().tabBar; }
UIVisualEffectView *NGDGlassPlusKeyCurrent(void) { return NGDebugCurrentDock().searchKey; }
UIView *NGDGlassPlatterCurrent(void) { NGSystemDock *dock=NGDebugCurrentDock(); return dock.collapsed ? dock.collapsePlatter : dock.platter; }
void NGDebugOpenSettings(void) { NGOpenSettings(); }

static NSDictionary *NGDebugMethod(NSString *name,NSString *selectorName) {
    Class cls=NSClassFromString(name); SEL sel=NSSelectorFromString(selectorName);
    Method method=cls ? class_getInstanceMethod(cls,sel) : NULL;
    if (!method) return @{@"class":name,@"selector":selectorName,@"available":@NO};
    IMP imp=method_getImplementation(method); Dl_info info={0}; dladdr((const void *)imp,&info);
    return @{@"class":name,@"selector":selectorName,@"available":@YES,@"currentIMP":[NSString stringWithFormat:@"%p",imp],@"image":info.dli_fname ? @(info.dli_fname).lastPathComponent : @"",@"symbol":info.dli_sname ? @(info.dli_sname) : @"",@"encoding":@(method_getTypeEncoding(method))};
}
NSString *NGDTabBarProbeReport(void) {
    if (!NSThread.isMainThread) return @"NaiwaGuo: diagnostic snapshot requires the main thread.\n";
    NGSystemDock *dock=NGDebugCurrentDock();
    NSMutableDictionary *preferences=[NSMutableDictionary dictionary];
    for (NSString *key in @[NGPreferenceDock,NGPreferenceDockClear,NGPreferenceDockCollapse,NGPreferenceDockHeight,NGPreferenceDockLength,NGPreferenceDockSearchSize,NGPreferenceDockContent,NGPreferenceDockAvatar,NGPreferenceDockAction,NGPreferenceDockHaptics,NGPreferenceDockSelectedColor,NGPreferenceDockNormalColor,NGPreferencePlaybackPreset,NGPreferencePlaybackFullscreen,NGPreferenceCollectionGlass,NGPreferencePlaybackGlass,NGPreferenceCommentGlass,NGPreferenceShareGlass,NGPreferenceFontScale,NGPreferenceFontName]) {
        id value=NGFeatureObject(key);
        if ([value isKindOfClass:NSString.class] || [value isKindOfClass:NSNumber.class]) preferences[key]=value;
        if (([key isEqual:NGPreferenceDockSelectedColor] || [key isEqual:NGPreferenceDockNormalColor]) && [value isKindOfClass:NSArray.class] && [value count]==4) preferences[key]=value;
    }
    NSMutableArray *effects=[NSMutableArray array];
    NSMutableArray *queue=[NSMutableArray arrayWithArray:NGDDebugActiveWindows()];
    for (NSUInteger i=0;i<queue.count && i<12000;i++) {
        UIView *v=queue[i];
        if ([v isKindOfClass:UIVisualEffectView.class]) [effects addObject:@{@"class":NGClassName(v),@"effect":[((UIVisualEffectView *)v).effect description] ?: @"none",@"frame":NSStringFromCGRect(v.frame),@"hidden":@(v.hidden),@"alpha":@(v.alpha),@"parent":NGClassName(v.superview)}];
        [queue addObjectsFromArray:v.subviews];
    }
    NSDictionary *report=@{@"product":NGPluginName,@"version":NGVersion,@"host":NSBundle.mainBundle.bundleIdentifier ?: @"",@"topController":NGClassName(NGTopController()),@"settings":preferences,@"runtimeGlass":NGRuntimeGlassStatus(),@"typography":@{@"configured":@(NGTypographyConfigured),@"generation":@(NGTypographyGeneration),@"scale":@(NGTypographyCachedScale),@"font":NGTypographyCachedName ?: @"system"},@"dock":@{@"found":@(dock!=nil),@"collapsed":@(dock.collapsed),@"side":@(dock.collapseSide),@"selectedIndex":@(dock.selectedIndex),@"tabCount":@(dock.viewControllers.count),@"collapsedFrame":NSStringFromCGRect(dock.collapseFrame),@"legacyMaterial":@(dock.legacyMaterial),@"selectedInk":NGDockColor(YES,dock.inkStyle).description,@"normalInk":NGDockColor(NO,dock.inkStyle).description,@"avatarImageFrame":NSStringFromCGRect(dock.searchIcon.frame),@"geometry":NGDAudioRuntimeGlassTarget()},@"effects":effects,@"hooks":@[NGDebugMethod(@"SSShortVideoRightInteractionLikeModel",@"setIsLiked:"),NGDebugMethod(@"SSShortVideoRightInteractionCollectModel",@"setIsCollected:"),NGDebugMethod(@"SSVideoSeriesFeedViewController",@"simpleSearchViewDidClicked"),NGDebugMethod(@"UILabel",@"setFont:")],@"scope":@"Current state and current IMP only; no historical hook chain or automatic deadlock diagnosis. Douyin-specific probes are replaced by Hongguo adapters."};
    NSData *json=[NSJSONSerialization dataWithJSONObject:NGSafeJSONObject(report,@"$",[NSMutableArray array]) options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
    return [@"===== NaiwaGuo / Hongguo runtime probe =====\n" stringByAppendingString:json ? [[NSString alloc] initWithData:json encoding:NSUTF8StringEncoding] : @"serialization failed"];
}
