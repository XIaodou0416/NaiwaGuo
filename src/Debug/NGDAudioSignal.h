// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDAudioSignal.h
//  NaiwaGuo
//
//  纯音频数据转换与离线信号分析；实时入口不分配内存。
//

#ifndef NGDAudioSignal_h
#define NGDAudioSignal_h

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/// 把线性 PCM 下混为单声道 Float32，返回实际写入帧数。
uint32_t NGDAudioSignalDownmix(const AudioBufferList *bufferList,
                              const AudioStreamBasicDescription *format,
                              uint32_t frameCount,
                              float *output,
                              uint32_t outputCapacity,
                              uint64_t *unreadableFrames);

NSData *NGDAudioSignalFloatWAV(const float *samples, uint64_t frameCount, double sampleRate);
NSDictionary *NGDAudioSignalMetrics(const float *samples, uint64_t count);
NSArray<NSNumber *> *NGDAudioSignalFrequencyBands(const float *samples,
                                                 uint64_t available,
                                                 double sampleRate);

/// Float32/Int16、交错/非交错、静音、频谱、WAV 与容量边界自检。
NSDictionary *NGDAudioSignalSyntheticValidation(void);

/// 共用的 1024 点 DFT setup（进程内只建一次）。供 NGDAudioLevels 逐帧复用，
/// 避免每帧新建/销毁。失败返回 NULL。
void *NGDAudioSignalDFTSetup1024(void);

#ifdef __cplusplus
}
#endif

#endif /* NGDAudioSignal_h */
