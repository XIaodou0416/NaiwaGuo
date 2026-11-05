// iOS 15-25 uses only public blur and button APIs; no native floating provider.
@implementation NGSystemDock (Legacy)
- (void)legacyPageTapped:(UIButton *)sender { [self selectPage:sender.tag]; }
- (void)refreshLegacyButtons {
    if (!self.legacyMaterial) return;
    while (self.legacyButtons.count>self.viewControllers.count) { [self.legacyButtons.lastObject removeFromSuperview]; [self.legacyButtons removeLastObject]; }
    while (self.legacyButtons.count<self.viewControllers.count) {
        UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom]; button.tag=self.legacyButtons.count;
        button.imageView.contentMode=UIViewContentModeScaleAspectFit; [button addTarget:self action:@selector(legacyPageTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self.legacyPlatter.contentView addSubview:button]; [self.legacyButtons addObject:button];
    }
    for (NSUInteger i=0;i<self.legacyButtons.count;i++) {
        UIButton *button=self.legacyButtons[i]; UITabBarItem *item=self.viewControllers[i].tabBarItem;
        [button setImage:item.image forState:UIControlStateNormal]; [button setImage:item.selectedImage forState:UIControlStateSelected];
        button.selected=i==self.selectedIndex; button.enabled=i>=self.buttons.count || self.buttons[i].enabled; button.accessibilityLabel=item.accessibilityLabel;
        button.backgroundColor=button.selected ? [UIColor.labelColor colorWithAlphaComponent:0.10] : UIColor.clearColor;
    }
}
- (void)layoutLegacyDock {
    CGFloat width=self.view.bounds.size.width,scale=NGDockSize(NGPreferenceDockHeight),searchScale=NGDockSize(NGPreferenceDockSearchSize);
    CGFloat capsule=NGDockCapsuleWidth(NGDockFallbackMetrics,self.viewControllers.count,4,width,NGDockSize(NGPreferenceDockLength))*scale;
    CGFloat height=NGDockFallbackMetrics.height*scale,diameter=NGDockFallbackMetrics.height*searchScale,left=(width-capsule-12-diameter)/2,bottom=NGDockFallbackMetrics.height;
    self.legacyPlatter.frame=CGRectMake(left,bottom-height,capsule,height); self.legacyPlatter.layer.cornerRadius=height/2;
    UIUserInterfaceStyle style=self.traitCollection.userInterfaceStyle; BOOL clear=[NSUserDefaults.standardUserDefaults boolForKey:NGPreferenceDockClear];
    if (self.glassStyle!=style || self.clearApplied!=clear) {
        self.glassStyle=style; self.clearApplied=clear;
        UIBlurEffect *effect=[UIBlurEffect effectWithStyle:clear ? UIBlurEffectStyleSystemUltraThinMaterial : UIBlurEffectStyleSystemThinMaterial];
        self.legacyPlatter.effect=effect; self.searchKey.effect=effect;
    }
    if (self.inkStyle!=style) { self.inkStyle=style; [self refreshImages]; }
    [self refreshLegacyButtons];
    CGFloat inset=4*scale,buttonWidth=(capsule-2*inset)/MAX(1,self.legacyButtons.count);
    for (NSUInteger i=0;i<self.legacyButtons.count;i++) {
        UIButton *button=self.legacyButtons[i]; button.frame=CGRectMake(inset+i*buttonWidth,inset,buttonWidth,height-2*inset); button.layer.cornerRadius=(height-2*inset)/2;
        button.imageView.transform=CGAffineTransformMakeScale(scale,scale);
    }
    self.searchKey.frame=CGRectMake(left+capsule+12,bottom-diameter,diameter,diameter); self.searchKey.layer.cornerRadius=diameter/2;
    CGFloat insetIcon=(NGDockAvatarEnabled() ? 7 : 14)*searchScale; self.searchIcon.frame=CGRectInset(self.searchKey.bounds,insetIcon,insetIcon);
    self.searchIcon.layer.cornerRadius=NGDockAvatarEnabled() ? self.searchIcon.bounds.size.width/2 : 0; self.searchIcon.clipsToBounds=NGDockAvatarEnabled(); self.searchHit.frame=self.searchKey.bounds;
}
@end
