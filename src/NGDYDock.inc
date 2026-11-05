// Adapted from HierarchThurs/DYKiller, commit bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
// Same native UITabBar platter geometry, glass effect and UIKit minimization.
// Hongguo adaptation: four native destinations and a separate native-search key.
static char NGDockPlatterOwnerKey,NGDockPlatterCollapsedKey,NGDockNativeOwnerKey;
static char NGDockOriginalGlassKey,NGDockAppliedGlassKey;
typedef struct { CGFloat buttonWidth,buttonPad,height,top,inset; } NGDockMetrics;
static const NGDockMetrics NGDockFallbackMetrics={94,4,62,0,21};
@class NGSystemDock;
static void NGDockInstallPlatterHooks(void);
@interface NGDockSurface : UIView
@property(nonatomic,weak) NGSystemDock *dock;
@end
@interface NGSystemDock : UIViewController <UITabBarDelegate,UITabBarControllerDelegate,UIGestureRecognizerDelegate>
@property(nonatomic,weak) UITabBarController *host;
@property(nonatomic,weak) UIView *originalBar;
@property(nonatomic) BOOL originalHidden,systemBarHidden,layingOut,collapsed,panFired;
@property(nonatomic,copy) NSArray<UIControl *> *buttons;
@property(nonatomic,copy) NSArray<NSNumber *> *sourceIndexes;
@property(nonatomic,strong) NSMutableDictionary<NSNumber *,UIViewController *> *pages;
@property(nonatomic,strong) UITabBar *tabBar;
@property(nonatomic) BOOL legacyMaterial;
@property(nonatomic,strong) UIVisualEffectView *legacyPlatter;
@property(nonatomic,strong) NSMutableArray<UIButton *> *legacyButtons;
@property(nonatomic,strong) UITabBarController *systemHost;
@property(nonatomic,copy) NSArray<UIViewController *> *viewControllers;
@property(nonatomic) NSUInteger selectedIndex;
@property(nonatomic) NSInteger collapseSide;
@property(nonatomic,strong) UIVisualEffectView *searchKey;
@property(nonatomic,strong) UIButton *searchHit;
@property(nonatomic,strong) UISelectionFeedbackGenerator *tapFeedback;
@property(nonatomic,strong) UIImageView *searchIcon;
@property(nonatomic,copy) NSString *actionAppearanceKey;
@property(nonatomic,strong) UIPanGestureRecognizer *collapsePan;
@property(nonatomic,weak) UIView *platter;
@property(nonatomic,weak) UIView *collapsePlatter;
@property(nonatomic) CGRect collapseFrame;
@property(nonatomic) NGDockMetrics metrics;
@property(nonatomic) CGFloat metricsWidth;
@property(nonatomic) UIUserInterfaceStyle glassStyle,inkStyle;
@property(nonatomic) BOOL clearApplied;
@property(nonatomic,strong) UIColor *nativeBackground;
@property(nonatomic,strong) UIColor *nativeLayerBackground;
@property(nonatomic,copy) NSString *inkAppearanceKey;
@property(nonatomic,strong) NSMapTable<UIView *,NSArray *> *nativeChildren;
- (void)setViewControllers:(NSArray<UIViewController *> *)pages animated:(BOOL)animated;
- (void)refreshImages;
- (void)layoutGlass;
- (void)setCollapsed:(BOOL)collapsed;
- (void)nativeExpanded;
- (void)searchTapped;
@end
@interface NGSystemDock (Legacy)
- (void)layoutLegacyDock;
- (void)refreshLegacyButtons;
- (void)legacyPageTapped:(UIButton *)sender;
@end
static char NGDockInkOwnerKey;
@interface NGDockInkOwner : NSObject
@property(nonatomic,weak) NGSystemDock *dock;
@end
@implementation NGDockInkOwner @end
static void NGDockBindInkOwner(UITabBar *bar,NGSystemDock *dock) {
    NGDockInkOwner *owner=[NGDockInkOwner new]; owner.dock=dock;
    objc_setAssociatedObject(bar,&NGDockInkOwnerKey,owner,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void NGDockInstallInkHooks(void) {
    static dispatch_once_t once; dispatch_once(&once,^{
        for (NSString *name in @[@"setTintColor:",@"setUnselectedItemTintColor:"]) {
            SEL sel=NSSelectorFromString(name); Method method=class_getInstanceMethod(UITabBar.class,sel); IMP original=method_getImplementation(method); BOOL selected=[name isEqual:@"setTintColor:"];
            class_replaceMethod(UITabBar.class,sel,imp_implementationWithBlock(^(UITabBar *bar,UIColor *value){
                NGSystemDock *dock=((NGDockInkOwner *)objc_getAssociatedObject(bar,&NGDockInkOwnerKey)).dock;
                if (dock && NGFeatureArray(selected ? NGPreferenceDockSelectedColor : NGPreferenceDockNormalColor).count==4) value=NGDockColor(selected,dock.inkStyle);
                ((void(*)(id,SEL,id))original)(bar,sel,value);
            }),method_getTypeEncoding(method));
        }
    });
}
static UIView *NGDockFindPlatter(UIView *root,BOOL square) {
    Class cls=NSClassFromString(@"UIKit._UITabBarPlatterView"); if (!cls) return nil;
    NSMutableArray *nodes=[NSMutableArray arrayWithArray:root.subviews]; UIView *fallback=nil;
    for (NSUInteger i=0;i<nodes.count && i<180;i++) {
        UIView *node=nodes[i];
        if ([node isKindOfClass:cls]) { if (!fallback) fallback=node; if ((node.bounds.size.width==node.bounds.size.height)==square) return node; }
        else [nodes addObjectsFromArray:node.subviews];
    }
    return square ? nil : fallback;
}
static UIGlassEffect *NGDockGlassEffect(UIUserInterfaceStyle style,BOOL interactive) API_AVAILABLE(ios(26.0)) {
    BOOL clear=NGFeatureBool(NGPreferenceDockClear);
    UIGlassEffect *effect=[UIGlassEffect effectWithStyle:clear ? UIGlassEffectStyleClear : UIGlassEffectStyleRegular];
    if (clear && style==UIUserInterfaceStyleDark) effect.tintColor=[UIColor colorWithWhite:0 alpha:0.3];
    effect.interactive=interactive; return effect;
}
static void NGDockApplyPlatterGlass(UIView *platter,UIUserInterfaceStyle style) {
    if (!platter) return;
    if (@available(iOS 26.0,*)) {
        NSNumber *applied=objc_getAssociatedObject(platter,&NGDockAppliedGlassKey);
        BOOL clear=NGFeatureBool(NGPreferenceDockClear);
        if (!clear) {
            if (!applied) return; id original=objc_getAssociatedObject(platter,&NGDockOriginalGlassKey);
            @try { [platter setValue:original==NSNull.null ? nil : original forKey:@"glassEffect"]; } @catch (__unused NSException *exception) {}
            objc_setAssociatedObject(platter,&NGDockAppliedGlassKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC); objc_setAssociatedObject(platter,&NGDockOriginalGlassKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC); [platter setNeedsLayout]; return;
        }
        if (applied.integerValue==(NSInteger)style && applied) return;
        static BOOL probed,supported;
        if (probed && !supported) return; id current=nil;
        @try { current=[platter valueForKey:@"glassEffect"]; supported=YES; } @catch (__unused NSException *exception) { probed=YES; supported=NO; return; }
        probed=YES;
        if (!objc_getAssociatedObject(platter,&NGDockOriginalGlassKey)) objc_setAssociatedObject(platter,&NGDockOriginalGlassKey,current ?: NSNull.null,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        BOOL interactive=[current isKindOfClass:UIGlassEffect.class] ? ((UIGlassEffect *)current).interactive : YES;
        @try { [platter setValue:NGDockGlassEffect(style,interactive) forKey:@"glassEffect"]; } @catch (__unused NSException *exception) { supported=NO; return; }
        objc_setAssociatedObject(platter,&NGDockAppliedGlassKey,@(style),OBJC_ASSOCIATION_RETAIN_NONATOMIC); [platter setNeedsLayout];
    }
}
static CGFloat NGDockCapsuleWidth(NGDockMetrics m,NSUInteger count,NSUInteger reference,CGFloat width,CGFloat length) {
    CGFloat single=m.buttonWidth+2*m.buttonPad,naturalPitch=MAX(m.buttonWidth-2*m.buttonPad,0); reference=MAX(reference,count);
    if (count<=1 || reference<=1) return single;
    CGFloat available=width-m.height-12-2*m.inset;
    CGFloat referenceWidth=MIN(available,single+(reference-1)*naturalPitch)*length;
    CGFloat pitch=MIN(MAX((referenceWidth-single)/(reference-1),0),naturalPitch);
    return single+(count-1)*pitch;
}
static BOOL NGDockMeasure(NGSystemDock *dock,UIView *platter) {
    if (!platter || platter.bounds.size.height<=0) return NO;
    CGRect frame=[platter convertRect:platter.bounds toView:dock.tabBar]; NGDockMetrics m=dock.metrics;
    m.height=frame.size.height; m.top=frame.origin.y;
    NSMutableArray *nodes=[NSMutableArray arrayWithArray:platter.subviews]; CGFloat first=CGFLOAT_MAX,second=CGFLOAT_MAX,width=0;
    for (NSUInteger i=0;i<nodes.count && i<200;i++) {
        UIView *node=nodes[i];
        if ([NGClassName(node) isEqualToString:@"_UITabButton"]) {
            CGRect r=[node convertRect:node.bounds toView:platter]; CGFloat x=r.origin.x;
            if (x<first-0.5) { if (first<CGFLOAT_MAX) second=first; first=x; width=r.size.width; }
            else if (x>first+0.5 && x<second-0.5) second=x;
        } else [nodes addObjectsFromArray:node.subviews];
    }
    if (first<CGFLOAT_MAX && width>0 && NGDockSize(NGPreferenceDockLength)>=1) { m.buttonPad=first; m.buttonWidth=width; }
    if (dock.viewControllers.count>=2 && second<CGFLOAT_MAX && second-first<m.buttonWidth-2*m.buttonPad-0.5) m.inset=(dock.tabBar.bounds.size.width-frame.size.width)/2;
    BOOL changed=fabs(m.buttonWidth-dock.metrics.buttonWidth)>0.5 || fabs(m.buttonPad-dock.metrics.buttonPad)>0.5 || fabs(m.height-dock.metrics.height)>0.5 || fabs(m.inset-dock.metrics.inset)>0.5;
    dock.metrics=m; return changed;
}
static BOOL NGDockInvokeNativeSearch(UITabBarController *host);
static BOOL NGDockInvokeAction(NGSystemDock *dock);
@implementation NGDockSurface
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    if (self.hidden || self.alpha<0.01 || !self.userInteractionEnabled) return nil;
    NGSystemDock *dock=self.dock;
    NSArray *targets=dock.collapsed ? (dock.collapsePlatter ? @[dock.collapsePlatter] : @[]) : @[(UIView *)dock.searchKey ?: self,(UIView *)dock.platter ?: dock.tabBar];
    for (UIView *target in targets) { if (target==self || target.hidden || target.alpha<0.01) continue; UIView *hit=[target hitTest:[self convertPoint:point toView:target] withEvent:event]; if (hit) return hit; }
    return nil;
}
@end
@implementation NGSystemDock
- (void)loadView { NGDockSurface *view=[NGDockSurface new]; view.dock=self; view.backgroundColor=UIColor.clearColor; view.opaque=NO; view.clipsToBounds=NO; self.view=view; }
- (void)viewDidLoad {
    [super viewDidLoad]; self.view.accessibilityIdentifier=@"NaiwaGuo.SystemDock"; self.pages=[NSMutableDictionary dictionary]; self.nativeChildren=[NSMapTable weakToStrongObjectsMapTable]; self.metrics=NGDockFallbackMetrics; self.collapseSide=-1;
    self.legacyMaterial=!NGDockNativeLiquidAvailable();
    [self rebuildBar]; NGDockInstallPlatterHooks();
    if (!self.legacyMaterial) if (@available(iOS 26.0,*)) {
        self.searchKey=[[UIVisualEffectView alloc] initWithEffect:NGDockGlassEffect(self.traitCollection.userInterfaceStyle,YES)]; self.searchKey.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
    }
    if (!self.searchKey) { self.searchKey=[[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterial]]; self.searchKey.clipsToBounds=YES; }
    self.searchKey.accessibilityIdentifier=@"NaiwaGuo.Dock.SearchGlass"; [self.view addSubview:self.searchKey];
    self.searchIcon=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"magnifyingglass" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:28 weight:UIImageSymbolWeightMedium]]]; self.searchIcon.contentMode=UIViewContentModeScaleAspectFit; [self.searchKey.contentView addSubview:self.searchIcon];
    self.searchHit=[UIButton buttonWithType:UIButtonTypeCustom]; self.searchHit.accessibilityLabel=@"搜索短剧"; self.searchHit.accessibilityIdentifier=@"NaiwaGuo.Dock.Search"; [self.searchHit addTarget:self action:@selector(searchTapped) forControlEvents:UIControlEventTouchUpInside]; [self.searchKey.contentView addSubview:self.searchHit];
    if (@available(iOS 17.0,*)) [self registerForTraitChanges:@[UITraitUserInterfaceStyle.class] withHandler:^(__kindof NGSystemDock *dock,__unused UITraitCollection *previous){ [dock layoutGlass]; }];
}
- (void)rebuildBar {
    NSInteger side=NGDockCollapseSide(); if (self.tabBar && side==self.collapseSide) return;
    for (UIView *old in @[self.platter ?: [UIView new],self.collapsePlatter ?: [UIView new]]) objc_setAssociatedObject(old,&NGDockPlatterOwnerKey,nil,OBJC_ASSOCIATION_ASSIGN);
    [self.collapsePan.view removeGestureRecognizer:self.collapsePan]; [self.systemHost.view removeFromSuperview]; [self.systemHost willMoveToParentViewController:nil]; [self.systemHost removeFromParentViewController]; [self.tabBar removeFromSuperview]; self.systemHost=nil; self.platter=nil; self.collapsePlatter=nil; self.collapseFrame=CGRectZero; self.collapsed=NO; self.collapseSide=side; self.metrics=NGDockFallbackMetrics; self.metricsWidth=0;
    if (self.legacyMaterial) {
        self.tabBar=[UITabBar new]; self.tabBar.delegate=self;
        self.legacyPlatter=[[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterial]];
        self.legacyPlatter.accessibilityIdentifier=@"NaiwaGuo.Dock.LegacyBlur"; self.legacyPlatter.clipsToBounds=YES;
        [self.view addSubview:self.legacyPlatter]; self.platter=self.legacyPlatter; self.legacyButtons=[NSMutableArray array];
    } else if (side) {
        self.systemHost=[UITabBarController new]; self.systemHost.delegate=self;
        if (@available(iOS 26.0,*)) self.systemHost.tabBarMinimizeBehavior=UITabBarMinimizeBehaviorOnScrollDown;
        [self addChildViewController:self.systemHost]; self.systemHost.view.backgroundColor=UIColor.clearColor; [self.view addSubview:self.systemHost.view]; [self.systemHost didMoveToParentViewController:self]; self.tabBar=self.systemHost.tabBar;
    } else { self.tabBar=[UITabBar new]; self.tabBar.delegate=self; [self.view addSubview:self.tabBar]; }
    NGDockBindInkOwner(self.tabBar,self);
    if (self.viewControllers) [self setViewControllers:self.viewControllers animated:NO];
    if (self.searchKey) [self.view bringSubviewToFront:self.searchKey];
    if (side && self.host) {
        if (!self.collapsePan) { self.collapsePan=[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)]; self.collapsePan.delegate=self; self.collapsePan.maximumNumberOfTouches=1; self.collapsePan.cancelsTouchesInView=NO; self.collapsePan.delaysTouchesBegan=NO; self.collapsePan.delaysTouchesEnded=NO; }
        [self.host.view addGestureRecognizer:self.collapsePan];
    }
}
- (void)setViewControllers:(NSArray<UIViewController *> *)pages { [self setViewControllers:pages animated:NO]; }
- (void)setViewControllers:(NSArray<UIViewController *> *)pages animated:(BOOL)animated {
    _viewControllers=[pages copy];
    if (self.systemHost) { [self.systemHost setViewControllers:pages animated:animated]; for (UIViewController *page in pages) { page.view.backgroundColor=UIColor.clearColor; page.view.userInteractionEnabled=NO; } }
    else [self.tabBar setItems:[pages valueForKey:@"tabBarItem"] animated:animated];
    if (pages.count) [self setSelectedIndex:MIN(self.selectedIndex,pages.count-1)];
}
- (void)setSelectedIndex:(NSUInteger)index {
    if (index>=self.viewControllers.count) return; _selectedIndex=index;
    if (self.systemHost) { if (self.systemHost.selectedIndex!=index) self.systemHost.selectedIndex=index; }
    else if (self.tabBar.selectedItem!=self.viewControllers[index].tabBarItem) self.tabBar.selectedItem=self.viewControllers[index].tabBarItem;
    if (self.legacyMaterial) [self refreshLegacyButtons];
}
- (void)playTapFeedback {
    if (!self.host || !self.view.window || self.view.hidden || self.view.alpha<0.01 ||
        !NGFeatureBool(NGPreferenceDock) ||
        !NGFeatureBool(NGPreferenceDockHaptics)) return;
    if (!self.tapFeedback) self.tapFeedback=[UISelectionFeedbackGenerator new];
    [self.tapFeedback selectionChanged];
    [self.tapFeedback prepare];
}
- (void)selectPage:(NSUInteger)index {
    if (index>=self.viewControllers.count) return;
    if (index<self.buttons.count) {
        if (!self.buttons[index].enabled) return;
        [self playTapFeedback];
        [self.buttons[index] sendActionsForControlEvents:UIControlEventTouchUpInside];
    }
    self.selectedIndex=index;
}
- (void)tabBar:(UITabBar *)bar didSelectItem:(UITabBarItem *)item { NSUInteger index=[bar.items indexOfObject:item]; if (index!=NSNotFound) [self selectPage:index]; }
- (BOOL)tabBarController:(UITabBarController *)controller shouldSelectViewController:(UIViewController *)page { NSUInteger index=[self.viewControllers indexOfObject:page]; if (index==NSNotFound) return NO; [self selectPage:index]; return YES; }
- (void)refreshImages {
    if (self.inkStyle==UIUserInterfaceStyleUnspecified) self.inkStyle=self.traitCollection.userInterfaceStyle;
    UIColor *selectedInk=NGDockColor(YES,self.inkStyle),*normalInk=NGDockColor(NO,self.inkStyle);
    NSString *inkKey=[NSString stringWithFormat:@"%@|%@|%p",selectedInk,normalInk,self.tabBar];
    if (![self.inkAppearanceKey isEqual:inkKey]) {
        self.inkAppearanceKey=inkKey;
        UITabBarAppearance *appearance=[self.tabBar.standardAppearance copy];
        for (UITabBarItemAppearance *items in @[appearance.stackedLayoutAppearance,appearance.inlineLayoutAppearance,appearance.compactInlineLayoutAppearance]) {
            items.normal.iconColor=normalInk; items.selected.iconColor=selectedInk;
            NSMutableDictionary *normal=[items.normal.titleTextAttributes mutableCopy] ?: [NSMutableDictionary dictionary]; normal[NSForegroundColorAttributeName]=normalInk; items.normal.titleTextAttributes=normal;
            NSMutableDictionary *selected=[items.selected.titleTextAttributes mutableCopy] ?: [NSMutableDictionary dictionary]; selected[NSForegroundColorAttributeName]=selectedInk; items.selected.titleTextAttributes=selected;
        }
        self.tabBar.standardAppearance=appearance; self.tabBar.scrollEdgeAppearance=appearance;
    }
    self.tabBar.tintColor = selectedInk;
    self.tabBar.unselectedItemTintColor = NGDockColor(NO,self.inkStyle);
    self.searchIcon.tintColor = NGDockColor(YES,self.inkStyle);
    NSUInteger actionIndex=[NGDockActions() indexOfObject:NGDockAction()]; UIImage *avatar=NGDockCurrentAvatar();
    NSString *appearance=[NSString stringWithFormat:@"%@|%d|%p",NGDockAction(),NGDockAvatarEnabled(),avatar];
    if (![self.actionAppearanceKey isEqual:appearance]) {
        self.actionAppearanceKey=appearance;
        self.searchIcon.image=avatar ?: ([UIImage systemImageNamed:NGDockAvatarEnabled() ? @"person.crop.circle" : NGDockActionSymbols()[actionIndex] withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:28 weight:UIImageSymbolWeightMedium]] ?: [UIImage systemImageNamed:@"cart"]);
        self.searchHit.accessibilityLabel=NGDockActionTitles()[actionIndex];
    }
    for (NSUInteger index=0;index<self.viewControllers.count;index++) {
        NSUInteger source=index<self.sourceIndexes.count ? self.sourceIndexes[index].unsignedIntegerValue : index; UITabBarItem *item=self.viewControllers[index].tabBarItem;
        if (self.collapsed && index==self.selectedIndex) {
            BOOL accountAvatar=self.collapseSide==3 && NGDockAvatarEnabled();
            UIImage *icon=accountAvatar ? (NGDockCurrentAvatar() ?: [UIImage systemImageNamed:@"person.crop.circle"]) : NGDockIcon(source,YES);
            CGFloat side=accountAvatar ? MAX(1,self.metrics.height-14) : (NGDockContentMode()==2 ? 24 : 26);
            // Apply symbol ink before rasterizing; uploaded photos retain their colors.
            if (icon.renderingMode!=UIImageRenderingModeAlwaysOriginal) icon=[icon imageWithTintColor:NGDockColor(YES,self.inkStyle) renderingMode:UIImageRenderingModeAlwaysOriginal];
            UIImage *image=[NGIconImageOnCanvas(icon,side,side) imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
            item.image=image; item.selectedImage=image;
        }
        else NGDockApplyRenderedImages(item,source,self.inkStyle);
    }
    if (self.legacyMaterial) [self refreshLegacyButtons];
}
- (void)viewDidLayoutSubviews { [super viewDidLayoutSubviews]; [self layoutGlass]; }
- (void)layoutGlass {
    if (self.layingOut || self.view.bounds.size.width<1) return; self.layingOut=YES;
    @try {
        [self rebuildBar];
        if (self.legacyMaterial) { [self layoutLegacyDock]; return; }
        CGSize size=self.view.bounds.size;
        UIUserInterfaceStyle style=self.view.window.windowScene.traitCollection.userInterfaceStyle; if (style==UIUserInterfaceStyleUnspecified) style=self.traitCollection.userInterfaceStyle;
        BOOL clear=NGFeatureBool(NGPreferenceDockClear);
        if (self.glassStyle!=style || self.clearApplied!=clear) { self.glassStyle=style; self.clearApplied=clear; self.tabBar.overrideUserInterfaceStyle=style; self.searchKey.overrideUserInterfaceStyle=style; if (@available(iOS 26.0,*)) self.searchKey.effect=NGDockGlassEffect(style,YES); }
        UIUserInterfaceStyle ink=style;
        if (self.selectedIndex<self.buttons.count) {
            NSMutableArray *nodes=[NSMutableArray arrayWithObject:self.buttons[self.selectedIndex]];
            for (NSUInteger i=0;i<nodes.count && i<30;i++) { UIView *node=nodes[i]; if ([node isKindOfClass:UILabel.class] && ((UILabel *)node).text.length) { UIColor *color=[((UILabel *)node).textColor resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:style]]; CGFloat r,g,b,a; if ([color getRed:&r green:&g blue:&b alpha:&a] && a>=0.5) ink=(0.2126*r+0.7152*g+0.0722*b)>0.5 ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight; break; } [nodes addObjectsFromArray:node.subviews]; }
        }
        if (self.inkStyle!=ink) { self.inkStyle=ink; [self refreshImages]; self.searchIcon.tintColor=NGDockColor(YES,ink); }
        if (fabs(size.width-self.metricsWidth)>0.5) { self.metrics=NGDockFallbackMetrics; self.metricsWidth=size.width; }
        UIView *barHost=self.systemHost ? self.systemHost.view : self.tabBar; CGFloat target=0;
        for (NSUInteger pass=0;pass<2;pass++) {
            target=NGDockCapsuleWidth(self.metrics,self.viewControllers.count,4,size.width,NGDockSize(NGPreferenceDockLength));
            CGRect bounds=CGRectMake(0,0,target+2*self.metrics.inset,size.height);
            if (!CGRectEqualToRect(barHost.bounds,bounds)) barHost.bounds=bounds;
            [barHost layoutIfNeeded]; self.platter=NGDockFindPlatter(self.tabBar,NO); if (!NGDockMeasure(self,self.platter)) break;
        }
        self.collapsePlatter=self.systemHost ? NGDockFindPlatter(self.tabBar,YES) : nil;
        for (UIView *platter in @[self.platter ?: [UIView new],self.collapsePlatter ?: [UIView new]]) if (platter.superview) { objc_setAssociatedObject(platter,&NGDockPlatterOwnerKey,self,OBJC_ASSOCIATION_ASSIGN); objc_setAssociatedObject(platter,&NGDockPlatterCollapsedKey,@(platter==self.collapsePlatter),OBJC_ASSOCIATION_RETAIN_NONATOMIC); NGDockApplyPlatterGlass(platter,style); }
        CGRect platter=self.platter ? [self.platter convertRect:self.platter.bounds toView:self.tabBar] : CGRectZero; BOOL measured=platter.size.width>0;
        CGFloat capsule=measured ? platter.size.width : target,capsuleX=measured ? platter.origin.x : self.metrics.inset;
        CGFloat scale=NGDockSize(NGPreferenceDockHeight),bottom=self.metrics.top+self.metrics.height,diameter=self.metrics.height*NGDockSize(NGPreferenceDockSearchSize),group=capsule*scale+12+diameter,left=(size.width-group)/2;
        CGPoint center=CGPointMake(left-scale*(capsuleX-CGRectGetMidX(barHost.bounds)),bottom-scale*(bottom-CGRectGetMidY(barHost.bounds)));
        if (!CGPointEqualToPoint(barHost.center,center)) barHost.center=center;
        CGAffineTransform transform=CGAffineTransformMakeScale(scale,scale); if (!CGAffineTransformEqualToTransform(barHost.transform,transform)) barHost.transform=transform;
        if (self.collapsePlatter) {
            CGFloat side=self.metrics.height,x=self.collapseSide==3 ? (size.width-side*scale)/2 : (self.collapseSide==2 ? size.width-self.metrics.inset-side*scale : self.metrics.inset);
            self.collapseFrame=CGRectMake(capsuleX+(x-left)/scale,self.metrics.top,side,side);
            if (!CGRectEqualToRect(self.collapsePlatter.frame,self.collapseFrame)) self.collapsePlatter.frame=self.collapseFrame;
        }
        CGRect keyFrame=CGRectMake(left+capsule*scale+12,bottom-diameter,diameter,diameter); if (!CGRectEqualToRect(self.searchKey.frame,keyFrame)) self.searchKey.frame=keyFrame;
        CGFloat iconInset=NGDockAvatarEnabled() ? 7 : 14; self.searchIcon.frame=CGRectInset(self.searchKey.bounds,iconInset*NGDockSize(NGPreferenceDockSearchSize),iconInset*NGDockSize(NGPreferenceDockSearchSize)); self.searchIcon.layer.cornerRadius=NGDockAvatarEnabled() ? self.searchIcon.bounds.size.width/2 : 0; self.searchIcon.clipsToBounds=NGDockAvatarEnabled(); self.searchHit.frame=self.searchKey.bounds;
        if (self.searchKey.alpha!=(self.collapsed ? 0 : 1)) self.searchKey.alpha=self.collapsed ? 0 : 1;
    } @finally { self.layingOut=NO; }
}
- (void)setCollapsed:(BOOL)collapsed {
    if (collapsed==_collapsed || !self.collapsePlatter || !self.collapseSide) return;
    Ivar ivar=class_getInstanceVariable(UITabBar.class,"_visualProvider"); id provider=ivar ? object_getIvar(self.tabBar,ivar) : nil; SEL selector=NSSelectorFromString(@"setMinimized:"); if (![provider respondsToSelector:selector]) return;
    _collapsed=collapsed; [self refreshImages]; ((void(*)(id,SEL,BOOL))[provider methodForSelector:selector])(provider,selector,collapsed);
    [UIView animateWithDuration:UIAccessibilityIsReduceMotionEnabled()?0:0.4 delay:0 usingSpringWithDamping:0.86 initialSpringVelocity:0 options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{ self.searchKey.alpha=collapsed ? 0 : 1; } completion:nil];
}
- (void)nativeExpanded { if (!_collapsed) return; _collapsed=NO; [self refreshImages]; [UIView animateWithDuration:UIAccessibilityIsReduceMotionEnabled()?0:0.4 animations:^{ self.searchKey.alpha=1; }]; }
- (void)handlePan:(UIPanGestureRecognizer *)pan {
    if (pan.state==UIGestureRecognizerStateBegan) self.panFired=NO;
    BOOL ended=pan.state==UIGestureRecognizerStateEnded; if ((pan.state!=UIGestureRecognizerStateChanged && !ended) || self.panFired) return;
    CGPoint translation=[pan translationInView:pan.view]; if (fabs(translation.y)<=fabs(translation.x)) return;
    if (ended) translation.y += [pan velocityInView:pan.view].y*0.1;
    if (translation.y<=-48) { self.panFired=YES; [self setCollapsed:YES]; } else if (translation.y>=12) { self.panFired=YES; [self setCollapsed:NO]; }
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)recognizer shouldReceiveTouch:(UITouch *)touch {
    if (!self.originalBar.window || self.view.hidden || self.originalBar.hidden || self.originalBar.alpha<0.01 || [touch.view isDescendantOfView:self.originalBar]) return NO;
    for (UIView *view=touch.view;view;view=view.superview) if ([view isKindOfClass:UIScrollView.class]) { UIScrollView *scroll=(id)view; if (scroll.scrollEnabled && (scroll.alwaysBounceVertical || scroll.contentSize.height>scroll.bounds.size.height)) return YES; }
    return NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)recognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other { return YES; }
