// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDAudioTap.h
//  NaiwaGuo
//
//  进程音频输出的只读旁路。用 fishhook 重绑定 AudioToolbox 的少数入口拿到
//  输出单元/队列句柄，再用公开的 AudioUnitAddRenderNotify 旁听已渲染的样本；
//  不替换红果自己的 AURenderCallback，因此不进它出声的主链路。
//
//  这一层是后续音频可视化功能要复用的部分：取样机制与功能将要使用的完全一致。
//

#ifndef NGDAudioTap_h
#define NGDAudioTap_h

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

typedef NS_ENUM(uint32_t, NGDAudioTapSourceKind) {
    NGDAudioTapSourceKindAudioUnit = 1,
    NGDAudioTapSourceKindAudioQueue = 2,
    NGDAudioTapSourceKindMediaTap = 3,
};

/// 一路已录到的单声道 Float32 样本。samples 由 tap 持有，读取方不得释放。
typedef struct {
    uint32_t sourceID;
    uint32_t kind;
    const float *samples;
    uint64_t frameCount;
    double sampleRate;
    AudioStreamBasicDescription sourceFormat;
} NGDAudioTapSlot;

#ifdef __cplusplus
extern "C" {
#endif

/// 安装符号包装。幂等；只有第一次真正执行 rebind。
void NGDAudioTapInstall(void);

/// 关闭后所有包装器只透明转发，不再登记、不再取样。
void NGDAudioTapSetEnabled(BOOL enabled);
BOOL NGDAudioTapIsInstalled(void);

/// 开启采样窗口：先静置 warmupSeconds，再录 recordSeconds 的 PCM。
/// 失败返回 NO（未安装、已在采样、PCM 缓冲分配失败）。
BOOL NGDAudioTapBeginCapture(double warmupSeconds, double recordSeconds);
void NGDAudioTapEndCapture(void);
BOOL NGDAudioTapIsCapturing(void);

/// 等待实时写入方全部离开缓冲区，之后读槽位才是安全的。
void NGDAudioTapWaitForWriters(void);

uint64_t NGDAudioTapCaptureStartTicks(void);
uint64_t NGDAudioTapCaptureStopTicks(void);
double NGDAudioTapSecondsFromTicks(uint64_t ticks);
double NGDAudioTapSecondsRelativeTo(uint64_t ticks, uint64_t origin);
/// 相对本次采样起点的秒数；未开始采样时返回 -1。
double NGDAudioTapCurrentCaptureSecond(void);

/// 采样结束后读取录到的各路 PCM。index < NGDAudioTapSlotCount()。
uint32_t NGDAudioTapSlotCount(void);
BOOL NGDAudioTapReadSlot(uint32_t index, NGDAudioTapSlot *out);

#pragma mark - 实时电平旁路

// 与上面那套五秒采集彼此独立：采集是"开一个窗口、录完就停"，这里是常开的滚动环，
// 供音频可视化逐帧取最近一段样本。两者共用同一次下混，互不影响对方的计数。

/// 常开电平旁路的开关。关闭时实时侧完全不碰环形缓冲。
void NGDAudioTapSetLiveMeteringEnabled(BOOL enabled);
BOOL NGDAudioTapIsLiveMeteringEnabled(void);

/// 拷出当前"最响的那一路"最近的 count 个单声道 Float32 样本（out[count-1] 为最新）。
/// 返回实际拷出的帧数；无活跃音源、或拷贝期间被实时侧覆盖时返回 0。
/// sampleRateOut 可为 NULL。只在主线程调用。
uint32_t NGDAudioTapCopyLatestSamples(float *out, uint32_t count, double *sampleRateOut);

/// withinSeconds 内是否还有音源在送 buffer。用于决定可视化的显隐与帧率档位。
BOOL NGDAudioTapHasRecentAudio(double withinSeconds);

/// 后端登记表：每个 unit / queue / tap 的身份、格式、启停与统计。
NSArray<NSDictionary *> *NGDAudioTapBackendsJSON(void);
/// 每个被重绑定符号的覆盖情况与命中调用者。
NSArray<NSDictionary *> *NGDAudioTapSymbolCoverageJSON(void);
/// 实时层事件行（含 ticks 键），供上层与 ObjC 事件合并后按时间排序。
NSArray<NSDictionary *> *NGDAudioTapEventRows(void);
/// 溢出、丢帧、争用等计数器汇总。
NSDictionary *NGDAudioTapCountersJSON(void);

NSArray<NSString *> *NGDAudioTapCallerImageNames(void);
/// 指针命中已登记后端时返回 source-NN，否则 nil。
NSString *NGDAudioTapSourceIDForPointer(uintptr_t pointer);

#ifdef __cplusplus
}
#endif

#endif /* NGDAudioTap_h */
