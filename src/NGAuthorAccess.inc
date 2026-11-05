// Hongguo 7.4.3 native adapters. Never persist a successful follow verdict.
// SSUgcApiService completion ABI: (BDRpcError *, SSGetUserRelationResponse *).
static void NGAccessOnMain(void (^work)(void)) { if (NSThread.isMainThread) work(); else dispatch_async(dispatch_get_main_queue(),work); }
static NSInteger NGAccessRelationType;
static NSString *NGAccessVerifiedAccount;
static id NGAccessValue(id object,NSString *key) {
    if (!object || !key.length) return nil;
    @try { return [object valueForKey:key]; } @catch (__unused NSException *e) { return nil; }
}
static id NGAccessObjectCall(id object,NSString *name) {
    SEL selector=NSSelectorFromString(name); if (![object respondsToSelector:selector]) return nil;
    NSMethodSignature *s=[object methodSignatureForSelector:selector]; if (s.numberOfArguments!=2 || s.methodReturnType[0]!='@') return nil;
    return ((id(*)(id,SEL))[object methodForSelector:selector])(object,selector);
}
static id NGAccessSharedService(NSString *name) {
    Class cls=NSClassFromString(name); if (!cls) return nil;
    id shared=NGAccessObjectCall(cls,@"sharedInstance"); if (shared) return shared;
    id center=NGAccessObjectCall(NSClassFromString(@"TTServiceCenter"),@"sharedInstance"); SEL selector=NSSelectorFromString(@"getService:");
    if (![center respondsToSelector:selector]) return nil;
    NSMethodSignature *s=[center methodSignatureForSelector:selector]; if (s.numberOfArguments!=3 || s.methodReturnType[0]!='@' || [s getArgumentTypeAtIndex:2][0]!='#') return nil;
    return ((id(*)(id,SEL,Class))[center methodForSelector:selector])(center,selector,cls);
}
static NSString *NGAccessAccountID(void) {
    id service=NGAccessSharedService(@"SSUserInfoService");
    if (![NGAccessValue(service,@"isLogin") boolValue]) return nil;
    id value=NGAccessValue(service,@"userId"); NSString *identifier=[value isKindOfClass:NSString.class] ? value : ([value isKindOfClass:NSNumber.class] ? [value stringValue] : nil);
    return identifier.length && ![identifier isEqualToString:@"0"] ? identifier : nil;
}
static BOOL NGAccessRPCCompatible(id service,SEL selector) {
    if (![service respondsToSelector:selector]) return NO;
    NSMethodSignature *s=[service methodSignatureForSelector:selector];
    return s.numberOfArguments==4 && s.methodReturnType[0]=='@' && !strcmp([s getArgumentTypeAtIndex:2],"@?") && !strcmp([s getArgumentTypeAtIndex:3],"@?");
}
static void NGAccessFinishQuery(NSUInteger generation,NSString *account,id error,id response) {
    if (generation!=NGAccessGeneration) return;
    if (![account isEqualToString:NGAccessAccountID()]) { NGAccessCheck(); return; }
    NSNumber *code=NGAccessValue(response,@"code"); id data=NGAccessValue(response,@"data"); id info=NGAccessValue(data,@"userInfo");
    id returnedID=NGAccessValue(info,@"userId"); NSNumber *relation=NGAccessValue(info,@"relationType");
    NSString *identifier=[returnedID isKindOfClass:NSString.class] ? returnedID : ([returnedID isKindOfClass:NSNumber.class] ? [returnedID stringValue] : nil);
    if (!identifier.length) {
        NSArray *relations=NGAccessValue(data,@"relationUserList");
        if (![relations isKindOfClass:NSArray.class]) relations=@[];
        for (id entry in relations) {
            id detail=NGAccessValue(entry,@"userInfo") ?: entry; id candidate=NGAccessValue(detail,@"userId");
            NSString *candidateID=[candidate isKindOfClass:NSString.class] ? candidate : ([candidate isKindOfClass:NSNumber.class] ? [candidate stringValue] : nil);
            if ([candidateID isEqual:NGAuthorUserID]) { identifier=candidateID; relation=NGAccessValue(entry,@"relationType") ?: NGAccessValue(detail,@"relationType"); break; }
        }
    }
    if (error || ![code isKindOfClass:NSNumber.class] || code.integerValue!=0) { NGAccessSetState(NGAccessUnavailable,@"网络或登录状态异常，暂时无法核验，请重新检测"); return; }
    if (![identifier isEqualToString:NGAuthorUserID] || ![relation isKindOfClass:NSNumber.class]) { NGAccessSetState(NGAccessUnavailable,@"未取得作者的有效关注状态，请重新检测"); return; }
    id model=[NSClassFromString(@"SSShortVideoFollowUserModel") new]; SEL setter=NSSelectorFromString(@"setFollowType:"),getter=NSSelectorFromString(@"isFollowed");
    NSMethodSignature *setSignature=[model methodSignatureForSelector:setter],*getSignature=[model methodSignatureForSelector:getter];
    if (![model respondsToSelector:setter] || ![model respondsToSelector:getter] || setSignature.numberOfArguments!=3 || strcmp([setSignature getArgumentTypeAtIndex:2],@encode(NSInteger)) || getSignature.numberOfArguments!=2 || strcmp(getSignature.methodReturnType,@encode(BOOL))) { NGAccessSetState(NGAccessUnavailable,@"当前红果版本暂不支持关注校验"); return; }
    ((void(*)(id,SEL,NSInteger))[model methodForSelector:setter])(model,setter,relation.integerValue);
    BOOL followed=((BOOL(*)(id,SEL))[model methodForSelector:getter])(model,getter);
    NGAccessRelationType=relation.integerValue; NGAccessVerifiedAccount=[account copy];
    NGAccessSetState(followed ? NGAccessFollowing : NGAccessNotFollowing,nil);
}
static void NGAccessExpireRequest(NSUInteger generation) {
    if (generation==NGAccessGeneration && NGAccessStatus==NGAccessChecking) { ++NGAccessGeneration; NGAccessSetState(NGAccessUnavailable,@"关注状态检测超时，请检查网络后重试"); }
}
static void NGAccessCheck(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(),^{NGAccessCheck();}); return; }
#if NG_TEST
    if (NGAccessTestBypass) return;