- (void)searchTapped { if (self.searchHit.enabled && self.host && NGDockInvokeAction(self)) [self playTapFeedback]; }
- (void)dealloc { [self.collapsePan.view removeGestureRecognizer:self.collapsePan]; for (UIView *p in @[self.platter ?: [UIView new],self.collapsePlatter ?: [UIView new]]) objc_setAssociatedObject(p,&NGDockPlatterOwnerKey,nil,OBJC_ASSOCIATION_ASSIGN); }
@end
static void NGDockInstallPlatterHooks(void) {
    NGDockInstallInkHooks();
    static BOOL installed; if (installed) return; Class cls=NSClassFromString(@"UIKit._UITabBarPlatterView"); if (!cls) return; installed=YES;
    for (NSString *name in @[@"setFrame:",@"setBounds:"]) {
        SEL sel=NSSelectorFromString(name); Method method=class_getInstanceMethod(cls,sel); IMP original=method_getImplementation(method);
        class_replaceMethod(cls,sel,imp_implementationWithBlock(^(UIView *view,CGRect rect){ NGSystemDock *dock=objc_getAssociatedObject(view,&NGDockPlatterOwnerKey); if (dock && [objc_getAssociatedObject(view,&NGDockPlatterCollapsedKey) boolValue] && dock.collapseFrame.size.width>0) { if ([name isEqualToString:@"setBounds:"]) rect.size=dock.collapseFrame.size; else rect=dock.collapseFrame; } ((void(*)(id,SEL,CGRect))original)(view,sel,rect); }),method_getTypeEncoding(method));
    }
    SEL center=@selector(setCenter:); Method m=class_getInstanceMethod(cls,center); IMP prior=method_getImplementation(m);
    class_replaceMethod(cls,center,imp_implementationWithBlock(^(UIView *view,CGPoint point){ NGSystemDock *dock=objc_getAssociatedObject(view,&NGDockPlatterOwnerKey); if (dock && [objc_getAssociatedObject(view,&NGDockPlatterCollapsedKey) boolValue] && dock.collapseFrame.size.width>0) point=CGPointMake(CGRectGetMidX(dock.collapseFrame),CGRectGetMidY(dock.collapseFrame)); ((void(*)(id,SEL,CGPoint))prior)(view,center,point); }),method_getTypeEncoding(m));
    SEL hidden=@selector(setHidden:); m=class_getInstanceMethod(cls,hidden); IMP hiddenPrior=method_getImplementation(m);
    class_replaceMethod(cls,hidden,imp_implementationWithBlock(^(UIView *view,BOOL value){ ((void(*)(id,SEL,BOOL))hiddenPrior)(view,hidden,value); NGSystemDock *dock=objc_getAssociatedObject(view,&NGDockPlatterOwnerKey); if (dock && !value && view==dock.platter) [dock nativeExpanded]; }),method_getTypeEncoding(m));
}

#include "NGDockLegacy.inc"
