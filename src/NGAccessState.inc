// Follow authorization is session-only. Saved feature choices are never erased.
#import <stdatomic.h>
static NSString *const NGAuthorUserID = @"106173406210";
static NSString *const NGAccessChangedNotification = @"NaiwaGuo.Access.Changed";
typedef NS_ENUM(NSInteger, NGAccessState) { NGAccessUnknown, NGAccessChecking, NGAccessNotFollowing, NGAccessFollowing, NGAccessUnavailable, NGAccessAuthor };
static NGAccessState NGAccessStatus=NGAccessUnknown;
static atomic_bool NGAccessVerified;
static NSUInteger NGAccessGeneration;
static NSString *NGAccessFailure;
#if NG_TEST
static BOOL NGAccessTestBypass=YES;
static BOOL NGAccessTestPrompts;
#endif
static void NGRefreshAfterAccessChange(void);
static void NGAccessCheck(void);
static BOOL NGAccessAllowed(void) {
#if NG_TEST
    if (NGAccessTestBypass) return YES;
#endif
    return atomic_load(&NGAccessVerified);
}
static BOOL NGAccessMasksPreference(NSUserDefaults *defaults,NSString *key) {
    return !NGAccessAllowed() && defaults==NSUserDefaults.standardUserDefaults && [key hasPrefix:@"com.naiwaguo."] && ![key hasPrefix:@"com.naiwaguo.prompts."] && ![key hasPrefix:@"com.naiwaguo.access."] && ![key hasPrefix:@"com.naiwaguo.migration."];
}
static void NGInstallAccessPreferenceHooks(void) {
    static dispatch_once_t once; dispatch_once(&once,^{
        for (NSString *name in @[@"objectForKey:",@"stringForKey:",@"arrayForKey:",@"dictionaryForKey:",@"dataForKey:",@"stringArrayForKey:"]) {
            SEL selector=NSSelectorFromString(name); Method method=class_getInstanceMethod(NSUserDefaults.class,selector); IMP original=method_getImplementation(method);
            method_setImplementation(method,imp_implementationWithBlock(^id(NSUserDefaults *defaults,NSString *key){ return NGAccessMasksPreference(defaults,key) ? nil : ((id(*)(id,SEL,id))original)(defaults,selector,key); }));
        }
        SEL boolSelector=@selector(boolForKey:); Method bm=class_getInstanceMethod(NSUserDefaults.class,boolSelector); IMP bo=method_getImplementation(bm);
        method_setImplementation(bm,imp_implementationWithBlock(^BOOL(NSUserDefaults *d,NSString *key){return NGAccessMasksPreference(d,key) ? NO : ((BOOL(*)(id,SEL,id))bo)(d,boolSelector,key);}));
        SEL integerSelector=@selector(integerForKey:); Method im=class_getInstanceMethod(NSUserDefaults.class,integerSelector); IMP io=method_getImplementation(im);
        method_setImplementation(im,imp_implementationWithBlock(^NSInteger(NSUserDefaults *d,NSString *key){return NGAccessMasksPreference(d,key) ? 0 : ((NSInteger(*)(id,SEL,id))io)(d,integerSelector,key);}));
        SEL doubleSelector=@selector(doubleForKey:); Method dm=class_getInstanceMethod(NSUserDefaults.class,doubleSelector); IMP d0=method_getImplementation(dm);
        method_setImplementation(dm,imp_implementationWithBlock(^double(NSUserDefaults *d,NSString *key){return NGAccessMasksPreference(d,key) ? 0 : ((double(*)(id,SEL,id))d0)(d,doubleSelector,key);}));
        SEL floatSelector=@selector(floatForKey:); Method fm=class_getInstanceMethod(NSUserDefaults.class,floatSelector); IMP fo=method_getImplementation(fm);
        method_setImplementation(fm,imp_implementationWithBlock(^float(NSUserDefaults *d,NSString *key){return NGAccessMasksPreference(d,key) ? 0 : ((float(*)(id,SEL,id))fo)(d,floatSelector,key);}));
    });
}
static void NGAccessSetState(NGAccessState state,NSString *failure) {
    NSCAssert(NSThread.isMainThread,@"Access state must update on the main thread");
    BOOL allowed=state==NGAccessFollowing || state==NGAccessAuthor;
    BOOL previous=atomic_exchange(&NGAccessVerified,allowed);
    NGAccessStatus=state; NGAccessFailure=[failure copy];
    if (previous!=allowed) NGRefreshAfterAccessChange();
    [NSNotificationCenter.defaultCenter postNotificationName:NGAccessChangedNotification object:nil];
}
static NSString *NGAccessStatusText(void) {
    switch (NGAccessStatus) {
        case NGAccessFollowing:return @"已确认关注作者，功能已解锁";
        case NGAccessAuthor:return @"当前为作者本人账号，功能已解锁";
        case NGAccessChecking:return @"正在核对关注状态，功能暂不可用";
        case NGAccessNotFollowing:return @"关注作者后可使用插件功能";
        case NGAccessUnavailable:return NGAccessFailure ?: @"暂时无法核对关注状态，请重试";
        default:return @"请先核对作者关注状态";
    }
}
