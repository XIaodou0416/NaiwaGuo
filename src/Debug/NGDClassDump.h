// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDClassDump.h
//  NaiwaGuo
//
//  把 ObjC 运行时元数据生成类头文件文本。内置安全内省检查。
//

#ifndef NGDClassDump_h
#define NGDClassDump_h

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#ifdef __cplusplus
extern "C" {
#endif

/// 该类是否可安全内省（不安全类集合 + 根类检查）。
BOOL NGDClassIsSafe(Class cls);

/// 该类名是否为运行时生成的子类噪声（NSKVONotifying_ / _hmd_subfix_ / _AWEPERF_ / _block_invoke / %）。
BOOL NGDClassNameIsRuntimeGenerated(NSString *name);

/// 生成单个类的头文件文本。
/// 内含安全内省检查 + @try 兜底；不安全 / nil 一律返回 nil。
NSString *NGDClassDumpHeaderForClass(Class cls);

#ifdef __cplusplus
}
#endif

#endif /* NGDClassDump_h */
