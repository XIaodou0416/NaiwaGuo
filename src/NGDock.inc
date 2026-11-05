#if NG_TEST
static BOOL NGDockForceLegacyTest;
#endif
static BOOL NGDockNativeLiquidAvailable(void) {
#if NG_TEST
    if (NGDockForceLegacyTest) return NO;
#endif
    if (@available(iOS 26.0,*)) return YES;
    return NO;
}
static NSString *const NGPreferenceDockAction=@"com.naiwaguo.appearance.dockAction";
static NSString *const NGPreferenceDockAvatar=@"com.naiwaguo.appearance.dockAvatar";
static NSString *const NGPreferenceDockHaptics=@"com.naiwaguo.appearance.dockHaptics";
static NSString *const NGPreferenceDockClear=@"com.naiwaguo.appearance.dockClear";
static NSString *const NGPreferenceDockCollapse=@"com.naiwaguo.appearance.dockCollapse";
static NSString *const NGPreferenceDockHeight=@"com.naiwaguo.appearance.dockHeight";
static NSString *const NGPreferenceDockLength=@"com.naiwaguo.appearance.dockLength";
static NSString *const NGPreferenceDockSearchSize=@"com.naiwaguo.appearance.dockSearchSize";
static NSString *const NGPreferenceDockContent=@"com.naiwaguo.appearance.dockContent";
static NSInteger NGDockContentMode(void) { NSInteger n=NGFeatureInteger(NGPreferenceDockContent); return n>=0 && n<=2 ? n : 0; }
static NSInteger NGDockCollapseSide(void) {
    if (!NGDockNativeLiquidAvailable()) return 0;
    if (@available(iOS 26.0,*)) { if (UIDevice.currentDevice.userInterfaceIdiom==UIUserInterfaceIdiomPhone) { NSInteger n=NGFeatureInteger(NGPreferenceDockCollapse); return n>=1 && n<=3 ? n : 0; } } return 0;
}
static CGFloat NGDockSize(NSString *key) { id n=NGFeatureObject(key); CGFloat value=n ? [n doubleValue] : 100; return isfinite(value) ? MIN(1,MAX([key isEqualToString:NGPreferenceDockLength]?0.8:0.6,value/100)) : 1; }
static NSString *const NGPreferenceDock = @"com.naiwaguo.appearance.dock";
static NSString *const NGPreferenceHiddenDockItems = @"com.naiwaguo.appearance.hiddenDockItems";
static NSString *const NGPreferenceDockIconStyle = @"com.naiwaguo.appearance.dockIconStyle";
static NSString *const NGPreferenceDockNormalColor = @"com.naiwaguo.appearance.dockColor.normal";
static NSString *const NGPreferenceDockSelectedColor = @"com.naiwaguo.appearance.dockColor.selected";
static UIColor *NGDockColor(BOOL selected, UIUserInterfaceStyle style) {
    NSArray *rgba = NGFeatureArray(selected ? NGPreferenceDockSelectedColor : NGPreferenceDockNormalColor);
    if (rgba.count == 4) {
        for (id value in rgba) if (![value isKindOfClass:NSNumber.class] || !isfinite([value doubleValue])) return selected ? UIColor.systemBlueColor : UIColor.labelColor;
        return [UIColor colorWithRed:MIN(1, MAX(0, [rgba[0] doubleValue])) green:MIN(1, MAX(0, [rgba[1] doubleValue])) blue:MIN(1, MAX(0, [rgba[2] doubleValue])) alpha:MIN(1, MAX(0, [rgba[3] doubleValue]))];
    }
    return [UIColor colorWithWhite:style==UIUserInterfaceStyleDark ? 1 : 0 alpha:selected ? 1 : 0.60];
}
#include "NGDockUnderlay.inc"

static char NGDockAssociation;
static char NGDockItemAssociation;
static char NGDockUpdatingAssociation;
static void NGUpdateDock(void);

