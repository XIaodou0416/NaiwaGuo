static NSString *const NGUsageInstallation=@"com.naiwaguo.service.installation";
static NSString *const NGUsageSecret=@"com.naiwaguo.service.installationSecret";
static NSString *const NGUsageLastSent=@"com.naiwaguo.service.lastSent";
static NSString *const NGUsageBans=@"com.naiwaguo.service.blockedIDs";
static NSString *const NGUsageOrigin=@"https://naiwa.xxdou.icu";
static BOOL NGUsageBusy;
static NSString *NGUsageLastIdentity;
#if NG_TEST
static BOOL NGUsageTestNetworking;
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

static void NGUsageApplyBlock(NSString *uid,BOOL blocked) {
    if (!uid.length) return;
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    NSMutableSet *ids=[NSMutableSet setWithArray:[defaults arrayForKey:NGUsageBans] ?: @[]];
    if (blocked) [ids addObject:uid]; else [ids removeObject:uid];
    [defaults setObject:ids.allObjects forKey:NGUsageBans];
}
static void NGUsageSyncAccess(void) {
    NSString *uid=NGHongguoUserID();
    BOOL blocked=uid.length && [[NSUserDefaults.standardUserDefaults arrayForKey:NGUsageBans] containsObject:uid];
    if (blocked==NGBlocked) return;
    NGBlocked=blocked; NGRefreshFeatureAccess();
    [NSNotificationCenter.defaultCenter postNotificationName:NGAccessChanged object:nil];
}
static void NGUsageSend(BOOL force) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    NGUsageSyncAccess();
#if NG_TEST
    if (!NGUsageTestNetworking) return;
#endif
    if (NGUsageBusy) return;
    NSString *uid=NGHongguoUserID();
    if (!force && [NGUsageLastIdentity isEqual:uid] && NSDate.date.timeIntervalSince1970-[defaults doubleForKey:NGUsageLastSent]<60) return;
    NSDictionary *payload=NGUsagePayload(defaults,uid);
    NGUsageBusy=YES;
    NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NGUsageOrigin stringByAppendingString:@"/api/guo/v1/usage"]]];
    request.HTTPMethod=@"POST"; [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    request.HTTPBody=[NSJSONSerialization dataWithJSONObject:payload options:0 error:nil]; request.timeoutInterval=8;
    NSURLSessionConfiguration *configuration=NSURLSessionConfiguration.ephemeralSessionConfiguration; configuration.URLCache=nil; configuration.HTTPCookieStorage=nil; configuration.HTTPMaximumConnectionsPerHost=1;
    NSURLSession *session=[NSURLSession sessionWithConfiguration:configuration delegate:[NGUsageTransport new] delegateQueue:nil];
    [[session dataTaskWithRequest:request completionHandler:^(NSData *data,NSURLResponse *response,NSError *error){
        NSInteger code=[response isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse *)response).statusCode : 0;
        id body=data.length ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
        [session finishTasksAndInvalidate];
        dispatch_async(dispatch_get_main_queue(),^{
            NGUsageBusy=NO;
            if (!error && code==200 && [body isKindOfClass:NSDictionary.class] && [body[@"ok"] isEqual:@YES]) {
                [defaults setDouble:NSDate.date.timeIntervalSince1970 forKey:NGUsageLastSent]; NGUsageLastIdentity=uid;
                if ([body[@"hongguo_id"] isEqual:uid] && [body[@"blocked"] isKindOfClass:NSNumber.class]) NGUsageApplyBlock(uid,[body[@"blocked"] boolValue]);
                NGUsageSyncAccess();
            }
        });
    }] resume];
}
static void NGUsageForeground(void) { NGUsageSend(NO); }
static void NGUsageStart(void) {
    NGUsageSyncAccess(); NGUsageSend(YES);
    static dispatch_once_t once;dispatch_once(&once,^{
        [NSTimer scheduledTimerWithTimeInterval:60 repeats:YES block:^(NSTimer *timer){if(UIApplication.sharedApplication.applicationState==UIApplicationStateActive)NGUsageForeground();}];
    });
}
