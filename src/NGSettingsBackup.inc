static NSError *NGBackupError(NSString *message) { return [NSError errorWithDomain:@"NaiwaGuo.Backup" code:1 userInfo:@{NSLocalizedDescriptionKey:message}]; }
static BOOL NGBackupKey(NSString *key) {
    if (![key isKindOfClass:NSString.class] || key.length>150) return NO;
    if ([@[NGPreferenceHUD,NGPreferencePerformance] containsObject:key]) return YES;
    return [key rangeOfString:@"^com\\.naiwaguo\\.(appearance|playback|typography|prompts)\\.[A-Za-z0-9_.-]+$" options:NSRegularExpressionSearch].location!=NSNotFound;
}
static BOOL NGBackupValue(NSString *key,id value) {
    if (!NGBackupKey(key)) return NO;
    if ([key containsString:@"customImage."] || [key containsString:@"dockIconImage."]) { if(![value isKindOfClass:NSData.class] || [value length]>5*1024*1024)return NO;UIImage *image=[UIImage imageWithData:value];return image && image.size.width>0 && image.size.height>0 && image.size.width*image.scale<=4096 && image.size.height*image.scale<=4096; }
    if ([key isEqual:NGPreferenceHiddenDockItems]) { if(![value isKindOfClass:NSArray.class] || [value count]>4)return NO;for(id item in value)if(![NGDockItemKeys() containsObject:item])return NO;return YES; }
    if ([key hasPrefix:@"com.naiwaguo.appearance.dockColor."]) { if(![value isKindOfClass:NSArray.class] || [value count]!=4)return NO;for(id item in value)if(![item isKindOfClass:NSNumber.class] || !isfinite([item doubleValue]) || [item doubleValue]<0 || [item doubleValue]>1)return NO;return YES; }
    if ([key containsString:@".custom."] || [key containsString:@".dockIcon."] || [@[NGPreferenceDockIconStyle,NGPreferencePlaybackPreset,NGPreferenceFontName,NGPreferenceFontFile] containsObject:key]) {
        if(![value isKindOfClass:NSString.class] || [value length]>256)return NO;
        if([key isEqual:NGPreferenceFontFile])return [[value lastPathComponent] isEqual:value] && ![value containsString:@".."];
        return YES;
    }
    return [value isKindOfClass:NSNumber.class] && isfinite([value doubleValue]) && fabs([value doubleValue])<=100000;
}
static NSData *NGCreateSettingsBackup(NSError **error) {
    NSDictionary *domain=[NSUserDefaults.standardUserDefaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier] ?: @{};
    NSMutableDictionary *preferences=[NSMutableDictionary dictionary];
    for(NSString *key in domain)if(NGBackupValue(key,domain[key]))preferences[key]=domain[key];
    NSMutableDictionary *archive=[@{@"format":@"naiwaguo-settings",@"schema":@1,@"pluginVersion":NGVersion,@"createdAt":NSDate.date,@"preferences":preferences} mutableCopy];
    NSString *font=preferences[NGPreferenceFontFile];
    if(font.length){ NSURL *url=[NGTypographyDirectory() URLByAppendingPathComponent:font];NSNumber *size=nil;[url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];if(size && size.unsignedLongLongValue<=50*1024*1024){NSData *data=[NSData dataWithContentsOfURL:url options:0 error:error];if(!data)return nil;archive[@"font"]=@{@"extension":url.pathExtension.lowercaseString,@"data":data};}else{if(error)*error=NGBackupError(@"当前字体文件不可用，请先恢复系统字体或重新导入后备份。");return nil;} }
    NSData *result=[NSPropertyListSerialization dataWithPropertyList:archive format:NSPropertyListBinaryFormat_v1_0 options:0 error:error];
    if(result.length>64*1024*1024){if(error)*error=NGBackupError(@"备份超过 64 MB，请减少自定义图片。");return nil;}return result;
}
static NSDictionary *NGReadSettingsBackup(NSData *data,NSError **error) {
    if(!data.length || data.length>64*1024*1024){if(error)*error=NGBackupError(@"备份为空或超过 64 MB。");return nil;}
    id archive=[NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nil error:error];
    if(![archive isKindOfClass:NSDictionary.class] || ![archive[@"format"] isEqual:@"naiwaguo-settings"] || ![archive[@"schema"] isEqual:@1] || ![archive[@"preferences"] isKindOfClass:NSDictionary.class] || [archive[@"preferences"] count]>250){if(error)*error=NGBackupError(@"不是支持的奶蛙设置备份。");return nil;}
    for(id key in archive[@"preferences"])if(!NGBackupValue(key,archive[@"preferences"][key])){if(error)*error=NGBackupError(@"备份包含无效设置，未作任何更改。");return nil;}
    id font=archive[@"font"];
    if(font && (![font isKindOfClass:NSDictionary.class] || ![@[@"ttf",@"otf",@"ttc"] containsObject:font[@"extension"]] || ![font[@"data"] isKindOfClass:NSData.class] || ![font[@"data"] length] || [font[@"data"] length]>50*1024*1024)){if(error)*error=NGBackupError(@"字体备份无效。");return nil;}
    if([archive[@"preferences"][NGPreferenceFontFile] length] && !font){if(error)*error=NGBackupError(@"备份缺少自定义字体文件。");return nil;}
    return archive;
}
static BOOL NGRestoreSettingsBackup(NSDictionary *archive,NSError **error) {
    NSMutableDictionary *preferences=[archive[@"preferences"] mutableCopy];
    id font=archive[@"font"];
    if(font){
        NSURL *directory=NGTypographyDirectory();if(![NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:error])return NO;
        NSURL *url=[directory URLByAppendingPathComponent:[NSString stringWithFormat:@"%@.%@",NSUUID.UUID.UUIDString,font[@"extension"]]];
        if(![font[@"data"] writeToURL:url options:NSDataWritingAtomic error:error])return NO;
        NSArray *descriptors=CFBridgingRelease(CTFontManagerCreateFontDescriptorsFromURL((__bridge CFURLRef)url));
        if(!descriptors.count || !NGTypographyRegisterURL(url,error)){[NSFileManager.defaultManager removeItemAtURL:url error:nil];if(error && !*error)*error=NGBackupError(@"字体不能加载，设置未更改。");return NO;}
        NSString *name=CFBridgingRelease(CTFontDescriptorCopyAttribute((__bridge CTFontDescriptorRef)descriptors.firstObject,kCTFontNameAttribute));
        preferences[NGPreferenceFontFile]=url.lastPathComponent;if(name)preferences[NGPreferenceFontName]=name;
    }
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    NSMutableDictionary *domain=[[defaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier] ?: @{} mutableCopy];
    for(NSString *key in domain.allKeys)if(NGBackupKey(key))[domain removeObjectForKey:key];
    [domain addEntriesFromDictionary:preferences]; [defaults setPersistentDomain:domain forName:NSBundle.mainBundle.bundleIdentifier];
    return YES;
}
static void NGApplyRestoredSettings(void) {
    NGLoadTypographyFont(); NGRefreshTypography(); NGUpdateDock(); NGUpdatePlaybackGlass(); NGUpdatePlaybackHeaderGlass(); NGUpdateCommentGlass(); NGUpdatePlaybackFullscreen(); NGUpdatePlaybackTextLayout(); NGRefreshPlaybackCorners(); NGSetPerformanceEnabled([NSUserDefaults.standardUserDefaults boolForKey:NGPreferencePerformance]); NGInstallOverlay(); NGDAudioProbePreferenceDidChange();
}

