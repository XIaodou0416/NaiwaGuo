// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDDebugInspector.h
//  NaiwaGuo
//

#ifndef NGDDebugInspector_h
#define NGDDebugInspector_h

#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

void NGDDebugInspectorInstall(void);
void NGDDebugInspectorPresentMenu(void);
UIWindow *NGDDebugInspectorWindow(void);
void NGDebugOpenSettings(void);
void NGDDebugInspectorRefreshOverlay(void);

#ifdef __cplusplus
}
#endif

#endif /* NGDDebugInspector_h */
