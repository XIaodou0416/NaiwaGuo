// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
#import "NGDFlexKingProviderSDK.h"
#import "NGDKeys.h"
#import "NGDTabBarProbe.h"
#import "NGDAudioHooks.h"
#import <dlfcn.h>
static NSString *const NGDFlexKingProviderID=@"naiwaguo_debug";
static id NGDFKProviderObject(NSString *name) { Class cls=NSClassFromString(name); return cls ? [[cls alloc] init] : nil; }
static NSDictionary *NGDFKProviderSnapshot(void) { return @{@"product":@"NaiwaGuo",@"version":NGD_VERSION,@"hostAdapter":@"Hongguo",@"probe":NGDTabBarProbeReport() ?: @"",@"audioHookCoverage":NGDAudioHooksCoverageJSON(),@"scope_limit":@"Current state/current IMP only; historical IMP chain and uninstrumented hook calls unavailable."}; }
void NGDRegisterFLEXKingProvider(void) {
    static BOOL registered; if (registered) return;
    typedef BOOL (*DKFKRegisterProviderFunction)(FLEXKingProvider *, NSError **);
    DKFKRegisterProviderFunction registerProvider =
        (DKFKRegisterProviderFunction)dlsym(RTLD_DEFAULT, "FLEXKingRegisterProvider");
    if (!registerProvider) return;
    id provider=NGDFKProviderObject(@"FLEXKingProvider");id operation=NGDFKProviderObject(@"FLEXKingProviderOperation");
    if(!provider||!operation)return;
    if (![provider respondsToSelector:NSSelectorFromString(@"setProductVersion:")]
        || ![provider respondsToSelector:NSSelectorFromString(@"setBuildIdentifier:")]
        || ![provider respondsToSelector:NSSelectorFromString(@"setLimits:")]
        || ![operation respondsToSelector:NSSelectorFromString(@"setLimits:")]) return;
    [provider setValue:@2 forKey:@"sdkVersion"];
    [provider setValue:NGDFlexKingProviderID forKey:@"identifier"];
    [provider setValue:@"NaiwaGuo Debug Evidence" forKey:@"title"];
    [provider setValue:NGD_VERSION ?: @"unknown" forKey:@"productVersion"];
    [provider setValue:@"control-version" forKey:@"buildIdentifier"];
    [provider setValue:@{@"max_result_bytes":@262144,@"max_event_rate_per_second":@10} forKey:@"limits"];
    [operation setValue:@"snapshot" forKey:@"name"];
    [operation setValue:@"NaiwaGuo 诊断快照" forKey:@"title"];
    [operation setValue:@"读取插件版本、布局 Hook 当前实现及现有专项探针" forKey:@"operationDescription"];
    [operation setValue:@{@"type":@"object",@"properties":@{},@"additionalProperties":@NO} forKey:@"parameters"];
    [operation setValue:@{@"type":@"object"} forKey:@"result"];
    [operation setValue:@{@"max_bytes":@262144} forKey:@"limits"];
    [operation setValue:@NO forKey:@"mutates"];
    [operation setValue:^id(NSDictionary *arguments,NSError **error){ return NGDFKProviderSnapshot(); } forKey:@"handler"];
    [provider setValue:@[operation] forKey:@"operations"];
    NSError *error=nil;
    registered=registerProvider(provider,&error);
    if(!registered) NSLog(@"NaiwaGuo FLEXKing Provider registration failed: %@",error.localizedDescription ?: @"unknown");
}