#endif
    NSUInteger generation=++NGAccessGeneration; NGAccessVerifiedAccount=nil; NGAccessSetState(NGAccessChecking,nil);
    NSString *account=NGAccessAccountID(); if (!account.length) { NGAccessSetState(NGAccessUnavailable,@"请先在红果登录，再点此重新检测"); return; }
    id service=NSClassFromString(@"SSUgcApiService"); SEL selector=NSSelectorFromString(@"getUserRelation:completion:");
    if (!NGAccessRPCCompatible(service,selector)) { NGAccessSetState(NGAccessUnavailable,@"当前红果版本暂不支持关注校验"); return; }
    void (^configure)(id)=^(id request){
        @try { [request setValue:NGAuthorUserID forKey:@"relativeUserId"]; [request setValue:NGAuthorUserID forKey:@"relativeId"]; [request setValue:@YES forKey:@"onlyRelationType"]; [request setValue:@1 forKey:@"count"]; }
        @catch (__unused NSException *e) { NGAccessOnMain(^{ if (generation==NGAccessGeneration) { ++NGAccessGeneration; NGAccessSetState(NGAccessUnavailable,@"当前红果关注接口不兼容"); } }); }
    };
    void (^completion)(id,id)=^(id error,id response){ NGAccessOnMain(^{NGAccessFinishQuery(generation,account,error,response);}); };
    @try { ((id(*)(id,SEL,id,id))[service methodForSelector:selector])(service,selector,configure,completion); }
    @catch (__unused NSException *e) { ++NGAccessGeneration; NGAccessSetState(NGAccessUnavailable,@"无法发起关注检测，请稍后重试"); return; }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(15*NSEC_PER_SEC)),dispatch_get_main_queue(),^{NGAccessExpireRequest(generation);});
}
static void NGAccessFollowAuthor(void) {
    if (!NSThread.isMainThread || NGAccessStatus!=NGAccessNotFollowing) return;
    NSString *account=NGAccessAccountID(); if (!account.length || ![account isEqualToString:NGAccessVerifiedAccount]) { NGAccessCheck(); return; }
    id manager=NGAccessSharedService(@"SSShortVideoFollowUserManager"); SEL selector=NSSelectorFromString(@"followUserById:currentFollowType:completion:");
    NSMethodSignature *s=[manager methodSignatureForSelector:selector];
    if (![manager respondsToSelector:selector] || s.numberOfArguments!=5 || strcmp([s getArgumentTypeAtIndex:3],@encode(NSInteger)) || strcmp([s getArgumentTypeAtIndex:4],"@?")) { NGAccessSetState(NGAccessUnavailable,@"当前版本无法发起关注，请在红果中关注作者后重新检测"); return; }
    NSUInteger generation=++NGAccessGeneration; NGAccessSetState(NGAccessChecking,nil);
    // Ignore the manager's result payload: only a new server query can unlock.
    void (^completion)(void)=^{NGAccessOnMain(^{if (generation==NGAccessGeneration) NGAccessCheck();});};
    @try { ((void(*)(id,SEL,id,NSInteger,id))[manager methodForSelector:selector])(manager,selector,NGAuthorUserID,NGAccessRelationType,completion); }
    @catch (__unused NSException *e) { ++NGAccessGeneration; NGAccessSetState(NGAccessUnavailable,@"关注未完成，请在红果中确认后重新检测"); return; }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(20*NSEC_PER_SEC)),dispatch_get_main_queue(),^{NGAccessExpireRequest(generation);});
}
static BOOL NGAccessAccountCheckScheduled;
static void NGAccessScheduleAccountCheck(void) {
    void (^invalidate)(void)=^{
#if NG_TEST
        if (NGAccessTestBypass) return;
#endif
        ++NGAccessGeneration; NGAccessVerifiedAccount=nil; NGAccessSetState(NGAccessUnknown,nil);
        if (NGAccessAccountCheckScheduled) return; NGAccessAccountCheckScheduled=YES;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(.15*NSEC_PER_SEC)),dispatch_get_main_queue(),^{NGAccessAccountCheckScheduled=NO;NGAccessCheck();});
    };
    if (NSThread.isMainThread) invalidate(); else dispatch_async(dispatch_get_main_queue(),invalidate);
}
static void NGInstallAuthorAccessHooks(void) {
    NGInstallAccessPreferenceHooks();
    static NSMutableSet *installed; if (!installed) installed=[NSMutableSet set];
    Class accountClass=NSClassFromString(@"SSUserInfoService");
    for (NSString *name in @[@"setCurUser:",@"onLoginWithUser:"]) {
        if (!accountClass || [installed containsObject:name]) continue; SEL sel=NSSelectorFromString(name); Method method=class_getInstanceMethod(accountClass,sel); if (!method || method_getNumberOfArguments(method)!=3) continue; IMP original=method_getImplementation(method);
        class_replaceMethod(accountClass,sel,imp_implementationWithBlock(^(id owner,id user){((void(*)(id,SEL,id))original)(owner,sel,user);NGAccessScheduleAccountCheck();}),method_getTypeEncoding(method)); [installed addObject:name];
    }
    Class followClass=NSClassFromString(@"SSShortVideoFollowUserManager");
    for (NSString *name in @[@"updateFollowType:followType:",@"syncFollowTypeById:followType:"]) {
        if (!followClass || [installed containsObject:name]) continue; SEL sel=NSSelectorFromString(name); Method method=class_getInstanceMethod(followClass,sel); if (!method || method_getNumberOfArguments(method)!=4) continue; IMP original=method_getImplementation(method);
        class_replaceMethod(followClass,sel,imp_implementationWithBlock(^(id owner,id user,NSInteger relation){((void(*)(id,SEL,id,NSInteger))original)(owner,sel,user,relation); if ([user isEqual:NGAuthorUserID]) NGAccessScheduleAccountCheck();}),method_getTypeEncoding(method)); [installed addObject:name];
    }
}