@interface NGSettingsBackupController : NGSettingsTableController <UIDocumentPickerDelegate>
@property(nonatomic,strong) NSURL *temporaryExport;
@end
@implementation NGSettingsBackupController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title=@"备份与恢复设置"; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 2; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section { return @"备份包含奶蛙设置、自定义图标与当前导入字体，保存为 .naiwaguo 文件。恢复会替换奶蛙设置，不修改红果账号和其他应用数据。统计凭据、安装编号与账号 ID 不写入备份。"; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];cell.textLabel.text=path.row ? @"从文件恢复" : @"导出设置备份";cell.imageView.image=[UIImage systemImageNamed:path.row ? @"square.and.arrow.down" : @"square.and.arrow.up"];cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;return cell;
}
- (void)showError:(NSError *)error { UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"备份与恢复" message:error.localizedDescription preferredStyle:UIAlertControllerStyleAlert];[alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];[self presentViewController:alert animated:YES completion:nil]; }
- (void)removeTemporaryExport { if(self.temporaryExport)[NSFileManager.defaultManager removeItemAtURL:self.temporaryExport.URLByDeletingLastPathComponent error:nil];self.temporaryExport=nil; }
- (void)dealloc { [self removeTemporaryExport]; }
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if(path.row){UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:YES];picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];return;}
    self.tableView.userInteractionEnabled=NO;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        NSError *error=nil;NSData *data=NGCreateSettingsBackup(&error);NSURL *directory=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString] isDirectory:YES];NSURL *file=[directory URLByAppendingPathComponent:@"奶蛙设置.naiwaguo"];
        BOOL ready=data && [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:&error] && [data writeToURL:file options:NSDataWritingAtomic error:&error];
        dispatch_async(dispatch_get_main_queue(),^{self.tableView.userInteractionEnabled=YES;if(!ready){[NSFileManager.defaultManager removeItemAtURL:directory error:nil];[self showError:error ?: NGBackupError(@"无法创建备份。")];return;}[self removeTemporaryExport];self.temporaryExport=file;UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForExportingURLs:@[file] asCopy:YES];picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];});
    });
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller { [self removeTemporaryExport]; }
- (void)restoreArchive:(NSDictionary *)archive {
    self.tableView.userInteractionEnabled=NO;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        NSError *failure=nil;BOOL restored=NGRestoreSettingsBackup(archive,&failure);
        dispatch_async(dispatch_get_main_queue(),^{self.tableView.userInteractionEnabled=YES;if(restored)NGApplyRestoredSettings();[self showError:restored ? NGBackupError(@"设置已恢复。字体或调试采集选项可能需要重新打开红果生效。") : failure ?: NGBackupError(@"恢复失败，原设置已保留。")];});
    });
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    if(self.temporaryExport){[self removeTemporaryExport];return;} NSURL *url=urls.firstObject;if(!url)return;
    self.tableView.userInteractionEnabled=NO;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        BOOL access=[url startAccessingSecurityScopedResource];NSError *error=nil;NSNumber *size=nil;[url getResourceValue:&size forKey:NSURLFileSizeKey error:&error];NSData *data=(size && size.unsignedLongLongValue<=64*1024*1024) ? [NSData dataWithContentsOfURL:url options:NSDataReadingMappedIfSafe error:&error] : nil;NSDictionary *archive=NGReadSettingsBackup(data,&error);if(access)[url stopAccessingSecurityScopedResource];
        dispatch_async(dispatch_get_main_queue(),^{
            self.tableView.userInteractionEnabled=YES;
            void (^present)(void)=^{if(!archive){[self showError:error ?: NGBackupError(@"无法读取备份。")];return;}UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"恢复设置" message:@"这会替换当前奶蛙设置。建议先导出现有设置备份，确认继续？" preferredStyle:UIAlertControllerStyleAlert];[alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[alert addAction:[UIAlertAction actionWithTitle:@"恢复" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action){[self restoreArchive:archive];}]];[self presentViewController:alert animated:YES completion:nil];};
            if(controller.presentingViewController)[controller dismissViewControllerAnimated:YES completion:present];else present();
        });
    });
}
@end
