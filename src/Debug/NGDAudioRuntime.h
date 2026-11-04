// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDAudioRuntime.h
//  NaiwaGuo
//
//  主线程采集当前媒体候选与玻璃底栏绘制几何。
//

#ifndef NGDAudioRuntime_h
#define NGDAudioRuntime_h

#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

NSDictionary *NGDAudioRuntimeMediaSnapshot(void);
NSDictionary *NGDAudioRuntimeGlassTarget(void);

#ifdef __cplusplus
}
#endif

#endif /* NGDAudioRuntime_h */
