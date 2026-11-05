#import <stdatomic.h>
static _Atomic(BOOL) NGBlocked;
BOOL NGPluginIsBlocked(void) { return NGBlocked; }
static NSString *const NGAccessChanged=@"NaiwaGuo.AccessChanged";
static void NGRefreshFeatureAccess(void);
static BOOL NGFeatureAllowed(NSString *key) {
    return !NGBlocked || !([key hasPrefix:@"com.naiwaguo.appearance."] || [key hasPrefix:@"com.naiwaguo.playback."] || [key hasPrefix:@"com.naiwaguo.typography."] || [key hasPrefix:@"com.naiwaguo.debug."]);
}
static BOOL NGFeatureBool(NSString *key) { return NGFeatureAllowed(key) && [NSUserDefaults.standardUserDefaults boolForKey:key]; }
static id NGFeatureObject(NSString *key) { return NGFeatureAllowed(key) ? [NSUserDefaults.standardUserDefaults objectForKey:key] : nil; }
static NSString *NGFeatureString(NSString *key) { id value=NGFeatureObject(key);return [value isKindOfClass:NSString.class] ? value : nil; }
static NSArray *NGFeatureArray(NSString *key) { id value=NGFeatureObject(key);return [value isKindOfClass:NSArray.class] ? value : nil; }
static NSData *NGFeatureData(NSString *key) { id value=NGFeatureObject(key);return [value isKindOfClass:NSData.class] ? value : nil; }
static CGFloat NGFeatureFloat(NSString *key) { return [NGFeatureObject(key) doubleValue]; }
static NSInteger NGFeatureInteger(NSString *key) { return [NGFeatureObject(key) integerValue]; }
