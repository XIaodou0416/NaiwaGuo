static NSString *const NGPreferencePlaybackPreset = @"com.naiwaguo.playback.iconPreset";
static char NGPlaybackPresetAssociation;
static char NGPlaybackPresetIconAssociation;
static NSHashTable<UIView *> *NGPlaybackPresetActions;
static void NGUpdatePlaybackGlass(void);
static NSArray<NSString *> *NGPlaybackActionClassNames(void) {
    static NSArray<NSString *> *names;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ names = @[@"SSShortVideoRightInteractionCollectView", @"SSShortVideoRightInteractionCommentView", @"SSShortVideoRightInteractionLikeView", @"SSShortVideoRightInteractionShareView"]; });
    return names;
}

static NSArray<NSString *> *NGPlaybackPresets(void) { return @[@"original", @"soft", @"bold", @"badge", @"duotone"]; }
static NSArray<NSString *> *NGPlaybackPresetTitles(void) { return @[@"保留原版", @"极简细线", @"圆润实心", @"轻巧线框", @"柔彩双色"]; }
static NSString *NGPlaybackCurrentPreset(void) {
    NSString *preset = [NSUserDefaults.standardUserDefaults stringForKey:NGPreferencePlaybackPreset];
    return [NGPlaybackPresets() containsObject:preset] ? preset : @"original";
}

@interface NGPlaybackPresetState : NSObject
@property(nonatomic, weak) UIImageView *original;
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, id> *originalImages;
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, id> *originalTints;
@property(nonatomic, strong) UIImage *originalHighlightedImage;
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, UIImage *> *lastAppliedImages;
@property(nonatomic, strong) UIImage *lastAppliedHighlightedImage;
@property(nonatomic) BOOL lastSelected;
@property(nonatomic,weak) id model;
@property(nonatomic, weak) UIView *action;
@property(nonatomic) NSUInteger actionIndex;
@property(nonatomic) BOOL applying;
@end
@implementation NGPlaybackPresetState
@end

