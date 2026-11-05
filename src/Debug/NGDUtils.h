#pragma once
#import "NGDKeys.h"
static inline BOOL NGDPrefBool(NSString *key) { return !NGPluginIsBlocked() && [NSUserDefaults.standardUserDefaults boolForKey:key]; }
static inline BOOL NGDAudioSamplingWanted(void) { return NGDPrefBool(NGDKeyDebugInspectorEnabled); }
