static NSString *const NGUsageInstallation=@"com.naiwaguo.service.installation";
static NSString *const NGUsageSecret=@"com.naiwaguo.service.installationSecret";
static NSString *const NGUsageLastSent=@"com.naiwaguo.service.lastSent";
static NSString *const NGUsageNoticeShown=@"com.naiwaguo.service.noticeShown";
static NSString *const NGUsageOrigin=@"https://naiwa.xxdou.icu";
static BOOL NGUsageBusy;
static NSString *NGUsageLastIdentity;
#if NG_TEST
static BOOL NGUsageTestNetworking;
static BOOL NGUsageTestNotices;
#endif

static NSString *NGHongguoUserID(void) {
    Class centerClass=NSClassFromString(@"HTSServiceCenter"),serviceClass=NSClassFromString(@"SSUserInfoService");
    SEL centerSelector=NSSelectorFromString(@"defaultCenter"),serviceSelector=NSSelectorFromString(@"getService:"),logged=NSSelectorFromString(@"isLogin");
    if(serviceClass && [centerClass respondsToSelector:centerSelector]) {
        id center=((id(*)(id,SEL))[centerClass methodForSelector:centerSelector])(centerClass,centerSelector);
        id service=[center respondsToSelector:serviceSelector] ? ((id(*)(id,SEL,Class))[center methodForSelector:serviceSelector])(center,serviceSelector,serviceClass) : nil;
        if([service respondsToSelector:logged]) {
            if(!((BOOL(*)(id,SEL))[service methodForSelector:logged])(service,logged))return @"";
            id user=NGObjectProperty(service,@"curUser"),value=NGObjectProperty(user,@"userId");
            if([value isKindOfClass:NSNumber.class])value=[value stringValue];
            if([value isKindOfClass:NSString.class] && [value rangeOfString:@"^[0-9]{1,20}$" options:NSRegularExpressionSearch].location!=NSNotFound)return value;
        }
    }
    Class account=NSClassFromString(@"TTAccount"); SEL shared=NSSelectorFromString(@"sharedAccount");
    if (![account respondsToSelector:shared]) return @"";
    id current=((id(*)(id,SEL))[account methodForSelector:shared])(account,shared); SEL identifier=NSSelectorFromString(@"userIdString");
    if (![current respondsToSelector:logged] || !((BOOL(*)(id,SEL))[current methodForSelector:logged])(current,logged) || ![current respondsToSelector:identifier]) return @"";
    id value=((id(*)(id,SEL))[current methodForSelector:identifier])(current,identifier);
    if (![value isKindOfClass:NSString.class] || [value length]>20 || ![value length]) return @"";
    return [value rangeOfString:@"^[0-9]{1,20}$" options:NSRegularExpressionSearch].location!=NSNotFound ? value : @"";
}

@interface NGUsageTransport : NSObject <NSURLSessionTaskDelegate>
@end
@implementation NGUsageTransport
- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task willPerformHTTPRedirection:(NSHTTPURLResponse *)response newRequest:(NSURLRequest *)request completionHandler:(void (^)(NSURLRequest *))completionHandler {
    completionHandler([request.URL.scheme isEqual:@"https"] && [request.URL.host isEqual:@"naiwa.xxdou.icu"] ? request : nil);
}
@end

static NSDictionary *NGUsagePayload(NSUserDefaults *defaults,NSString *uid) {
    if (![defaults stringForKey:NGUsageInstallation]) {
        [defaults setObject:NSUUID.UUID.UUIDString forKey:NGUsageInstallation];
        [defaults setObject:[NSUUID.UUID.UUIDString stringByAppendingString:NSUUID.UUID.UUIDString] forKey:NGUsageSecret];
    }
    return @{@"installation":[defaults stringForKey:NGUsageInstallation] ?: @"",@"secret":[defaults stringForKey:NGUsageSecret] ?: @"",@"hongguo_id":uid ?: @"",@"plugin_version":NGVersion,@"host_version":[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"unknown",@"system_version":UIDevice.currentDevice.systemVersion,@"schema_version":@1};
}

static void NGUsageSend(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
#if NG_TEST
    if (!NGUsageTestNetworking) return;
#endif
    if (NGUsageBusy) return;
    NSString *uid=NGHongguoUserID();
    if ([NGUsageLastIdentity isEqual:uid] && NSDate.date.timeIntervalSince1970-[defaults doubleForKey:NGUsageLastSent]<21600) return;
    NSDictionary *payload=NGUsagePayload(defaults,uid);
    NGUsageBusy=YES;
    NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NGUsageOrigin stringByAppendingString:@"/api/guo/v1/usage"]]];
    request.HTTPMethod=@"POST"; [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    request.HTTPBody=[NSJSONSerialization dataWithJSONObject:payload options:0 error:nil]; request.timeoutInterval=8;
    NSURLSessionConfiguration *configuration=NSURLSessionConfiguration.ephemeralSessionConfiguration; configuration.URLCache=nil; configuration.HTTPCookieStorage=nil; configuration.HTTPMaximumConnectionsPerHost=1;
    NSURLSession *session=[NSURLSession sessionWithConfiguration:configuration delegate:[NGUsageTransport new] delegateQueue:nil];
    [[session dataTaskWithRequest:request completionHandler:^(NSData *data,NSURLResponse *response,NSError *error){
        NSInteger code=[response isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse *)response).statusCode : 0;
        [session finishTasksAndInvalidate];
        dispatch_async(dispatch_get_main_queue(),^{
            NGUsageBusy=NO;
            if (!error && code==200) { [defaults setDouble:NSDate.date.timeIntervalSince1970 forKey:NGUsageLastSent]; NGUsageLastIdentity=uid; }
        });
    }] resume];
}
static void NGUsageForeground(void) { NGUsageSend(); }

@interface NGUsageSettingsController : NGSettingsTableController
@end
@implementation NGUsageSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title=@"使用统计与隐私"; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 2; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section { return @"启动红果后，奶蛙会自动向 naiwa.xxdou.icu 发送红果版本、奶蛙版本、iOS 版本、红果 ID（未登录或未读取时为空）和随机安装编号，用于统计使用人数与版本分布。不读取登录凭据、观看内容或联系方式。网络异常不影响插件功能，统计凭据不写入设置备份。日趋势保留 90 天，180 天未活跃的安装自动删除。"; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.selectionStyle=UITableViewCellSelectionStyleNone;
    if (!path.row) { cell.textLabel.text=@"使用统计"; cell.detailTextLabel.text=@"随应用启动自动上报"; }
    else { cell.textLabel.text=@"当前红果 ID"; NSString *uid=NGHongguoUserID();cell.detailTextLabel.text=uid.length ? uid : @"未读取到，统计时留空"; }
    return cell;
}
@end