static BOOL NGPlaybackActionSelected(UIView *action);
static void NGRestorePlaybackPreset(UIView *action) {
    NGPlaybackPresetState *state = objc_getAssociatedObject(action, &NGPlaybackPresetAssociation);
    if (!state) return;
    UIImageView *icon = state.original;
    objc_setAssociatedObject(icon, &NGPlaybackPresetIconAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (icon) {
        NSNumber *currentKey = @(NGPlaybackActionSelected(action) || icon.highlighted);
        BOOL imageWasApplied = icon.image && [state.lastAppliedImages.allValues containsObject:icon.image];
        if (!imageWasApplied) {
            state.originalImages[currentKey] = icon.image ?: NSNull.null;
            state.originalTints[currentKey] = icon.tintColor ?: NSNull.null;
        } else {
            id original = state.originalImages[currentKey] ?: state.originalImages[@(state.lastSelected)] ?: NSNull.null;
            icon.image = original == NSNull.null ? nil : original;
            id tint = state.originalTints[currentKey] ?: NSNull.null;
            if (tint != NSNull.null) icon.tintColor = tint;
        }
        if ((state.lastAppliedHighlightedImage && icon.highlightedImage == state.lastAppliedHighlightedImage) ||
            (icon.highlightedImage && [state.lastAppliedImages.allValues containsObject:icon.highlightedImage]))
            icon.highlightedImage = state.originalHighlightedImage;
    }
    objc_setAssociatedObject(action, &NGPlaybackPresetAssociation, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static NSString *NGPlaybackCustomSymbol(NSUInteger index, BOOL selected) {
    NSArray *names = @[@"collect", @"comment", @"like", @"share"];
    NSString *key = [NSString stringWithFormat:@"com.naiwaguo.playback.custom.%@.%@", names[MIN(index, names.count - 1)], selected ? @"selected" : @"normal"];
    return [NSUserDefaults.standardUserDefaults stringForKey:key];
}
static NSString *NGPlaybackCustomImageKey(NSUInteger index, BOOL selected) {
    NSArray *names = @[@"collect", @"comment", @"like", @"share"];
    return [NSString stringWithFormat:@"com.naiwaguo.playback.customImage.%@.%@", names[MIN(index, names.count - 1)], selected ? @"selected" : @"normal"];
}
static BOOL NGPlaybackHasCustomForAction(NSUInteger index) {
    for (NSNumber *selected in @[@NO, @YES]) {
        if (NGPlaybackCustomSymbol(index, selected.boolValue).length ||
            [NSUserDefaults.standardUserDefaults dataForKey:NGPlaybackCustomImageKey(index, selected.boolValue)].length) return YES;
    }
    return NO;
}
static UIImage *NGPlaybackSizedCustomImage(UIImage *source) {
    if (!source) return nil;
    CGSize canvas = CGSizeMake(46, 46), limit = CGSizeMake(25, 25);
    CGFloat ratio = MIN(limit.width / MAX(1, source.size.width), limit.height / MAX(1, source.size.height));
    CGSize size = CGSizeMake(source.size.width * ratio, source.size.height * ratio);
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:canvas];
    return [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        [source drawInRect:CGRectMake((canvas.width - size.width) / 2.0, (canvas.height - size.height) / 2.0, size.width, size.height)];
    }];
}
static NSMapTable<id,NSHashTable<UIView *> *> *NGPlaybackModelActions;
static id NGPlaybackActionModel(UIView *action) {
    SEL selector=NSSelectorFromString(@"model");
    if (![action respondsToSelector:selector]) return nil;
    NSMethodSignature *signature=[action methodSignatureForSelector:selector];
    if (signature.numberOfArguments!=2 || signature.methodReturnType[0]!='@') return nil;
    return ((id(*)(id,SEL))[action methodForSelector:selector])(action,selector);
}
static NSNumber *NGPlaybackNativeSelection(UIView *action) {
    id model=NGPlaybackActionModel(action);
    // Confirmed against Hongguo 7.4.3: LikeView keeps isLiked on its model;
    // CollectView and its model expose isCollected. Never toggle speculatively.
    for (id object in model ? @[model,action] : @[action]) {
        for (NSString *name in @[@"isLiked",@"isCollected",@"isFavorited"]) {
            SEL selector=NSSelectorFromString(name); if (![object respondsToSelector:selector]) continue;
            NSMethodSignature *signature=[object methodSignatureForSelector:selector]; const char *type=signature.methodReturnType;
            if (signature.numberOfArguments==2 && (*type=='B' || *type=='c')) return @(((BOOL(*)(id,SEL))[object methodForSelector:selector])(object,selector));
        }
    }
    return nil;
}
static void NGPlaybackObserveModel(UIView *action,NGPlaybackPresetState *state) {
    id model=NGPlaybackActionModel(action); if (model==state.model) return;
    if (state.model) [[NGPlaybackModelActions objectForKey:state.model] removeObject:action]; state.model=model;
    if (!model) return;
    if (!NGPlaybackModelActions) NGPlaybackModelActions=[NSMapTable weakToStrongObjectsMapTable];
    NSHashTable *actions=[NGPlaybackModelActions objectForKey:model]; if (!actions) { actions=[NSHashTable weakObjectsHashTable]; [NGPlaybackModelActions setObject:actions forKey:model]; }
    [actions addObject:action];
}
static BOOL NGPlaybackActionSelected(UIView *action) {
    NSNumber *native=NGPlaybackNativeSelection(action); if (native) return native.boolValue;
    if ([action isKindOfClass:UIControl.class] && ((UIControl *)action).selected) return YES;
    NSMutableArray *queue = [NSMutableArray arrayWithObject:action];
    for (NSUInteger i = 0; i < queue.count && i < 80; i++) {
        UIView *view = queue[i];
        if ([view isKindOfClass:UIControl.class] && ((UIControl *)view).selected) return YES;
        for (NSString *selectorName in @[@"isLiked", @"isCollected", @"isFavorited", @"isSelected"]) {
            SEL selector = NSSelectorFromString(selectorName);
            if ([view respondsToSelector:selector]) { BOOL (*sendBool)(id, SEL) = (BOOL (*)(id, SEL))[view methodForSelector:selector]; if (sendBool(view, selector)) return YES; }
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return NO;
}

// Four new vector families, rendered on the same 46-point canvas as custom images.
static UIBezierPath *NGPlaybackVectorPath(NSUInteger index) {
    UIBezierPath *p=[UIBezierPath bezierPath];
    if (index==0) { [p moveToPoint:CGPointMake(7,4)]; [p addLineToPoint:CGPointMake(21,4)]; [p addQuadCurveToPoint:CGPointMake(23,6) controlPoint:CGPointMake(23,4)]; [p addLineToPoint:CGPointMake(23,26)]; [p addLineToPoint:CGPointMake(14,21)]; [p addLineToPoint:CGPointMake(5,26)]; [p addLineToPoint:CGPointMake(5,6)]; [p addQuadCurveToPoint:CGPointMake(7,4) controlPoint:CGPointMake(5,4)]; [p closePath]; }
    else if (index==1) { [p moveToPoint:CGPointMake(7,5)]; [p addLineToPoint:CGPointMake(22,5)]; [p addQuadCurveToPoint:CGPointMake(26,9) controlPoint:CGPointMake(26,5)]; [p addLineToPoint:CGPointMake(26,18)]; [p addQuadCurveToPoint:CGPointMake(22,22) controlPoint:CGPointMake(26,22)]; [p addLineToPoint:CGPointMake(13,22)]; [p addLineToPoint:CGPointMake(6,26)]; [p addLineToPoint:CGPointMake(7,22)]; [p addQuadCurveToPoint:CGPointMake(3,18) controlPoint:CGPointMake(3,22)]; [p addLineToPoint:CGPointMake(3,9)]; [p addQuadCurveToPoint:CGPointMake(7,5) controlPoint:CGPointMake(3,5)]; [p closePath]; }
    else if (index==2) { [p moveToPoint:CGPointMake(14,26)]; [p addCurveToPoint:CGPointMake(3,11) controlPoint1:CGPointMake(9,22) controlPoint2:CGPointMake(3,17)]; [p addCurveToPoint:CGPointMake(14,8) controlPoint1:CGPointMake(3,3) controlPoint2:CGPointMake(11,2)]; [p addCurveToPoint:CGPointMake(25,11) controlPoint1:CGPointMake(18,2) controlPoint2:CGPointMake(25,3)]; [p addCurveToPoint:CGPointMake(14,26) controlPoint1:CGPointMake(25,17) controlPoint2:CGPointMake(19,22)]; [p closePath]; }
    else { [p moveToPoint:CGPointMake(3,12)]; [p addLineToPoint:CGPointMake(26,3)]; [p addLineToPoint:CGPointMake(18,27)]; [p addLineToPoint:CGPointMake(12,18)]; [p addLineToPoint:CGPointMake(3,12)]; [p closePath]; }
    p.lineCapStyle=kCGLineCapRound; p.lineJoinStyle=kCGLineJoinRound; return p;
}
static UIImage *NGPlaybackVectorIcon(NSUInteger index,BOOL selected,NSString *preset) {
    BOOL solid=[preset isEqualToString:@"bold"] || selected, framed=[preset isEqualToString:@"badge"], duo=[preset isEqualToString:@"duotone"];
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(46,46)];
    UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        CGContextRef cg=context.CGContext; UIColor *ink=selected && index==0 ? UIColor.systemYellowColor : (selected && index==2 ? UIColor.systemPinkColor : UIColor.whiteColor);
        if (duo) {
            NSArray *colors=@[[UIColor colorWithRed:1 green:0.76 blue:0.25 alpha:1],[UIColor colorWithRed:0.4 green:0.75 blue:1 alpha:1],[UIColor colorWithRed:1 green:0.39 blue:0.51 alpha:1],[UIColor colorWithRed:0.47 green:0.85 blue:0.72 alpha:1]]; ink=colors[MIN(index,3)];
            [[ink colorWithAlphaComponent:0.18] setFill]; [[UIBezierPath bezierPathWithRoundedRect:CGRectMake(4,4,38,38) cornerRadius:13] fill];
        }
        [ink setStroke]; [ink setFill];
        if (framed) { UIBezierPath *border=[UIBezierPath bezierPathWithRoundedRect:CGRectMake(5,5,36,36) cornerRadius:11]; border.lineWidth=1.5; [border stroke]; CGContextTranslateCTM(cg,11.8,11.8); CGContextScaleCTM(cg,0.8,0.8); }
        else CGContextTranslateCTM(cg,9,8);
        UIBezierPath *shape=NGPlaybackVectorPath(index); shape.lineWidth=[preset isEqualToString:@"soft"] ? 1.65 : 2.1;
        if (solid) [shape fill]; else [shape stroke];
        if (index==1) {
            if (solid) CGContextSetBlendMode(cg,kCGBlendModeClear);
            for (NSUInteger i=0;i<3;i++) [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(8+i*5,12,2,2)] fill];
        }
        if (index==3 && !solid) { UIBezierPath *fold=[UIBezierPath bezierPath]; [fold moveToPoint:CGPointMake(12,18)]; [fold addLineToPoint:CGPointMake(22,7)]; fold.lineWidth=shape.lineWidth; fold.lineCapStyle=kCGLineCapRound; [fold stroke]; }
    }];
    return [image imageWithRenderingMode:duo || (selected && (index==0 || index==2)) ? UIImageRenderingModeAlwaysOriginal : UIImageRenderingModeAlwaysTemplate];
}
static UIImage *NGPlaybackIconPreview(NSUInteger index, BOOL selected, NSString *preset) {
    NSData *data = [NSUserDefaults.standardUserDefaults dataForKey:NGPlaybackCustomImageKey(index, selected)];
    NSString *custom = NGPlaybackCustomSymbol(index, selected);
    NSString *key = [NSString stringWithFormat:@"%@|%lu|%d|%@|%lu", preset, (unsigned long)index, selected, custom ?: @"", (unsigned long)data.hash];
    static NSCache<NSString *, UIImage *> *cache;
    if (!cache) { cache = [NSCache new]; cache.countLimit = 80; }
    UIImage *image = [cache objectForKey:key];
    if (image) return image;
    if (data.length) image = [NGPlaybackSizedCustomImage([UIImage imageWithData:data]) imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    if (!image && !custom.length && ![preset isEqualToString:@"original"]) image=NGPlaybackVectorIcon(index,selected,preset);
    if (!image) {
        NSArray *symbols = [preset isEqualToString:@"badge"] ? @[@"bookmark.circle", @"text.bubble", @"heart.circle", @"arrowshape.turn.up.right"] :
            ([preset isEqualToString:@"bold"] ? @[@"bookmark.fill", @"text.bubble.fill", @"heart.fill", @"arrowshape.turn.up.right.fill"] : @[@"bookmark", @"text.bubble", @"heart", @"arrowshape.turn.up.right"]);
        NSString *symbol = custom.length ? custom : symbols[MIN(index, symbols.count - 1)];
        if (selected && !custom.length && ![symbol hasSuffix:@".fill"]) symbol = [symbol stringByAppendingString:@".fill"];
        CGFloat size = [preset isEqualToString:@"soft"] ? 21 : 23;
        UIImageSymbolWeight weight = [preset isEqualToString:@"soft"] ? UIImageSymbolWeightLight : ([preset isEqualToString:@"bold"] ? UIImageSymbolWeightSemibold : UIImageSymbolWeightMedium);
        image = [NGIconImageOnCanvas([UIImage systemImageNamed:symbol withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:size weight:weight]], 46, size) imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    }
    if (image) [cache setObject:image forKey:key];
    return image;
}

static void NGApplyPlaybackPreset(UIView *action, NSUInteger actionIndex) {
    if (!NGPlaybackPresetActions) NGPlaybackPresetActions = [NSHashTable weakObjectsHashTable];
    [NGPlaybackPresetActions addObject:action];
    NSString *preset = NGPlaybackCurrentPreset();
    UIImageView *icon = nil;
    for (UIView *view in action.subviews) {
        if ([view.accessibilityIdentifier isEqualToString:@"NaiwaGuo.Playback.IconPreset"]) continue;
        if ([view isKindOfClass:UIImageView.class]) { icon = (UIImageView *)view; break; }
    }
    if (!icon) { NGRestorePlaybackPreset(action); return; }
    NSNumber *nativeSelection=NGPlaybackNativeSelection(action);
    BOOL selected = actionIndex == 0 || actionIndex == 2 ? (nativeSelection ? nativeSelection.boolValue : (NGPlaybackActionSelected(action) || icon.highlighted)) : NO;
    NSString *custom = NGPlaybackCustomSymbol(actionIndex, selected);
    NSData *customData = [NSUserDefaults.standardUserDefaults dataForKey:NGPlaybackCustomImageKey(actionIndex, selected)];
    UIImage *customImage = customData.length ? NGPlaybackIconPreview(actionIndex, selected, preset) : nil;
    NGPlaybackPresetState *state = objc_getAssociatedObject(action, &NGPlaybackPresetAssociation);
    if (state.applying) return;
    if (state && state.original != icon) { NGRestorePlaybackPreset(action); state = nil; }
    BOOL anyCustom = NGPlaybackHasCustomForAction(actionIndex);
    if ([preset isEqualToString:@"original"] && !anyCustom) { NGRestorePlaybackPreset(action); return; }
    if (!state) {
        state = [NGPlaybackPresetState new];
        state.original = icon;
        state.action = action;
        state.actionIndex = actionIndex;
        state.originalImages = [NSMutableDictionary dictionary];
        state.originalTints = [NSMutableDictionary dictionary];
        state.lastAppliedImages = [NSMutableDictionary dictionary];
        state.originalImages[@NO] = icon.image ?: NSNull.null;
        state.originalImages[@YES] = icon.highlightedImage ?: icon.image ?: NSNull.null;
        state.originalTints[@NO] = icon.tintColor ?: NSNull.null;
        state.originalTints[@YES] = icon.tintColor ?: NSNull.null;
        state.originalHighlightedImage = icon.highlightedImage;
        objc_setAssociatedObject(action, &NGPlaybackPresetAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(icon, &NGPlaybackPresetIconAssociation, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    } else {
        BOOL imageWasApplied = icon.image && [state.lastAppliedImages.allValues containsObject:icon.image];
        if (!imageWasApplied) {
            state.originalImages[@(selected)] = icon.image ?: NSNull.null;
            state.originalTints[@(selected)] = icon.tintColor ?: NSNull.null;
        }
        if (icon.highlightedImage != state.lastAppliedHighlightedImage &&
            (!icon.highlightedImage || ![state.lastAppliedImages.allValues containsObject:icon.highlightedImage]))
            state.originalHighlightedImage = icon.highlightedImage;
    }
    NGPlaybackObserveModel(action,state);
    state.lastSelected = selected;
    NSNumber *stateKey = @(selected);
    state.applying = YES;
    @try {
    if ([preset isEqualToString:@"original"] && !custom.length && !customImage) {
        id original = state.originalImages[stateKey] ?: NSNull.null;
        icon.image = original == NSNull.null ? nil : original;
        icon.highlightedImage = state.originalHighlightedImage;
        id tint = state.originalTints[stateKey] ?: NSNull.null;
        if (tint != NSNull.null) icon.tintColor = tint;
        [state.lastAppliedImages removeObjectForKey:stateKey];
        state.lastAppliedHighlightedImage = nil;
        return;
    }
    UIImage *image = NGPlaybackIconPreview(actionIndex, selected, preset);
    BOOL photo = image.renderingMode == UIImageRenderingModeAlwaysOriginal;
    if (!image) { NGRestorePlaybackPreset(action); return; }
    image = [image imageWithRenderingMode:photo ? UIImageRenderingModeAlwaysOriginal : UIImageRenderingModeAlwaysTemplate];
    // Keep the host's UIImageView and its tap/gesture path intact; never hide it behind an overlay.
    if (icon.image != image) icon.image = image;
    icon.highlightedImage = image;
    if (!photo) icon.tintColor = selected && actionIndex==0 ? UIColor.systemYellowColor : (selected && actionIndex==2 ? UIColor.systemPinkColor : UIColor.whiteColor);
    state.lastAppliedImages[stateKey] = image;
    state.lastAppliedHighlightedImage = image;
    } @finally { state.applying = NO; }
}

static void NGRefreshPlaybackPresetIcons(void) {
    for (UIView *action in NGPlaybackPresetActions.allObjects) {
        NSUInteger index = [NGPlaybackActionClassNames() indexOfObject:NGClassName(action)];
        if (index != NSNotFound) NGApplyPlaybackPreset(action, index);
    }
}

@interface NGPlaybackPresetSettingsController : NGSettingsTableController
@end
@implementation NGPlaybackPresetSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"右侧图标";
    self.navigationItem.largeTitleDisplayMode = UINavigationItemLargeTitleDisplayModeNever;
    self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"chevron.left"] style:UIBarButtonItemStylePlain target:self action:@selector(goBack)];
    self.navigationItem.leftBarButtonItem.accessibilityLabel = @"返回";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 54;
}
- (void)goBack { [self.navigationController popViewControllerAnimated:YES]; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 2; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section ? 6 : NGPlaybackPresets().count; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return section ? @"自定义单个图标" : @"预设"; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.detailTextLabel.textColor = UIColor.secondaryLabelColor;
    if (path.section == 1) {
        NSArray *names = @[@"收藏图标", @"收藏后图标", @"评论图标", @"点赞图标", @"点赞后图标", @"分享图标"];
        NSArray *actionIndexes = @[@0, @0, @1, @2, @2, @3];
        NSArray *selectedStates = @[@NO, @YES, @NO, @NO, @YES, @NO];
        NSUInteger actionIndex = [actionIndexes[path.row] unsignedIntegerValue];
        BOOL selected = [selectedStates[path.row] boolValue];
        UIImage *preview = NGPlaybackIconPreview(actionIndex, selected, NGPlaybackCurrentPreset());
        cell.textLabel.text = names[path.row];
        BOOL custom = NGPlaybackCustomSymbol(actionIndex, selected).length || [NSUserDefaults.standardUserDefaults dataForKey:NGPlaybackCustomImageKey(actionIndex, selected)].length;
        cell.detailTextLabel.text = custom ? @"已自定义" : NGPlaybackPresetTitles()[[NGPlaybackPresets() indexOfObject:NGPlaybackCurrentPreset()]];
        cell.imageView.image = preview;
        cell.imageView.tintColor = UIColor.systemBlueColor;
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        cell.accessibilityIdentifier = [NSString stringWithFormat:@"NaiwaGuo.Playback.Custom.%ld", (long)path.row];
        return cell;
    }
    NSString *preset = NGPlaybackPresets()[path.row];
    BOOL selected = [preset isEqualToString:NGPlaybackCurrentPreset()];
    cell.textLabel.text = NGPlaybackPresetTitles()[path.row];
    cell.tintColor = UIColor.systemBlueColor;
    cell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    cell.accessibilityIdentifier = [@"NaiwaGuo.Playback.Preset." stringByAppendingString:preset];
    cell.accessibilityTraits = UIAccessibilityTraitButton | (selected ? UIAccessibilityTraitSelected : 0);
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if (path.section == 1) {
        NSArray *keys = @[@"collect.normal", @"collect.selected", @"comment.normal", @"like.normal", @"like.selected", @"share.normal"];
        NSString *suffix = keys[path.row];
        NSString *key = [@"com.naiwaguo.playback.custom." stringByAppendingString:suffix];
        NSString *imageKey = [@"com.naiwaguo.playback.customImage." stringByAppendingString:suffix];
        UIAlertController *menu = [UIAlertController alertControllerWithTitle:@"自定义图标" message:@"上传自己的图片，或使用系统 SF Symbols 名称。" preferredStyle:UIAlertControllerStyleActionSheet];
        [menu addAction:[UIAlertAction actionWithTitle:@"从照片上传" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) { NGPresentIconPhotoPicker(self, imageKey); }]];
        [menu addAction:[UIAlertAction actionWithTitle:@"输入 SF Symbols 名称" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
            UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"输入图标名称" message:@"例如 heart.fill、star.fill、hand.thumbsup.fill" preferredStyle:UIAlertControllerStyleAlert];
            [alert addTextFieldWithConfigurationHandler:^(UITextField *field) { field.text = [NSUserDefaults.standardUserDefaults stringForKey:key]; field.placeholder = @"heart.fill"; field.autocapitalizationType = UITextAutocapitalizationTypeNone; }];
            [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
            [alert addAction:[UIAlertAction actionWithTitle:@"保存" style:UIAlertActionStyleDefault handler:^(UIAlertAction *save) {
                NSString *symbol = alert.textFields.firstObject.text;
                if (symbol.length && [UIImage systemImageNamed:symbol]) { [NSUserDefaults.standardUserDefaults setObject:symbol forKey:key]; [NSUserDefaults.standardUserDefaults removeObjectForKey:imageKey]; }
                else { [NSUserDefaults.standardUserDefaults removeObjectForKey:key]; [NSUserDefaults.standardUserDefaults removeObjectForKey:imageKey]; }
                NGUpdatePlaybackGlass(); [self.tableView reloadData];
            }]];
            [self presentViewController:alert animated:YES completion:nil];
        }]];
        [menu addAction:[UIAlertAction actionWithTitle:@"恢复预设" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action) { [NSUserDefaults.standardUserDefaults removeObjectForKey:key]; [NSUserDefaults.standardUserDefaults removeObjectForKey:imageKey]; NGUpdatePlaybackGlass(); [self.tableView reloadData]; }]];
        [menu addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [self presentViewController:menu animated:YES completion:nil];
        return;
    }
    [NSUserDefaults.standardUserDefaults setObject:NGPlaybackPresets()[path.row] forKey:NGPreferencePlaybackPreset];
    NGUpdatePlaybackGlass();
    UISelectionFeedbackGenerator *feedback = [UISelectionFeedbackGenerator new]; [feedback selectionChanged];
    [UIView transitionWithView:tableView duration:UIAccessibilityIsReduceMotionEnabled() ? 0 : 0.22 options:UIViewAnimationOptionTransitionCrossDissolve | UIViewAnimationOptionAllowUserInteraction animations:^{ [tableView reloadData]; } completion:nil];
}
@end

