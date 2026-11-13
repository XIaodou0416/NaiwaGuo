#import <Foundation/Foundation.h>
#import <math.h>
#include "../src/NGPreferences.inc"

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "Failed: %s (line %d)\n", #condition, __LINE__); return 1; } } while (0)

int main(void) {
    @autoreleasepool {
        NSString *domain = NSBundle.mainBundle.bundleIdentifier;
        CHECK([domain hasPrefix:@"com.naiwaguo.preferences-test."]);
        NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
        NSDictionary *saved = [defaults persistentDomainForName:domain];
        @try {
            [defaults removePersistentDomainForName:domain];
            NSString *dock = @"com.naiwaguo.appearance.dock";
            NSString *scale = @"com.naiwaguo.typography.scale";
            NSString *font = @"com.naiwaguo.typography.name";
            NSString *image = @"com.naiwaguo.appearance.customImage";
            NSString *items = @"com.naiwaguo.appearance.items";
            NSString *count = @"com.naiwaguo.appearance.count";
            CHECK(!NGFeatureBool(dock));
            CHECK(NGFeatureObject(font) == nil);
            [defaults setBool:YES forKey:dock];
            [defaults setDouble:0.7 forKey:scale];
            [defaults setObject:@"Georgia" forKey:font];
            [defaults setObject:[@"image-data" dataUsingEncoding:NSUTF8StringEncoding] forKey:image];
            [defaults setObject:@[@"home", @"shop"] forKey:items];
            [defaults setInteger:3 forKey:count];
            [defaults setObject:@"host-credential" forKey:@"host.accountToken"];
            [defaults setBool:YES forKey:@"com.naiwaguo.prompts.sponsorNever"];
            NSArray *retired = @[@"installation", @"installationSecret", @"lastSent", @"blockedIDs", @"noticeShown"];
            for (NSString *suffix in retired) {
                [defaults setObject:@"old-service-data" forKey:[@"com.naiwaguo.service." stringByAppendingString:suffix]];
            }
            [defaults setObject:@"unrelated" forKey:@"com.other.service.installation"];
            [defaults setObject:@"preserve" forKey:@"com.naiwaguo.serviceExtra"];
            // Old service data cannot gate local settings, even before cleanup.
            CHECK(NGFeatureBool(dock));
            NGRemoveRetiredServicePreferences(defaults);
            NGRemoveRetiredServicePreferences(defaults);
            [defaults synchronize];
            NSUserDefaults *reloaded = [NSUserDefaults new];
            NSDictionary *persisted = [reloaded persistentDomainForName:domain];
            for (NSString *suffix in retired) {
                NSString *key = [@"com.naiwaguo.service." stringByAppendingString:suffix];
                CHECK([defaults objectForKey:key] == nil);
                CHECK(persisted[key] == nil);
            }
            CHECK(NGFeatureBool(dock));
            CHECK(fabs(NGFeatureFloat(scale) - 0.7) < 0.001);
            CHECK([NGFeatureString(font) isEqual:@"Georgia"]);
            CHECK(NGFeatureData(image).length == 10);
            CHECK(([NGFeatureArray(items) isEqual:@[@"home", @"shop"]]));
            CHECK(NGFeatureInteger(count) == 3);
            CHECK(NGFeatureString(dock) == nil && NGFeatureArray(dock) == nil && NGFeatureData(dock) == nil);
            CHECK([persisted[@"host.accountToken"] isEqual:@"host-credential"]);
            CHECK([persisted[@"com.naiwaguo.prompts.sponsorNever"] boolValue]);
            CHECK([persisted[@"com.other.service.installation"] isEqual:@"unrelated"]);
            CHECK([persisted[@"com.naiwaguo.serviceExtra"] isEqual:@"preserve"]);
            puts("Local preferences and persisted service-data migration: passed");
        } @finally {
            if (saved) [defaults setPersistentDomain:saved forName:domain];
            else [defaults removePersistentDomainForName:domain];
            [defaults synchronize];
        }
    }
    return 0;
}
