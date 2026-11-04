// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDZipWriter.h
//  NaiwaGuo
//
//  调试导出使用的最小 ZIP 写入器。
//  以不压缩方式写入文件，使用 zlib 计算 CRC32，并支持 ZIP64 条目计数。
//

#ifndef NGDZipWriter_h
#define NGDZipWriter_h

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^NGDZipProgressBlock)(CGFloat progress);

@interface NGDZipWriter : NSObject

+ (BOOL)createZipAtPath:(NSString *)zipPath
                rootDir:(NSString *)rootDir
                  files:(NSArray<NSString *> *)files
               progress:(NGDZipProgressBlock _Nullable)progress
                  error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END

#endif /* NGDZipWriter_h */