static void NGInstallPlaybackPresetHooks(void) {
    static NSMutableSet *installed;
    if (!installed) installed = [NSMutableSet set];
    if (![installed containsObject:@"UIImageView.presets"]) {
        // Host image loaders can complete after layout or after a video cell is reused.
        for (NSString *name in @[@"setImage:", @"setHighlightedImage:"]) {
            SEL selector = NSSelectorFromString(name);
            Method method = class_getInstanceMethod(UIImageView.class, selector);
            IMP original = method_getImplementation(method);
            IMP replacement = imp_implementationWithBlock(^(UIImageView *icon, UIImage *image) {
                ((void (*)(id, SEL, id))original)(icon, selector, image);
                NGPlaybackPresetState *state = objc_getAssociatedObject(icon, &NGPlaybackPresetIconAssociation);
                if (state && !state.applying && state.action) NGApplyPlaybackPreset(state.action, state.actionIndex);
            });
            class_replaceMethod(UIImageView.class, selector, replacement, method_getTypeEncoding(method));
        }
        SEL highlighted=@selector(setHighlighted:); Method highlightMethod=class_getInstanceMethod(UIImageView.class,highlighted); IMP highlightOriginal=method_getImplementation(highlightMethod);
        class_replaceMethod(UIImageView.class,highlighted,imp_implementationWithBlock(^(UIImageView *icon,BOOL value){
            ((void(*)(id,SEL,BOOL))highlightOriginal)(icon,highlighted,value);
            NGPlaybackPresetState *state=objc_getAssociatedObject(icon,&NGPlaybackPresetIconAssociation); if (state && !state.applying && state.action) NGApplyPlaybackPreset(state.action,state.actionIndex);
        }),method_getTypeEncoding(highlightMethod));
        SEL selector = @selector(didMoveToSuperview);
        Method method = class_getInstanceMethod(UIImageView.class, selector);
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIImageView *icon) {
            ((void (*)(id, SEL))original)(icon, selector);
            NGPlaybackPresetState *old = objc_getAssociatedObject(icon, &NGPlaybackPresetIconAssociation);
            if (old && old.action != icon.superview) { NGRestorePlaybackPreset(old.action); }
            NSUInteger index = [NGPlaybackActionClassNames() indexOfObject:NGClassName(icon.superview)];
            if (index != NSNotFound) NGApplyPlaybackPreset(icon.superview, index);
        });
        if (!class_addMethod(UIImageView.class, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(UIImageView.class, selector, replacement, method_getTypeEncoding(method));
        [installed addObject:@"UIImageView.presets"];
    }
    NSArray *classes = NGPlaybackActionClassNames();
    [classes enumerateObjectsUsingBlock:^(NSString *name, NSUInteger index, BOOL *stop) {
        Class target = NSClassFromString(name);
        if (!target || [installed containsObject:name]) return;
        SEL selector = @selector(layoutSubviews);
        Method method = class_getInstanceMethod(target, selector);
        if (!method) return;
        IMP original = method_getImplementation(method);
        IMP replacement = imp_implementationWithBlock(^(UIView *action) {
            ((void (*)(id, SEL))original)(action, selector);
            NGApplyPlaybackPreset(action, index);
            NGRefreshPlaybackActionAppearance(action);
        });
        if (!class_addMethod(target, selector, replacement, method_getTypeEncoding(method))) class_replaceMethod(target, selector, replacement, method_getTypeEncoding(method));
        for (NSString *setter in @[@"setModel:",@"refreshUIWithModel:"]) {
            SEL sel=NSSelectorFromString(setter); Method m=class_getInstanceMethod(target,sel); if (!m || method_getNumberOfArguments(m)!=3) continue; IMP prior=method_getImplementation(m);
            class_replaceMethod(target,sel,imp_implementationWithBlock(^(UIView *action,id model){ ((void(*)(id,SEL,id))prior)(action,sel,model); NGApplyPlaybackPreset(action,index); }),method_getTypeEncoding(m));
        }
        SEL refresh=NSSelectorFromString(@"refreshUIWithModel:animated:"); Method refreshMethod=class_getInstanceMethod(target,refresh);
        if (refreshMethod && method_getNumberOfArguments(refreshMethod)==4) { IMP prior=method_getImplementation(refreshMethod); class_replaceMethod(target,refresh,imp_implementationWithBlock(^(UIView *action,id model,BOOL animated){ ((void(*)(id,SEL,id,BOOL))prior)(action,refresh,model,animated); NGApplyPlaybackPreset(action,index); }),method_getTypeEncoding(refreshMethod)); }
        SEL collected=NSSelectorFromString(@"setIsCollected:"); Method collectMethod=class_getInstanceMethod(target,collected);
        if (collectMethod) { IMP prior=method_getImplementation(collectMethod); class_replaceMethod(target,collected,imp_implementationWithBlock(^(UIView *action,BOOL value){ ((void(*)(id,SEL,BOOL))prior)(action,collected,value); NGApplyPlaybackPreset(action,index); }),method_getTypeEncoding(collectMethod)); }
        [installed addObject:name];
    }];
    for (NSString *modelName in @[@"SSShortVideoRightInteractionLikeModel",@"SSShortVideoRightInteractionCollectModel"]) {
        Class model=NSClassFromString(modelName); if (!model || [installed containsObject:modelName]) continue;
        SEL selector=NSSelectorFromString([modelName containsString:@"LikeModel"] ? @"setIsLiked:" : @"setIsCollected:"); Method method=class_getInstanceMethod(model,selector); if (!method) continue; IMP prior=method_getImplementation(method);
        class_replaceMethod(model,selector,imp_implementationWithBlock(^(id object,BOOL selected){
            ((void(*)(id,SEL,BOOL))prior)(object,selector,selected);
            void (^refresh)(void)=^{ for (UIView *action in [NGPlaybackModelActions objectForKey:object].allObjects) { NGPlaybackPresetState *state=objc_getAssociatedObject(action,&NGPlaybackPresetAssociation); if (state && state.model==object) NGApplyPlaybackPreset(action,state.actionIndex); } };
            if (NSThread.isMainThread) refresh(); else dispatch_async(dispatch_get_main_queue(),refresh);
        }),method_getTypeEncoding(method)); [installed addObject:modelName];
    }

}
