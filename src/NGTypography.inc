// App-process text customization. Native UILabel/UITextView/UITextField and
// the host's YYLabel are supported. Video pixels and custom canvas text are not rewritten.
static NSString *const NGPreferenceFontScale=@"com.naiwaguo.typography.scale";
static NSString *const NGPreferenceFontName=@"com.naiwaguo.typography.fontName";
static NSString *const NGPreferenceFontFile=@"com.naiwaguo.typography.fontFile";
static char NGTypographyStateKey;
static NSUInteger NGTypographyGeneration=1;
static BOOL NGTypographyRefreshPending;
static NSHashTable<UIView *> *NGTypographyViews;
static CGFloat NGTypographyCachedScale=1;
static NSString *NGTypographyCachedName;
static BOOL NGTypographyConfigured;
static NSCache<UIFont *,UIFont *> *NGTypographyFontCache;
static CGFloat NGTypographyScale(void) {
    id raw=[NSUserDefaults.standardUserDefaults objectForKey:NGPreferenceFontScale]; CGFloat scale=raw ? [raw doubleValue] : 1;
    return isfinite(scale) ? MIN(1,MAX(0.5,scale)) : 1;
}
static NSString *NGTypographyName(void) { return [NSUserDefaults.standardUserDefaults stringForKey:NGPreferenceFontName] ?: @""; }
static BOOL NGTypographyEnabled(void) { return fabs(NGTypographyScale()-1)>0.001 || NGTypographyName().length>0; }
@interface NGTypographyState : NSObject
@property(nonatomic,strong) UIFont *sourceFont;
@property(nonatomic,strong) UIFont *lastFont;
@property(nonatomic,strong) NSAttributedString *sourceText;
@property(nonatomic,strong) NSAttributedString *lastText;
@property(nonatomic,strong) NSAttributedString *sourcePlaceholder;
@property(nonatomic,strong) NSAttributedString *lastPlaceholder;
@property(nonatomic) BOOL writing;
@property(nonatomic) NSUInteger generation;
@end
@implementation NGTypographyState @end
static id NGTypographyGet(id target,SEL selector) { return [target respondsToSelector:selector] ? ((id(*)(id,SEL))[target methodForSelector:selector])(target,selector) : nil; }
static void NGTypographySet(id target,SEL selector,id value) { if ([target respondsToSelector:selector]) ((void(*)(id,SEL,id))[target methodForSelector:selector])(target,selector,value); }
static BOOL NGTypographyLabel(UIView *view) { return [view isKindOfClass:UILabel.class] || [view isKindOfClass:UITextView.class] || [view isKindOfClass:UITextField.class] || [NGClassName(view) isEqualToString:@"YYLabel"]; }
static BOOL NGTypographyIconFont(UIFont *font) {
    NSString *name=font.fontName.lowercaseString;
    return [name containsString:@"icon"] || [name containsString:@"emoji"] || [name containsString:@"fontawesome"] || [name containsString:@"symbol"];
}
static UIFont *NGTypographyTransformFont(UIFont *font) {
    if (![font isKindOfClass:UIFont.class] || NGTypographyIconFont(font)) return font;
    UIFont *cached=[NGTypographyFontCache objectForKey:font]; if (cached) return cached;
    CGFloat size=MAX(5,font.pointSize*NGTypographyCachedScale); NSString *name=NGTypographyCachedName;
    UIFont *result=name.length ? [UIFont fontWithName:name size:size] : nil;
    if (!result) result=[font fontWithSize:size];
    UIFontDescriptorSymbolicTraits traits=font.fontDescriptor.symbolicTraits & (UIFontDescriptorTraitBold|UIFontDescriptorTraitItalic);
    if (traits) { UIFontDescriptor *descriptor=[result.fontDescriptor fontDescriptorWithSymbolicTraits:result.fontDescriptor.symbolicTraits|traits]; if (descriptor) result=[UIFont fontWithDescriptor:descriptor size:size]; }
    if (result && NGTypographyFontCache) [NGTypographyFontCache setObject:result forKey:font];
    return result;
}
static NSAttributedString *NGTypographyTransformText(NSAttributedString *text) {
    if (!text.length) return text;
    NSMutableAttributedString *result=[text mutableCopy];
    [text enumerateAttribute:NSFontAttributeName inRange:NSMakeRange(0,text.length) options:0 usingBlock:^(id value,NSRange range,BOOL *stop){
        if ([value isKindOfClass:UIFont.class]) [result addAttribute:NSFontAttributeName value:NGTypographyTransformFont(value) range:range];
    }];
    return result;
}
static BOOL NGTypographyNestedTextRenderer(UIView *view) {
    // A UITextField/UITextView owns its internal labels. Transform the public
    // control once, not its already transformed rendering children a second time.
    for (UIView *parent=view.superview;parent;parent=parent.superview) {
        if (NGTypographyLabel(parent)) return YES;
        if ([parent isKindOfClass:UIWindow.class]) break;
    }
    return NO;
}
static NGTypographyState *NGTypographyRemember(UIView *view) {
    NGTypographyState *state=objc_getAssociatedObject(view,&NGTypographyStateKey);
    if (state) return state;
    state=[NGTypographyState new]; state.writing=YES;
    // Install the guard before UIKit getters/setters can re-enter another hook.
    objc_setAssociatedObject(view,&NGTypographyStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    state.sourceFont=NGTypographyGet(view,@selector(font));
    state.sourceText=[NGTypographyGet(view,@selector(attributedText)) copy];
    state.sourcePlaceholder=[NGTypographyGet(view,@selector(attributedPlaceholder)) copy];
    state.writing=NO;
    if (!NGTypographyViews) NGTypographyViews=[NSHashTable weakObjectsHashTable]; [NGTypographyViews addObject:view];
    return state;
}
static void NGApplyTypography(UIView *view) {
    if (!NGTypographyLabel(view)) return;
    NGTypographyState *state=objc_getAssociatedObject(view,&NGTypographyStateKey); if (state.writing) return;
    BOOL active=NGTypographyConfigured && view.window && view.window.windowLevel==UIWindowLevelNormal && !NGTypographyNestedTextRenderer(view);
    if (!active && !state) return;
    if (!state) state=NGTypographyRemember(view);
    if (active && state.generation==NGTypographyGeneration) return;
    // Do not compare attributed-string pointer identities: some native labels
    // return a fresh equal copy on every read. Rewriting those during layout
    // causes another layout forever. Only setters and attachment invalidate us.
    UIFont *font=active ? NGTypographyTransformFont(state.sourceFont) : state.sourceFont;
    NSAttributedString *text=active ? NGTypographyTransformText(state.sourceText) : state.sourceText;
    NSAttributedString *placeholder=active ? NGTypographyTransformText(state.sourcePlaceholder) : state.sourcePlaceholder;
    state.writing=YES;
    @try {
        if (font && ![font isEqual:NGTypographyGet(view,@selector(font))]) NGTypographySet(view,@selector(setFont:),font);
        NSAttributedString *current=NGTypographyGet(view,@selector(attributedText));
        if (text && ![text isEqualToAttributedString:current]) NGTypographySet(view,@selector(setAttributedText:),text);
        NSAttributedString *currentPlaceholder=NGTypographyGet(view,@selector(attributedPlaceholder));
        if (placeholder && ![placeholder isEqualToAttributedString:currentPlaceholder]) NGTypographySet(view,@selector(setAttributedPlaceholder:),placeholder);
        state.lastFont=NGTypographyGet(view,@selector(font)); state.lastText=[NGTypographyGet(view,@selector(attributedText)) copy]; state.lastPlaceholder=[NGTypographyGet(view,@selector(attributedPlaceholder)) copy]; state.generation=NGTypographyGeneration;
    } @finally { state.writing=NO; }
    if (!active) { objc_setAssociatedObject(view,&NGTypographyStateKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC); [NGTypographyViews removeObject:view]; }
}
static void NGRefreshTypography(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(),^{ NGRefreshTypography(); }); return; }
    NGTypographyCachedScale=NGTypographyScale(); NGTypographyCachedName=[NGTypographyName() copy];
    NGTypographyConfigured=fabs(NGTypographyCachedScale-1)>0.001 || NGTypographyCachedName.length>0;
    if (!NGTypographyFontCache) { NGTypographyFontCache=[NSCache new]; NGTypographyFontCache.countLimit=256; }
    [NGTypographyFontCache removeAllObjects]; NGTypographyGeneration++;
    for (UIView *view in NGTypographyViews.allObjects) NGApplyTypography(view);
    if (!NGTypographyEnabled()) return;
    NSMutableArray *queue=[NSMutableArray array]; for (UIWindow *window in NGCaptureWindows()) if (window.windowLevel==UIWindowLevelNormal) [queue addObject:window];
    for (NSUInteger i=0;i<queue.count && i<18000;i++) { UIView *view=queue[i]; if (NGTypographyLabel(view)) NGApplyTypography(view); [queue addObjectsFromArray:view.subviews]; }
}
static void NGScheduleTypographyRefresh(void) {
    if (NGTypographyRefreshPending) return; NGTypographyRefreshPending=YES;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.12*NSEC_PER_SEC),dispatch_get_main_queue(),^{ NGTypographyRefreshPending=NO; NGRefreshTypography(); });
}
static NSURL *NGTypographyDirectory(void) {
    NSURL *base=[NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
    return [base URLByAppendingPathComponent:@"NaiwaGuo/Fonts" isDirectory:YES];
}
static BOOL NGTypographyRegisterURL(NSURL *url,NSError **error) {
    CFErrorRef failure=nil; BOOL result=CTFontManagerRegisterFontsForURL((__bridge CFURLRef)url,kCTFontManagerScopeProcess,&failure);
    if (!result && failure && CFErrorGetCode(failure)==kCTFontManagerErrorAlreadyRegistered) result=YES;
    if (!result && failure && CFErrorGetCode(failure)==kCTFontManagerErrorDuplicatedName) {
        NSArray *descriptors=CFBridgingRelease(CTFontManagerCreateFontDescriptorsFromURL((__bridge CFURLRef)url));
        NSString *name=descriptors.count ? CFBridgingRelease(CTFontDescriptorCopyAttribute((__bridge CTFontDescriptorRef)descriptors.firstObject,kCTFontNameAttribute)) : nil;
        if (name.length && [UIFont fontWithName:name size:16]) {
            CTFontRef font=CTFontCreateWithName((__bridge CFStringRef)name,16,NULL); NSURL *registered=CFBridgingRelease(CTFontCopyAttribute(font,kCTFontURLAttribute)); CFRelease(font);
            NSData *existing=registered ? [NSData dataWithContentsOfURL:registered] : nil;
            result=existing && [existing isEqualToData:[NSData dataWithContentsOfURL:url]];
        }
    }
    if (failure) { if (!result && error) *error=CFBridgingRelease(failure); else CFRelease(failure); }
    return result;
}
static BOOL NGImportTypographyFont(NSURL *url,NSError **outError) {
    BOOL access=[url startAccessingSecurityScopedResource];
    @try {
        NSNumber *size=nil; [url getResourceValue:&size forKey:NSURLFileSizeKey error:outError];
        if (!size || size.unsignedLongLongValue>50*1024*1024) { if (outError) *outError=[NSError errorWithDomain:@"NaiwaGuo.Font" code:1 userInfo:@{NSLocalizedDescriptionKey:@"请选择不超过 50 MB 的字体文件。"}]; return NO; }
        NSString *ext=url.pathExtension.lowercaseString;
        if (![@[@"ttf",@"otf",@"ttc"] containsObject:ext]) { if (outError) *outError=[NSError errorWithDomain:@"NaiwaGuo.Font" code:2 userInfo:@{NSLocalizedDescriptionKey:@"支持 TTF、OTF 和 TTC 字体文件。"}]; return NO; }
        NSURL *directory=NGTypographyDirectory(); [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:outError];
        NSURL *destination=[directory URLByAppendingPathComponent:[[NSUUID.UUID.UUIDString stringByAppendingString:@"."] stringByAppendingString:ext]];
        if (![NSFileManager.defaultManager copyItemAtURL:url toURL:destination error:outError]) return NO;
        NSArray *descriptors=CFBridgingRelease(CTFontManagerCreateFontDescriptorsFromURL((__bridge CFURLRef)destination));
        CTFontDescriptorRef descriptor=descriptors.count ? (__bridge CTFontDescriptorRef)descriptors.firstObject : nil;
        NSString *name=descriptor ? CFBridgingRelease(CTFontDescriptorCopyAttribute(descriptor,kCTFontNameAttribute)) : nil;
        NSError *error=nil;
        if (!name.length || !NGTypographyRegisterURL(destination,&error) || ![UIFont fontWithName:name size:16]) {
            [NSFileManager.defaultManager removeItemAtURL:destination error:nil];
            if (outError) *outError=error ?: [NSError errorWithDomain:@"NaiwaGuo.Font" code:3 userInfo:@{NSLocalizedDescriptionKey:@"无法读取这个字体文件，请检查文件是否完整。"}]; return NO;
        }
        [NSUserDefaults.standardUserDefaults setObject:destination.lastPathComponent forKey:NGPreferenceFontFile];
        [NSUserDefaults.standardUserDefaults setObject:name forKey:NGPreferenceFontName]; NGRefreshTypography(); return YES;
    } @finally { if (access) [url stopAccessingSecurityScopedResource]; }
}
static void NGLoadTypographyFont(void) {
    NSArray<NSURL *> *files=[NSFileManager.defaultManager contentsOfDirectoryAtURL:NGTypographyDirectory() includingPropertiesForKeys:@[NSURLFileSizeKey] options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
    for (NSURL *url in files) {
        if (![@[@"ttf",@"otf",@"ttc"] containsObject:url.pathExtension.lowercaseString]) continue;
        NSNumber *size=nil; [url getResourceValue:&size forKey:NSURLFileSizeKey error:nil]; if (size.unsignedLongLongValue>50*1024*1024) continue;
        NGTypographyRegisterURL(url,nil);
    }
}
static void NGInstallTypographyHooks(void) {
    static NSMutableSet *installed; if (!installed) installed=[NSMutableSet set];
    for (NSString *name in @[@"UILabel",@"UITextView",@"UITextField",@"YYLabel"]) {
        Class cls=NSClassFromString(name); if (!cls || [installed containsObject:name]) continue;
        for (NSString *setter in @[@"setFont:",@"setAttributedText:",@"setAttributedPlaceholder:",@"setText:"]) {
            SEL selector=NSSelectorFromString(setter); Method method=class_getInstanceMethod(cls,selector); if (!method) continue; IMP original=method_getImplementation(method);
            class_replaceMethod(cls,selector,imp_implementationWithBlock(^(UIView *view,id value){
                NGTypographyState *state=objc_getAssociatedObject(view,&NGTypographyStateKey);
                if (state.writing || !NSThread.isMainThread || (!state && !NGTypographyConfigured)) { ((void(*)(id,SEL,id))original)(view,selector,value); return; }
                if (!state) state=NGTypographyRemember(view);
                if ([setter isEqualToString:@"setFont:"]) { if (![value isEqual:state.lastFont]) state.sourceFont=value; }
                else if ([setter isEqualToString:@"setAttributedText:"]) { if (value!=state.lastText && ![value isEqualToAttributedString:state.lastText]) state.sourceText=[value copy]; }
                else if ([setter isEqualToString:@"setAttributedPlaceholder:"]) { if (value!=state.lastPlaceholder && ![value isEqualToAttributedString:state.lastPlaceholder]) state.sourcePlaceholder=[value copy]; }
                else state.sourceText=nil;
                state.generation=0; state.writing=YES;
                @try { ((void(*)(id,SEL,id))original)(view,selector,value); } @finally { state.writing=NO; }
                NGApplyTypography(view);
            }),method_getTypeEncoding(method));
        }
        // Never write typography from layoutSubviews: table auto-sizing and
        // profile/YYLabel layout must be able to settle without invalidation.
        SEL selector=@selector(didMoveToWindow); Method method=class_getInstanceMethod(cls,selector); IMP original=method_getImplementation(method);
        class_replaceMethod(cls,selector,imp_implementationWithBlock(^(UIView *view){
            ((void(*)(id,SEL))original)(view,selector);
            NGTypographyState *state=objc_getAssociatedObject(view,&NGTypographyStateKey);
            if (state && !state.writing) state.generation=0;
            if (NGTypographyConfigured || state) NGApplyTypography(view);
        }),method_getTypeEncoding(method));
        [installed addObject:name];
    }
}

@interface NGTypographyController : NGSettingsTableController <UIFontPickerViewControllerDelegate,UIDocumentPickerDelegate>
@property(nonatomic,strong) UILabel *sample;
@property(nonatomic,strong) UILabel *percentage;
@end
@implementation NGTypographyController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title=@"字体与字号"; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 3; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section==2 ? 4 : 1; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return @[@"预览",@"全局字号",@"字体"][section]; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section==1) return @"应用内原生文字按原字号的 50%–100% 缩放，不改变系统设置。";
    if (section==2) return @"支持 TTF / OTF / TTC。缺失的汉字由系统回退显示。视频字幕、网页和自绘画布文字不在此范围内。";
    return nil;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path { return path.section==0 ? 78 : (path.section==1 ? 76 : UITableViewAutomaticDimension); }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    if (path.section==0) {
        cell.textLabel.text=@"每一段故事，都值得细细看。"; cell.detailTextLabel.text=@"奶蛙 · Aa 0123456789"; cell.textLabel.font=[UIFont systemFontOfSize:18]; self.sample=cell.textLabel;
    } else if (path.section==1) {
        UILabel *value=[UILabel new]; value.text=[NSString stringWithFormat:@"%.0f%%",NGTypographyScale()*100]; value.font=[UIFont monospacedDigitSystemFontOfSize:14 weight:UIFontWeightMedium]; self.percentage=value;
        UISlider *slider=[UISlider new]; slider.minimumValue=0.5; slider.maximumValue=1; slider.value=NGTypographyScale(); slider.accessibilityLabel=@"全局字号比例"; slider.accessibilityIdentifier=NGPreferenceFontScale; [slider addTarget:self action:@selector(scaleChanged:) forControlEvents:UIControlEventValueChanged];
        for (UIView *view in @[value,slider]) { view.translatesAutoresizingMaskIntoConstraints=NO; [cell.contentView addSubview:view]; }
        [NSLayoutConstraint activateConstraints:@[[value.leadingAnchor constraintEqualToAnchor:cell.contentView.leadingAnchor constant:16],[value.centerYAnchor constraintEqualToAnchor:cell.contentView.centerYAnchor],[value.widthAnchor constraintEqualToConstant:52],[slider.leadingAnchor constraintEqualToAnchor:value.trailingAnchor constant:10],[slider.trailingAnchor constraintEqualToAnchor:cell.contentView.trailingAnchor constant:-16],[slider.centerYAnchor constraintEqualToAnchor:value.centerYAnchor]]];
    } else {
        cell.textLabel.text=@[@"跟随原字体",@"选择已安装字体",@"导入字体文件",@"恢复默认字体与字号"][path.row];
        cell.detailTextLabel.text=path.row==1 && NGTypographyName().length ? NGTypographyName() : (path.row==2 ? @"从“文件”中选择字体" : @"");
        cell.accessoryType=path.row==0 ? (NGTypographyName().length ? UITableViewCellAccessoryNone : UITableViewCellAccessoryCheckmark) : UITableViewCellAccessoryDisclosureIndicator;
    }
    return cell;
}
- (void)scaleChanged:(UISlider *)slider { CGFloat value=round(slider.value*100)/100; [NSUserDefaults.standardUserDefaults setDouble:value forKey:NGPreferenceFontScale]; self.percentage.text=[NSString stringWithFormat:@"%.0f%%",value*100]; NGScheduleTypographyRefresh(); }
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES]; if (path.section!=2) return;
    if (path.row==0 || path.row==3) { [NSUserDefaults.standardUserDefaults removeObjectForKey:NGPreferenceFontName]; if (path.row==3) [NSUserDefaults.standardUserDefaults removeObjectForKey:NGPreferenceFontScale]; NGRefreshTypography(); [self.tableView reloadData]; }
    else if (path.row==1) { UIFontPickerViewControllerConfiguration *configuration=[UIFontPickerViewControllerConfiguration new]; configuration.includeFaces=YES; UIFontPickerViewController *picker=[[UIFontPickerViewController alloc] initWithConfiguration:configuration]; picker.delegate=self; [self presentViewController:picker animated:YES completion:nil]; }
    else { UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeFont] asCopy:YES]; picker.delegate=self; picker.allowsMultipleSelection=NO; [self presentViewController:picker animated:YES completion:nil]; }
}
- (void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)picker {
    NSString *name=(picker.selectedFontDescriptor ? [UIFont fontWithDescriptor:picker.selectedFontDescriptor size:16].fontName : nil); if (name.length) [NSUserDefaults.standardUserDefaults setObject:name forKey:NGPreferenceFontName];
    [picker dismissViewControllerAnimated:YES completion:^{ NGRefreshTypography(); [self.tableView reloadData]; }];
}
- (void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)picker { [picker dismissViewControllerAnimated:YES completion:nil]; }
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    if (!urls.count) return; NSError *error=nil; BOOL imported=NGImportTypographyFont(urls.firstObject,&error);
    void (^finish)(void)=^{
        if (imported) { [self.tableView reloadData]; return; }
        NSString *message=error.localizedDescription ?: @"无法读取这个字体文件。";
        if ([error.domain isEqualToString:(__bridge NSString *)kCTFontManagerErrorDomain] && error.code==kCTFontManagerErrorDuplicatedName) message=@"已有同名字体且内容不同，请改选已安装字体，或使用不同名称的字体文件。";
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"字体导入失败" message:message preferredStyle:UIAlertControllerStyleAlert]; [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
        [self presentViewController:alert animated:YES completion:nil];
    };
    if (controller.presentingViewController) [controller dismissViewControllerAnimated:YES completion:finish]; else finish();
}

@end