static NSArray<NSString *> *NGDockItemKeys(void) { return @[@"home", @"theater", @"shop", @"mine"]; }
static NSArray<NSString *> *NGDockItemTitles(void) { return @[@"首页", @"剧场", @"商城", @"我的"]; }
static NSArray<NSString *> *NGDockIconStyles(void) { return @[@"original", @"minimal", @"vivid", @"capsule"]; }
static NSArray<NSString *> *NGDockIconStyleTitles(void) { return @[@"清简导航", @"柔角线条", @"光影剧场", @"几何卡片"]; }
static NSString *NGDockIconStyle(void) {
    NSString *style = NGFeatureString(NGPreferenceDockIconStyle);
    return [NGDockIconStyles() containsObject:style] ? style : @"original";
}
static NSString *NGDockIconImageKey(NSUInteger index) { return [NSString stringWithFormat:@"com.naiwaguo.appearance.dockIconImage.%lu", (unsigned long)index]; }
static UIImage *NGDockIcon(NSUInteger index, BOOL selected) {
    NSString *style = NGDockIconStyle();
    NSArray *normal = @[@"house", @"play.square", @"storefront", @"person"];
    NSArray *filled = @[@"house.fill", @"play.square.fill", @"storefront.fill", @"person.fill"];
    if ([style isEqualToString:@"minimal"]) {
        normal = @[@"house", @"rectangle.stack", @"basket", @"person.crop.square"];
        filled = @[@"house.fill", @"rectangle.stack.fill", @"basket.fill", @"person.crop.square.fill"];
    } else if ([style isEqualToString:@"vivid"]) {
        normal = @[@"square.grid.2x2", @"movieclapper", @"handbag", @"person.crop.rectangle"];
        filled = @[@"square.grid.2x2.fill", @"movieclapper.fill", @"handbag.fill", @"person.crop.rectangle.fill"];
    } else if ([style isEqualToString:@"capsule"]) {
        normal = @[@"square.on.square", @"film", @"shippingbox", @"person.crop.square.filled.and.at.rectangle"];
        filled = @[@"square.on.square.fill", @"film.fill", @"shippingbox.fill", @"person.crop.square.filled.and.at.rectangle.fill"];
    }
    NSString *symbol = (selected ? filled : normal)[MIN(index, 3)];
    NSData *customData = NGFeatureData(NGDockIconImageKey(index));
    UIImage *customImage = customData.length ? [UIImage imageWithData:customData] : nil;
    if (customImage) return [NGIconImageOnCanvas(customImage, 28, 26) imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    NSString *custom = NGFeatureString([NSString stringWithFormat:@"com.naiwaguo.appearance.dockIcon.%lu", (unsigned long)index]);
    if (custom.length) symbol = custom;
    UIImageSymbolWeight weight = [style isEqualToString:@"minimal"] ? UIImageSymbolWeightRegular : UIImageSymbolWeightMedium;
    return [UIImage systemImageNamed:symbol withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:21 weight:weight]] ?: [UIImage systemImageNamed:@[@"house",@"play.rectangle",@"cart",@"person"][MIN(index,3)]];
}
static char NGDockRenderedItemAssociation;
@interface NGDockRenderedItemState : NSObject
@property(nonatomic, copy) NSString *key;
@property(nonatomic, strong) UIImage *normalImage;
@property(nonatomic, strong) UIImage *selectedImage;
@end
@implementation NGDockRenderedItemState @end
// DYKiller MIT title/caption rendering; see third_party/DYKiller-LICENSE.
static UIImage *NGDockTitleImage(NSString *title, UIColor *color) {
    UIFont *font = [UIFont systemFontOfSize:15.0 weight:UIFontWeightSemibold];
    CGFloat width = [title sizeWithAttributes:@{ NSFontAttributeName: font }].width;
    if (width > 64.0) {
        CGFloat fontSize = MAX(11.0, floor(15.0 * 64.0 / width));
        font = [UIFont systemFontOfSize:fontSize weight:UIFontWeightSemibold];
    }
    NSDictionary *attributes = @{
        NSFontAttributeName: font,
        NSForegroundColorAttributeName: color
    };
    CGSize size = [title sizeWithAttributes:attributes];
    size.width = ceil(size.width);
    size.height = ceil(size.height);

    UIGraphicsImageRendererFormat *format = UIGraphicsImageRendererFormat.preferredFormat;
    format.opaque = NO;
    UIImage *image = [[[UIGraphicsImageRenderer alloc] initWithSize:size format:format]
        imageWithActions:^(__unused UIGraphicsImageRendererContext *context) {
            [title drawAtPoint:CGPointZero withAttributes:attributes];
        }];
    return [image imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}

static UIImage *NGDockIconCaptionImage(UIImage *icon, NSString *title, UIColor *color) {
    icon=NGIconImageOnCanvas(icon,24,24);
    NSMutableParagraphStyle *paragraph = [[NSMutableParagraphStyle alloc] init];
    paragraph.alignment = NSTextAlignmentCenter;
    paragraph.lineBreakMode = NSLineBreakByTruncatingTail;
    NSDictionary *attributes = @{
        NSFontAttributeName: [UIFont systemFontOfSize:10.0 weight:UIFontWeightSemibold],
        NSForegroundColorAttributeName: color,
        NSParagraphStyleAttributeName: paragraph
    };
    CGSize textSize = [title sizeWithAttributes:attributes];
    CGFloat side = 24.0;
    CGFloat width = ceil(MAX(side, MIN(textSize.width, 64.0)));
    CGFloat textHeight = ceil(textSize.height);
    CGSize size = CGSizeMake(width, side + 2.0 + textHeight);

    UIGraphicsImageRendererFormat *format = UIGraphicsImageRendererFormat.preferredFormat;
    format.opaque = NO;
    UIImage *image = [[[UIGraphicsImageRenderer alloc] initWithSize:size format:format]
        imageWithActions:^(__unused UIGraphicsImageRendererContext *context) {
            [icon drawInRect:CGRectMake((width - side) / 2.0, 0, side, side)];
            [title drawWithRect:CGRectMake(0, side + 2.0, width, textHeight)
                        options:NSStringDrawingUsesLineFragmentOrigin attributes:attributes context:nil];
        }];
    return [image imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}

static void NGDockApplyRenderedImages(UITabBarItem *item, NSUInteger index, UIUserInterfaceStyle style) {
    NSString *title = item.accessibilityLabel ?: item.title;
    if (!title.length) title = NGDockItemTitles()[MIN(index, NGDockItemTitles().count - 1)];
    NSData *customData = NGFeatureData(NGDockIconImageKey(index));
    NSString *customSymbol = NGFeatureString([NSString stringWithFormat:@"com.naiwaguo.appearance.dockIcon.%lu", (unsigned long)index]) ?: @"";
    NSString *key = [NSString stringWithFormat:@"%@|%@|%@|%@|%lu|%ld|%@|%@", title, NGDockIconStyle(), customSymbol, @(customData.hash), (unsigned long)style, (long)NGDockContentMode(), NGFeatureArray(NGPreferenceDockNormalColor), NGFeatureArray(NGPreferenceDockSelectedColor)];
    NGDockRenderedItemState *state = objc_getAssociatedObject(item, &NGDockRenderedItemAssociation);
    if (state && [state.key isEqualToString:key]) {
        if (item.image != state.normalImage) item.image = state.normalImage;
        if (item.selectedImage != state.selectedImage) item.selectedImage = state.selectedImage;
        if (item.title) item.title = nil;
        return;
    }
    UIColor *normal = NGDockColor(NO, style);
    UIColor *selected = NGDockColor(YES, style);
    UIImage *normalIcon = NGDockIcon(index, NO), *selectedIcon = NGDockIcon(index, YES);
    state = [NGDockRenderedItemState new]; state.key = key;
    if (normalIcon.renderingMode!=UIImageRenderingModeAlwaysOriginal) normalIcon=[normalIcon imageWithTintColor:normal renderingMode:UIImageRenderingModeAlwaysOriginal];
    if (selectedIcon.renderingMode!=UIImageRenderingModeAlwaysOriginal) selectedIcon=[selectedIcon imageWithTintColor:selected renderingMode:UIImageRenderingModeAlwaysOriginal];
    NSInteger mode=NGDockContentMode();
    state.normalImage = mode==0 ? NGDockTitleImage(title,normal) : (mode==1 ? NGIconImageOnCanvas(normalIcon,26,26) : NGDockIconCaptionImage(normalIcon,title,normal));
    state.selectedImage = mode==0 ? NGDockTitleImage(title,selected) : (mode==1 ? NGIconImageOnCanvas(selectedIcon,26,26) : NGDockIconCaptionImage(selectedIcon,title,selected));
    objc_setAssociatedObject(item, &NGDockRenderedItemAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    state.normalImage = [state.normalImage imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    state.selectedImage = [state.selectedImage imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    item.title = nil; item.image = state.normalImage; item.selectedImage = state.selectedImage;
}
static NSString *NGDockItemKey(NSUInteger index) {
    return index < NGDockItemKeys().count ? NGDockItemKeys()[index] : [NSString stringWithFormat:@"tab.%lu", (unsigned long)index];
}
static NSSet<NSString *> *NGHiddenDockItems(void) {
    id value = NGFeatureObject(NGPreferenceHiddenDockItems);
    return [value isKindOfClass:NSArray.class] ? [NSSet setWithArray:value] : [NSSet set];
}

@interface NGDockButtonState : NSObject
@property(nonatomic) CGRect originalFrame;
@property(nonatomic) CGRect appliedFrame;
@property(nonatomic) BOOL originalHidden;
@property(nonatomic) BOOL frameManaged;
@property(nonatomic) BOOL hiddenManaged;
@end
@implementation NGDockButtonState
@end

static NGDockButtonState *NGDockRememberButton(UIControl *button) {
    NGDockButtonState *state = objc_getAssociatedObject(button, &NGDockItemAssociation);
    if (!state) {
        state = [NGDockButtonState new];
        state.originalFrame = button.frame;
        state.originalHidden = button.hidden;
        objc_setAssociatedObject(button, &NGDockItemAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    } else if (!state.frameManaged || !CGRectEqualToRect(button.frame, state.appliedFrame)) {
        state.originalFrame = button.frame;
    }
    return state;
}
static void NGDockRestoreButton(UIControl *button) {
    NGDockButtonState *state = objc_getAssociatedObject(button, &NGDockItemAssociation);
    if (!state) return;
    if (state.frameManaged && CGRectEqualToRect(button.frame, state.appliedFrame)) button.frame = state.originalFrame;
    if (state.hiddenManaged) button.hidden = state.originalHidden;
    state.frameManaged = NO;
    state.hiddenManaged = NO;
}

#include "NGDockActions.inc"
#include "NGDYDock.inc"

static NSArray<UIControl *> *NGDockOriginalButtons(UIView *bar) {
    NSMutableArray<UIControl *> *buttons = [NSMutableArray array];
    NSMutableArray *queue = [NSMutableArray arrayWithObject:bar];
    for (NSUInteger index = 0; index < queue.count && index < 300; index++) {
        UIView *view = queue[index];
        if ([NGClassName(view) isEqualToString:@"SSTabBarTextButton"] && [view isKindOfClass:UIControl.class]) {
            NGDockRememberButton((UIControl *)view);
            [buttons addObject:(UIControl *)view];
        }
        [queue addObjectsFromArray:view.subviews];
    }
    [buttons sortUsingComparator:^NSComparisonResult(UIControl *a, UIControl *b) {
        NGDockButtonState *left = objc_getAssociatedObject(a, &NGDockItemAssociation);
        NGDockButtonState *right = objc_getAssociatedObject(b, &NGDockItemAssociation);
        CGFloat x = [a.superview convertRect:left.originalFrame toView:bar].origin.x;
        CGFloat y = [b.superview convertRect:right.originalFrame toView:bar].origin.x;
        return x < y ? NSOrderedAscending : (x > y ? NSOrderedDescending : NSOrderedSame);
    }];
    return buttons;
}

static BOOL NGDockControllerIsVisible(UIView *view, UIWindow *window) {
    if (!view || !window || view.window != window || CGRectIsEmpty(view.bounds)) return NO;
    CGRect visible = [view convertRect:view.bounds toView:window];
    visible = CGRectIntersection(visible, window.bounds);
    for (UIView *current = view; current && !CGRectIsEmpty(visible); current = current.superview) {
        if (current.hidden || current.alpha < 0.01) return NO;
        if (current.clipsToBounds && current != view) {
            visible = CGRectIntersection(visible, [current convertRect:current.bounds toView:window]);
        }
    }
    return !CGRectIsEmpty(visible);
}

static BOOL NGDockPlaybackPanelClass(NSString *name) {
    return [name containsString:@"ShortVideoCommentPanel"] || [name containsString:@"ShortVideoCommentList"] ||
           [name containsString:@"CommentContainer"] || [name containsString:@"VideoSharePanel"] ||
           [name containsString:@"UGSharePanel"] || [name containsString:@"UGKmpSharePanel"] ||
           [name containsString:@"ShortVideoShareViewController"] || [name isEqualToString:@"FQVShortVideoSettingsViewController"];
}

static BOOL NGDockHasPlaybackPanel(UITabBarController *host) {
    UIWindow *window = host.view.window;
    if (!window) return NO;
    NSMutableArray<UIViewController *> *queue = [NSMutableArray arrayWithObject:host];
    for (NSUInteger index = 0; index < queue.count && index < 500; index++) {
        UIViewController *controller = queue[index];
        if (NGDockPlaybackPanelClass(NGClassName(controller)) && NGDockControllerIsVisible(controller.viewIfLoaded, window)) return YES;
        if (controller.presentedViewController && !controller.presentedViewController.isBeingDismissed) [queue addObject:controller.presentedViewController];
        [queue addObjectsFromArray:controller.childViewControllers];
    }
    NSMutableArray<UIView *> *views = [NSMutableArray arrayWithObject:window];
    for (NSUInteger index = 0; index < views.count && index < 12000; index++) {
        UIView *view = views[index];
        NSString *name = NGClassName(view);
        if ([name containsString:@"ShortVideoCommentPanel"] || [name containsString:@"VideoSharePanel"] ||
            [name containsString:@"UGSharePanel"] || [name containsString:@"UGKmpSharePanel"] || [name containsString:@"ShortVideoMorePanelShareHeader"]) {
            if (NGDockControllerIsVisible(view, window)) return YES;
        }
        [views addObjectsFromArray:view.subviews];
    }
    return NO;
}

#include "NGDockNativeSurface.inc"
static void NGDockRestoreNativeContent(NGSystemDock *dock) {
    NGDockNativeSurfaceWriting=YES;
    for (UIView *view in dock.nativeChildren.keyEnumerator.allObjects) {
        NGDockNativeSurfaceState *state=objc_getAssociatedObject(view,&NGDockNativeSurfaceKey);
        NSArray *saved=[dock.nativeChildren objectForKey:view]; view.layer.opacity=state ? state.opacity : [saved[0] floatValue]; view.userInteractionEnabled=state ? state.interaction : [saved[1] boolValue];
        objc_setAssociatedObject(view,&NGDockNativeSurfaceKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC); objc_setAssociatedObject(view.layer,&NGDockNativeSurfaceKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [dock.nativeChildren removeAllObjects]; dock.originalBar.backgroundColor=dock.nativeBackground; dock.originalBar.layer.backgroundColor=dock.nativeLayerBackground.CGColor;
    objc_setAssociatedObject(dock.originalBar,&NGDockNativeSurfaceKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC); objc_setAssociatedObject(dock.originalBar.layer,&NGDockNativeSurfaceKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    NGDockNativeSurfaceWriting=NO;
    objc_setAssociatedObject(dock.originalBar,&NGDockNativeOwnerKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [dock.collapsePan.view removeGestureRecognizer:dock.collapsePan];
}
static void NGDockHideNativeContent(NGSystemDock *dock) {
    UIView *bar=dock.originalBar; if (!bar) return;
    NGDockNativeSurfaceWriting=YES;
    for (UIView *child in bar.subviews) {
        if (child==dock.view) continue;
        if (![dock.nativeChildren objectForKey:child]) [dock.nativeChildren setObject:@[@(child.layer.opacity),@(child.userInteractionEnabled)] forKey:child];
        NGDockRegisterNativeSurface(child,dock,NO);
        if (child.layer.opacity!=0) child.layer.opacity=0; if (child.userInteractionEnabled) child.userInteractionEnabled=NO;
    }
    if (bar.backgroundColor && CGColorGetAlpha(bar.backgroundColor.CGColor)>0) bar.backgroundColor=UIColor.clearColor;
    if (bar.layer.backgroundColor && CGColorGetAlpha(bar.layer.backgroundColor)>0) bar.layer.backgroundColor=UIColor.clearColor.CGColor;
    NGDockNativeSurfaceWriting=NO;
}
static void NGApplyDockItems(UITabBarController *host) {
    NGSystemDock *dock = objc_getAssociatedObject(host, &NGDockAssociation);
    if (dock && NGDockHasPlaybackPanel(host)) dock.view.hidden = YES;
    UIView *original = dock.originalBar;
    if (!original) for (UIView *view in host.view.subviews) if ([NGClassName(view) isEqualToString:@"SSTabBar"]) { original = view; break; }
    if (!original.superview) return;
    NSArray<UIControl *> *allButtons = NGDockOriginalButtons(original);
    NSMutableArray<UIViewController *> *hostPages = [NSMutableArray array];
    for (UIViewController *page in host.viewControllers) {
        if ([page isKindOfClass:NGSystemDock.class] || NGDockPlaybackPanelClass(NGClassName(page)) || [NGClassName(page) isEqualToString:@"FQVModalNavigationController"]) continue;
        [hostPages addObject:page];
    }
    if (!allButtons.count || allButtons.count != hostPages.count) return;

    NSSet *hidden = NGHiddenDockItems();
    NSMutableArray<NSNumber *> *indexes = [NSMutableArray array];
    NSMutableArray<UIControl *> *visibleButtons = [NSMutableArray array];
    for (NSUInteger index = 0; index < allButtons.count; index++) {
        if ([hidden containsObject:NGDockItemKey(index)]) continue;
        [indexes addObject:@(index)];
        [visibleButtons addObject:allButtons[index]];
    }
    // A damaged or old preference must not remove every way to navigate.
    if (!indexes.count) { [indexes addObject:@0]; [visibleButtons addObject:allButtons[0]]; }
    NSUInteger selected = [hostPages indexOfObject:host.selectedViewController];
    if (![indexes containsObject:@(selected)]) {
        NSUInteger replacement = indexes.firstObject.unsignedIntegerValue;
        [allButtons[replacement] sendActionsForControlEvents:UIControlEventTouchUpInside];
        if (host.selectedViewController != hostPages[replacement]) host.selectedViewController = hostPages[replacement];
        selected = replacement;
    }

    BOOL enabled = NGFeatureBool(NGPreferenceDock);
    if (!enabled && dock) {
        NGDockRestoreNativeContent(dock);
        host.tabBar.hidden = dock.systemBarHidden;
        [dock willMoveToParentViewController:nil];
        [dock.view removeFromSuperview];
        [dock removeFromParentViewController];
        objc_setAssociatedObject(host, &NGDockAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        dock = nil;
    }
    for (UIControl *button in allButtons) NGDockRestoreButton(button);
    if (!enabled) {
        if (visibleButtons.count == allButtons.count) return;
        // Reflow only the native navigation buttons, within their original row.
        UIView *container = allButtons.firstObject.superview;
        BOOL commonParent = YES;
        CGRect row = CGRectNull;
        for (UIControl *button in allButtons) {
            commonParent = commonParent && button.superview == container;
            NGDockButtonState *state = objc_getAssociatedObject(button, &NGDockItemAssociation);
            row = CGRectUnion(row, state.originalFrame);
            state.hiddenManaged = YES;
            button.hidden = ![visibleButtons containsObject:button] || state.originalHidden;
        }
        if (commonParent) {
            CGFloat width = CGRectGetWidth(row) / visibleButtons.count;
            [visibleButtons enumerateObjectsUsingBlock:^(UIControl *button, NSUInteger position, BOOL *stop) {
                NGDockButtonState *state = objc_getAssociatedObject(button, &NGDockItemAssociation);
                CGRect frame = state.originalFrame;
                frame.origin.x = CGRectGetMinX(row) + position * width;
                frame.size.width = width;
                state.appliedFrame = frame;
                state.frameManaged = YES;
                button.frame = frame;
            }];
        }
        return;
    }
    if (!dock) {
        dock = [NGSystemDock new];
        dock.host = host;
        dock.originalBar = original;
        dock.originalHidden = original.hidden;
        dock.systemBarHidden = host.tabBar.hidden;
        [host addChildViewController:dock];
        [original addSubview:dock.view];
        dock.nativeBackground=original.backgroundColor; dock.nativeLayerBackground=original.layer.backgroundColor ? [UIColor colorWithCGColor:original.layer.backgroundColor] : nil;
        NGDockRegisterNativeSurface(original,dock,YES);
        objc_setAssociatedObject(original,&NGDockNativeOwnerKey,dock,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [dock didMoveToParentViewController:host];
        objc_setAssociatedObject(host, &NGDockAssociation, dock, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    dock.buttons = visibleButtons;
    if (![dock.sourceIndexes isEqualToArray:indexes]) {
        NSMutableArray *pages = [NSMutableArray array];
        for (NSNumber *source in indexes) {
            NSUInteger index = source.unsignedIntegerValue;
            UIViewController *page = dock.pages[source];
            if (!page) {
                page = [UIViewController new];
                page.view.backgroundColor = UIColor.clearColor;
                dock.pages[source] = page;
            }
            NSString *title = allButtons[index].accessibilityLabel ?: hostPages[index].tabBarItem.title;
            NSMutableArray *children = [NSMutableArray arrayWithObject:allButtons[index]];
            for (NSUInteger cursor = 0; cursor < children.count && cursor < 50; cursor++) {
                UIView *child = children[cursor];
                if ([child isKindOfClass:UILabel.class] && [(UILabel *)child text].length) { title = [(UILabel *)child text]; break; }
                [children addObjectsFromArray:child.subviews];
            }
            page.tabBarItem = [[UITabBarItem alloc] initWithTitle:nil image:nil tag:index];
            page.tabBarItem.accessibilityLabel = title;
            [pages addObject:page];
        }
        dock.sourceIndexes = indexes;
        [dock setViewControllers:pages animated:NO];
    }
    dock.selectedIndex = [indexes indexOfObject:@(selected)];
    [dock refreshImages];
    CGRect frame=original.bounds;
    if (!CGRectEqualToRect(dock.view.frame,frame)) dock.view.frame=frame;
    dock.view.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    dock.view.hidden=!host.view.window || CGRectGetMinY(original.frame)>=CGRectGetHeight(host.view.bounds) || NGDockHasPlaybackPanel(host);
    [dock layoutGlass]; NGDockHideNativeContent(dock);
    host.tabBar.hidden=YES;
    if (original.subviews.lastObject!=dock.view) [original bringSubviewToFront:dock.view];

}

static void NGRefreshDock(UITabBarController *host) {
    if (!host || [host isKindOfClass:NGSystemDock.class] || !host.isViewLoaded || [objc_getAssociatedObject(host, &NGDockUpdatingAssociation) boolValue]) return;
    objc_setAssociatedObject(host, &NGDockUpdatingAssociation, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    @try { NGApplyDockItems(host); }
    @finally { objc_setAssociatedObject(host, &NGDockUpdatingAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
}
static void NGUpdateDock(void) {
    NGUpdateDockUnderlay();
    NSMutableArray *queue = [NSMutableArray array];
    UIViewController *root = NGBaseWindow().rootViewController;
    if (root) [queue addObject:root];
    for (NSUInteger index = 0; index < queue.count && index < 200; index++) {
        UIViewController *owner = queue[index];
        if ([owner isKindOfClass:NGSystemDock.class]) continue;
        if ([NGClassName(owner) isEqualToString:@"SSTabBarController"]) NGRefreshDock((UITabBarController *)owner);
        [queue addObjectsFromArray:owner.childViewControllers];
    }
}

@interface NGDockItemsSettingsController : NGSettingsTableController
@end
@implementation NGDockItemsSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"隐藏底栏项目";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
    self.navigationItem.leftBarButtonItem.accessibilityLabel = @"返回";
}
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return NGDockItemKeys().count; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    NSString *key = NGDockItemKeys()[path.row];
    NSSet *hidden = NGHiddenDockItems();
    cell.textLabel.text = NGDockItemTitles()[path.row];
    UISwitch *toggle = [UISwitch new];
    toggle.on = [hidden containsObject:key];
    NSUInteger visible = 0;
    for (NSString *item in NGDockItemKeys()) if (![hidden containsObject:item]) visible++;
    toggle.enabled = toggle.on || visible > 1;
    toggle.accessibilityIdentifier = key;
    toggle.accessibilityLabel = [@"隐藏" stringByAppendingString:cell.textLabel.text];
    [toggle addTarget:self action:@selector(itemChanged:) forControlEvents:UIControlEventValueChanged];
    cell.accessoryView = toggle;
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    return cell;
}
- (void)itemChanged:(UISwitch *)sender {
    if (![NGDockItemKeys() containsObject:sender.accessibilityIdentifier]) return;
    NSMutableSet *hidden = [NGHiddenDockItems() mutableCopy];
    if (sender.on) [hidden addObject:sender.accessibilityIdentifier];
    else [hidden removeObject:sender.accessibilityIdentifier];
    BOOL hidesEveryItem = YES;
    for (NSString *key in NGDockItemKeys()) if (![hidden containsObject:key]) { hidesEveryItem = NO; break; }
    if (hidesEveryItem) { sender.on = NO; return; }
    [NSUserDefaults.standardUserDefaults setObject:hidden.allObjects forKey:NGPreferenceHiddenDockItems];
    NGUpdateDock();
    [self.tableView reloadData];
}
@end

@interface NGDockPreviewController : NGSystemDock
- (void)refreshIcons;
@end
@implementation NGDockPreviewController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.accessibilityIdentifier = @"NaiwaGuo.Dock.Preview";
    NSMutableArray *pages = [NSMutableArray array];
    for (NSUInteger index = 0; index < NGDockItemTitles().count; index++) {
        UIViewController *page = [UIViewController new];
        page.tabBarItem = [[UITabBarItem alloc] initWithTitle:nil image:nil tag:index];
        page.tabBarItem.accessibilityLabel = NGDockItemTitles()[index];
        [pages addObject:page];
    }
    self.viewControllers = pages;
    [self refreshIcons];
}
- (void)refreshIcons { [self refreshImages]; }
- (BOOL)tabBarController:(UITabBarController *)controller shouldSelectViewController:(UIViewController *)page { self.selectedIndex=[self.viewControllers indexOfObject:page]; return YES; }
- (void)traitCollectionDidChange:(UITraitCollection *)previous { [super traitCollectionDidChange:previous]; [self refreshIcons]; }
@end

@interface NGDockPreviewSettingsController : NGSettingsTableController
@property(nonatomic, strong) NGDockPreviewController *preview;
@end
@implementation NGDockPreviewSettingsController
- (void)viewDidLoad {
    [super viewDidLoad];
    CGFloat bottom = NGBaseWindow().safeAreaInsets.bottom;
    UIView *header = [[UIView alloc] initWithFrame:CGRectMake(0, 0, CGRectGetWidth(self.tableView.bounds), 80 + bottom)];
    self.preview = [NGDockPreviewController new];
    [self addChildViewController:self.preview];
    self.preview.additionalSafeAreaInsets = UIEdgeInsetsZero;
    self.preview.view.frame = CGRectMake(0, 0, CGRectGetWidth(header.bounds), 49 + bottom);
    self.preview.view.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    [header addSubview:self.preview.view];
    [self.preview didMoveToParentViewController:self];
    self.tableView.tableHeaderView = header;
}
- (void)viewWillAppear:(BOOL)animated { [super viewWillAppear:animated]; [self.preview refreshIcons]; }
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CGFloat width = CGRectGetWidth(self.tableView.bounds);
    CGRect frame = self.preview.view.frame;
    frame.origin.x = (CGRectGetWidth(self.tableView.bounds) - width) / 2;
    frame.size.width = width;
    self.preview.view.frame = frame;
    [self.preview refreshIcons];
}
@end

@interface NGDockIconSettingsController : NGDockPreviewSettingsController
@end
@implementation NGDockIconSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"底栏图标预设";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
}
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 2; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section ? NGDockItemKeys().count : NGDockIconStyles().count; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return section ? @"自定义单个图标" : @"预设"; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    if (!path.section) {
        NSString *style = NGDockIconStyles()[path.row]; cell.textLabel.text = NGDockIconStyleTitles()[path.row];
        cell.detailTextLabel.text = @[@"轻盈线条，选中时填充", @"圆角轮廓，统一笔画", @"以场记板和网格为主题", @"叠层方形与胶片元素"][path.row];
        cell.accessoryType = [style isEqualToString:NGDockIconStyle()] ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    } else {
    cell.textLabel.text = NGDockItemTitles()[path.row];
    UIImage *icon = NGDockIcon(path.row, NO);
    cell.imageView.image = [NGIconImageOnCanvas(icon, 28, 25) imageWithRenderingMode:icon.renderingMode == UIImageRenderingModeAlwaysOriginal ? UIImageRenderingModeAlwaysOriginal : UIImageRenderingModeAlwaysTemplate];
    cell.detailTextLabel.text = @"上传图片或输入 SF Symbols 名称";
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if (!path.section) { [NSUserDefaults.standardUserDefaults setObject:NGDockIconStyles()[path.row] forKey:NGPreferenceDockIconStyle]; NGUpdateDock(); [self.preview refreshIcons]; [tableView reloadData]; return; }
    NSString *key = [NSString stringWithFormat:@"com.naiwaguo.appearance.dockIcon.%ld", (long)path.row];
    NSString *imageKey = NGDockIconImageKey(path.row);
    UIAlertController *menu = [UIAlertController alertControllerWithTitle:@"自定义底栏图标" message:@"上传自己的图片，或输入 SF Symbols 名称。" preferredStyle:UIAlertControllerStyleActionSheet];
    [menu addAction:[UIAlertAction actionWithTitle:@"从照片上传" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) { NGPresentIconPhotoPicker(self, imageKey); }]];
    [menu addAction:[UIAlertAction actionWithTitle:@"输入 SF Symbols 名称" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"输入图标名称" message:@"例如 house.fill、film、person.crop.circle" preferredStyle:UIAlertControllerStyleAlert];
        [alert addTextFieldWithConfigurationHandler:^(UITextField *field) { field.text = NGFeatureString(key); field.placeholder = @"house.fill"; field.autocapitalizationType = UITextAutocapitalizationTypeNone; }];
        [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [alert addAction:[UIAlertAction actionWithTitle:@"保存" style:UIAlertActionStyleDefault handler:^(UIAlertAction *save) { NSString *name = alert.textFields.firstObject.text; if (name.length && [UIImage systemImageNamed:name]) { [NSUserDefaults.standardUserDefaults setObject:name forKey:key]; [NSUserDefaults.standardUserDefaults removeObjectForKey:imageKey]; } else { [NSUserDefaults.standardUserDefaults removeObjectForKey:key]; [NSUserDefaults.standardUserDefaults removeObjectForKey:imageKey]; } NGUpdateDock(); [tableView reloadData]; }]];
        [self presentViewController:alert animated:YES completion:nil];
    }]];
    [menu addAction:[UIAlertAction actionWithTitle:@"恢复预设" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action) { [NSUserDefaults.standardUserDefaults removeObjectForKey:key]; [NSUserDefaults.standardUserDefaults removeObjectForKey:imageKey]; NGUpdateDock(); [tableView reloadData]; }]];
    [menu addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [self presentViewController:menu animated:YES completion:nil];
}
@end

static void NGInstallDockHooks(void) {
    NGInstallDockAvatarHooks();
    NGInstallDockNativeSurfaceHooks();
    NGDockInstallPlatterHooks();
    static BOOL hitInstalled; Class barClass=NSClassFromString(@"SSTabBar");
    if (barClass && !hitInstalled) {
        hitInstalled=YES; SEL selector=@selector(hitTest:withEvent:); Method method=class_getInstanceMethod(barClass,selector); IMP prior=method_getImplementation(method);
        class_replaceMethod(barClass,selector,imp_implementationWithBlock(^UIView *(UIView *bar,CGPoint point,UIEvent *event){ NGSystemDock *dock=objc_getAssociatedObject(bar,&NGDockNativeOwnerKey); if (dock && !bar.hidden && bar.alpha>=0.01) return [dock.view hitTest:[bar convertPoint:point toView:dock.view] withEvent:event]; return ((UIView *(*)(id,SEL,CGPoint,id))prior)(bar,selector,point,event); }),method_getTypeEncoding(method));
    }

    static BOOL additionInstalled;
    if (barClass && !additionInstalled) {
        additionInstalled=YES; SEL sel=@selector(didAddSubview:); Method method=class_getInstanceMethod(barClass,sel); IMP original=method_getImplementation(method);
        class_replaceMethod(barClass,sel,imp_implementationWithBlock(^(UIView *bar,UIView *child){
            ((void(*)(id,SEL,id))original)(bar,sel,child); NGSystemDock *dock=objc_getAssociatedObject(bar,&NGDockNativeOwnerKey); if (dock && child!=dock.view) NGDockHideNativeContent(dock);
        }),method_getTypeEncoding(method));
    }
    NGInstallDockUnderlayHooks();
    static NSMutableSet *installed;
    if (!installed) installed = [NSMutableSet set];
    for (NSString *name in @[@"SSTabBarController", @"SSTabBar"]) {
        Class target = NSClassFromString(name);
        if (!target || [installed containsObject:name]) continue;
        BOOL controller = [name isEqualToString:@"SSTabBarController"];
        SEL selector = controller ? @selector(viewDidLayoutSubviews) : @selector(layoutSubviews);
        Method method = class_getInstanceMethod(target, selector);
        if (!method) continue;
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(id owner) {
            ((void (*)(id, SEL))original)(owner, selector);
            if (controller) NGRefreshDock(owner);
            else for (UIResponder *next = [owner nextResponder]; next; next = next.nextResponder) {
                if ([NGClassName(next) isEqualToString:@"SSTabBarController"]) { NGRefreshDock((UITabBarController *)next); break; }
            }
        });
        if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
        [installed addObject:name];
    }
}

@interface NGDockColorSettingsController : NGDockPreviewSettingsController <UIColorPickerViewControllerDelegate>
@property(nonatomic) BOOL editingSelected;
@property(nonatomic) BOOL refreshScheduled;
@end
@implementation NGDockColorSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"底栏颜色";
    [self updatePreview];
}
- (void)updatePreview {
    [self.preview refreshIcons];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 2; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section == 0 ? 2 : 1; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return section == 0 ? @"图标与文字" : nil; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    if (path.section) { cell.textLabel.text = @"恢复默认颜色"; cell.textLabel.textColor = NGSettingsAccent(); return cell; }
    cell.textLabel.text = path.row ? @"已选中" : @"未选中";
    UIView *swatch = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 30, 30)];
    swatch.backgroundColor = NGDockColor(path.row == 1, self.traitCollection.userInterfaceStyle);
    swatch.layer.cornerRadius = 15; swatch.layer.borderWidth = 1; swatch.layer.borderColor = UIColor.separatorColor.CGColor;
    cell.accessoryView = swatch; return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if (path.section) {
        [NSUserDefaults.standardUserDefaults removeObjectForKey:NGPreferenceDockNormalColor]; [NSUserDefaults.standardUserDefaults removeObjectForKey:NGPreferenceDockSelectedColor];
        NGUpdateDock(); [self updatePreview]; [tableView reloadData]; return;
    }
    self.editingSelected = path.row == 1;
    UIColorPickerViewController *picker = [UIColorPickerViewController new]; picker.delegate = self; picker.supportsAlpha = YES;
    picker.title = self.editingSelected ? @"已选中颜色" : @"未选中颜色";
    picker.selectedColor = NGDockColor(self.editingSelected, self.traitCollection.userInterfaceStyle);
    [self presentViewController:picker animated:YES completion:nil];
}
- (void)colorPickerViewController:(UIColorPickerViewController *)picker didSelectColor:(UIColor *)color continuously:(BOOL)continuously {
    [self saveDockColor:color];
}
- (void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)picker { [self saveDockColor:picker.selectedColor]; }
- (void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)picker { [self saveDockColor:picker.selectedColor]; }
- (void)saveDockColor:(UIColor *)color {
    CGFloat r, g, b, a;
    if (![color getRed:&r green:&g blue:&b alpha:&a]) return;
    [NSUserDefaults.standardUserDefaults setObject:@[@(r), @(g), @(b), @(a)] forKey:self.editingSelected ? NGPreferenceDockSelectedColor : NGPreferenceDockNormalColor];
    [self updatePreview];
    if (!self.refreshScheduled) {
        self.refreshScheduled = YES;
        dispatch_async(dispatch_get_main_queue(), ^{ self.refreshScheduled = NO; NGUpdateDock(); [self.tableView reloadData]; });
    }
}
- (void)traitCollectionDidChange:(UITraitCollection *)previous { [super traitCollectionDidChange:previous]; [self updatePreview]; }
@end

static BOOL NGDockInvokeNativeSearch(UITabBarController *host) {
    // Verified public Objective-C entry point in Hongguo 7.4.3; no guessed URL.
    for (UIViewController *page in host.viewControllers) {
        NSMutableArray *controllers=[NSMutableArray arrayWithObject:page];
        for (NSUInteger i=0;i<controllers.count && i<80;i++) {
            UIViewController *controller=controllers[i]; SEL selector=NSSelectorFromString(@"simpleSearchViewDidClicked");
            if ([NGClassName(controller) isEqualToString:@"SSVideoSeriesFeedViewController"] && [controller respondsToSelector:selector]) {
                NSMethodSignature *signature=[controller methodSignatureForSelector:selector];
                if (signature.numberOfArguments!=2 || signature.methodReturnType[0]!='v') continue;
                host.selectedViewController=page; [controller loadViewIfNeeded];
                dispatch_async(dispatch_get_main_queue(),^{ ((void(*)(id,SEL))[controller methodForSelector:selector])(controller,selector); }); return YES;
            }
            [controllers addObjectsFromArray:controller.childViewControllers];
        }
    }
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"暂时无法打开搜索" message:@"当前红果版本未找到原生搜索入口，请先返回首页使用顶部搜索。" preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]]; [NGTopController() presentViewController:alert animated:YES completion:nil]; return NO;
}
static BOOL NGDockInvokeAction(NGSystemDock *dock) {
    if (!dock.host) return NO;
    NSString *action=NGDockAction();
    if ([action isEqual:@"search"]) return NGDockInvokeNativeSearch(dock.host);
    if ([action isEqual:@"settings"]) { NGOpenSettings(); return YES; }
    NSUInteger index=[NGDockItemKeys() indexOfObject:action];
    NSArray<UIControl *> *buttons=dock.originalBar ? NGDockOriginalButtons(dock.originalBar) : @[];
    if (index==NSNotFound || index>=buttons.count || !buttons[index].enabled) return NO;
    [buttons[index] sendActionsForControlEvents:UIControlEventTouchUpInside];
    NGRefreshDock(dock.host); return YES;
}
#include "NGDockAppearanceSettings.inc"
