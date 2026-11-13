static BOOL NGFeatureBool(NSString *key) { return [NSUserDefaults.standardUserDefaults boolForKey:key]; }
static id NGFeatureObject(NSString *key) { return [NSUserDefaults.standardUserDefaults objectForKey:key]; }
static NSString *NGFeatureString(NSString *key) { id value=NGFeatureObject(key);return [value isKindOfClass:NSString.class] ? value : nil; }
static NSArray *NGFeatureArray(NSString *key) { id value=NGFeatureObject(key);return [value isKindOfClass:NSArray.class] ? value : nil; }
static NSData *NGFeatureData(NSString *key) { id value=NGFeatureObject(key);return [value isKindOfClass:NSData.class] ? value : nil; }
static CGFloat NGFeatureFloat(NSString *key) { return [NGFeatureObject(key) doubleValue]; }
static NSInteger NGFeatureInteger(NSString *key) { return [NGFeatureObject(key) integerValue]; }

// Remove only this plugin's retired service data when upgrading from older builds.
// Host credentials and local customization settings are deliberately preserved.
static void NGRemoveRetiredServicePreferences(NSUserDefaults *defaults) {
    NSDictionary *domain = [defaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier] ?: @{};
    for (NSString *key in domain) {
        if ([key hasPrefix:@"com.naiwaguo.service."]) [defaults removeObjectForKey:key];
    }
}
