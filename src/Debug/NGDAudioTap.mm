// Adapted from HierarchThurs/DYKiller bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4.
// Copyright (c) 2026 Hierarch. MIT; see third_party/DYKiller-LICENSE.
//
//  NGDAudioTap.mm
//  NaiwaGuo
//
//  实时层。这个文件里凡是会被音频渲染线程执行到的代码（NGDRenderNotify、
//  NGDCaptureBuffer 及其调用链）必须遵守：不分配内存、不加锁、不发 ObjC 消息、
//  不做任何可能阻塞的系统调用。缓冲区在采样开始前于主线程一次性分配。
//
//  取样口是公开只读的 AudioUnitAddRenderNotify：红果全二进制 0 引用该 API，
//  我们独占；回调里只读 ioData 并原样返回 noErr，不影响它的输出。
//

#import "NGDAudioTap.h"
#import "NGDAudioSignal.h"
#import "fishhook.h"

#import <CoreMedia/CoreMedia.h>
#import <MediaToolbox/MediaToolbox.h>
#import <dlfcn.h>
#import <mach/mach_time.h>
#import <os/lock.h>
#import <unistd.h>
#if __has_feature(ptrauth_calls)
#import <ptrauth.h>
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

namespace {

constexpr uint32_t kDKMaxSources = 32;
constexpr uint32_t kDKMaxEvents = 4096;
constexpr uint32_t kDKCaptureSlots = 8;
constexpr uint32_t kDKMaxFormatRecords = 8;
constexpr double kDKMaxSampleRate = 192000.0;
constexpr double kDKMaxPCMSeconds = 4.0;
constexpr uint64_t kDKMaxPCMFrames = (uint64_t)(kDKMaxSampleRate * kDKMaxPCMSeconds);

// 实时电平旁路的容量。全部是 BSS 里的定长数组：实时侧永远不分配，也没有生命周期问题。
// 4 路 × 4096 帧 × 4 字节 = 64 KB；4096 帧在 48 kHz 下是 85 ms，够 1024 点分析窗用还有余量。
constexpr uint32_t kDKLiveRings = 4;
constexpr uint32_t kDKLiveRingFrames = 4096;
constexpr uint32_t kDKLiveRingMask = kDKLiveRingFrames - 1;
static_assert((kDKLiveRingFrames & kDKLiveRingMask) == 0, "环长必须是 2 的幂");
// 单次回调最多下混这么多帧进环。红果实测 1024，留一倍余量；栈上 8 KB。
constexpr uint32_t kDKLiveBlockFrames = 2048;

// MARK: - 事件

enum NGDEventKind : uint32_t {
    NGDEventComponentNew = 1,
    NGDEventComponentDispose,
    NGDEventUnitInitialize,
    NGDEventOutputStart,
    NGDEventOutputStop,
    NGDEventRenderNotifyInstall,
    NGDEventQueueNew,
    NGDEventQueueStart,
    NGDEventQueueStop,
    NGDEventQueueDispose,
    NGDEventQueueEnqueue,
    NGDEventMediaTapCreate,
};

static const char *NGDEventName(uint32_t kind) {
    switch (kind) {
        case NGDEventComponentNew: return "AudioComponentInstanceNew";
        case NGDEventComponentDispose: return "AudioComponentInstanceDispose";
        case NGDEventUnitInitialize: return "AudioUnitInitialize";
        case NGDEventOutputStart: return "AudioOutputUnitStart";
        case NGDEventOutputStop: return "AudioOutputUnitStop";
        case NGDEventRenderNotifyInstall: return "AudioUnitAddRenderNotify";
        case NGDEventQueueNew: return "AudioQueueNewOutput";
        case NGDEventQueueStart: return "AudioQueueStart";
        case NGDEventQueueStop: return "AudioQueueStop";
        case NGDEventQueueDispose: return "AudioQueueDispose";
        case NGDEventQueueEnqueue: return "AudioQueueEnqueueBuffer";
        case NGDEventMediaTapCreate: return "MTAudioProcessingTapCreate";
    }
    return "Unknown";
}

struct NGDEvent {
    std::atomic<uint32_t> ready;
    uint32_t kind;
    uint32_t sourceID;
    int32_t status;
    uint64_t ticks;
    uintptr_t caller;
    uint64_t value1;
    uint64_t value2;
};

// MARK: - 符号

enum NGDSymbolIndex : uint32_t {
    NGDSymbolComponentNew = 0,
    NGDSymbolComponentDispose,
    NGDSymbolUnitInitialize,
    NGDSymbolOutputStart,
    NGDSymbolOutputStop,
    NGDSymbolQueueNewOutput,
    NGDSymbolQueueNewOutputDispatch,
    NGDSymbolQueueStart,
    NGDSymbolQueueStop,
    NGDSymbolQueueDispose,
    NGDSymbolQueueEnqueue,
    NGDSymbolTapCreate,
    NGDSymbolCount,
};

static const char *const NGDSymbolNames[NGDSymbolCount] = {
    "AudioComponentInstanceNew",
    "AudioComponentInstanceDispose",
    "AudioUnitInitialize",
    "AudioOutputUnitStart",
    "AudioOutputUnitStop",
    "AudioQueueNewOutput",
    "AudioQueueNewOutputWithDispatchQueue",
    "AudioQueueStart",
    "AudioQueueStop",
    "AudioQueueDispose",
    "AudioQueueEnqueueBuffer",
    "MTAudioProcessingTapCreate",
};

struct NGDSymbolCoverage {
    void *original;
    int rebindResult;
    std::atomic<uint64_t> hits;
    std::atomic<uint64_t> firstTicks;
    std::atomic<uint64_t> lastTicks;
    std::atomic<uintptr_t> firstCaller;
    std::atomic<uintptr_t> lastCaller;
    std::atomic<int32_t> lastStatus;
};

// MARK: - 源

struct NGDFormatRecord {
    std::atomic<uint32_t> version;
    uint32_t scope;
    uint64_t ticks;
    AudioStreamBasicDescription format;
};

struct NGDSource {
    std::atomic<uintptr_t> handle;
    uint32_t sourceID;
    uint32_t kind;
    OSType componentType;
    OSType componentSubType;
    OSType componentManufacturer;
    // seqlock：实时侧只读，写在非实时侧。
    std::atomic<uint32_t> formatVersion;
    AudioStreamBasicDescription format;
    std::atomic<uint32_t> formatRecordCount;
    NGDFormatRecord formatRecords[kDKMaxFormatRecords];
    std::atomic<int> active;
    std::atomic<int> disposed;
    std::atomic<int> notifyInstalled;
    std::atomic<int32_t> notifyStatus;
    std::atomic<uint64_t> hits;
    std::atomic<uint64_t> frames;
    std::atomic<uint64_t> firstCallbackTicks;
    std::atomic<uint64_t> previousCallbackTicks;
    std::atomic<uint64_t> callbackIntervalCount;
    std::atomic<uint64_t> callbackIntervalTicks;
    std::atomic<uint64_t> minCallbackIntervalTicks;
    std::atomic<uint64_t> maxCallbackIntervalTicks;
    std::atomic<uint64_t> estimatedDroppedFrames;
    std::atomic<uint64_t> lastTicks;
    std::atomic<int32_t> lastStatus;
    std::atomic<uint64_t> contentionDrops;
    std::atomic<uint64_t> unsupportedBuffers;
    std::atomic<uint64_t> unreadableFrames;
    std::atomic<uint64_t> captureSlotMisses;
    std::atomic<uint64_t> formatMismatchBuffers;
    std::atomic<uint64_t> pcmCapacityDroppedFrames;
    std::atomic<uint64_t> configurationOverflow;
    std::atomic<uint64_t> probeTicks;
    std::atomic<uint64_t> probeBlocks;
    std::atomic<uint64_t> maxProbeTicks;
    std::atomic<int> captureSlot;
    // ioData 的实测排布。缓存的 ASBD 描述的是客户端推进去的格式，与 PostRender 拿到的
    // buffer 排布允许不同（beta2 就在这里读错了），故把真实几何原样记下来。
    std::atomic<uint32_t> layoutNumberBuffers;
    std::atomic<uint32_t> layoutChannelsPerBuffer;
    std::atomic<uint32_t> layoutDataByteSize;
    std::atomic<uint32_t> layoutRenderFrames;
    // 实时电平旁路。liveRing 一旦认领就不再归还——源本身是只增不减的。
    std::atomic<int> liveRing;
    std::atomic<uint64_t> liveWrite;
    std::atomic<uint64_t> liveTicks;
    std::atomic<double> liveSampleRate;
    // 块 RMS 的指数滑动平均，选源用。float 的 atomic 在 arm64 上是无锁的。
    std::atomic<float> liveEnergy;
    std::atomic_flag captureGuard = ATOMIC_FLAG_INIT;
};

struct NGDPCMSlot {
    float *samples;
    std::atomic<int> sourceID;
    std::atomic<uint64_t> frameCount;
    AudioStreamBasicDescription format;
};

// MARK: - 全局状态

static NGDSource NGDSources[kDKMaxSources];
static std::atomic<uint32_t> NGDSourceCount { 0 };
static std::atomic<uint64_t> NGDSourceOverflow { 0 };
static os_unfair_lock NGDSourceLock = OS_UNFAIR_LOCK_INIT;

static NGDEvent NGDEvents[kDKMaxEvents];
static std::atomic<uint32_t> NGDEventCount { 0 };
static std::atomic<uint64_t> NGDEventOverflow { 0 };
static NGDSymbolCoverage NGDCoverage[NGDSymbolCount];

static NGDPCMSlot NGDPCMSlots[kDKCaptureSlots];
static std::atomic<uint64_t> NGDPCMAllocationFailures { 0 };
static std::atomic<int> NGDActiveWriters { 0 };

// 实时电平旁路：每路一个滚动环，单写者（该源自己的渲染线程）+ 单读者（主线程）。
static float NGDLiveRing[kDKLiveRings][kDKLiveRingFrames];
static std::atomic<uint32_t> NGDLiveRingsClaimed { 0 };
static std::atomic<bool> NGDLiveEnabled { false };

static std::atomic<bool> NGDTapEnabled { false };
static std::atomic<bool> NGDTapInstalled { false };
static std::atomic<bool> NGDCaptureActive { false };
static os_unfair_lock NGDCaptureControlLock = OS_UNFAIR_LOCK_INIT;
static std::atomic<uint64_t> NGDCaptureStartTicks { 0 };
static std::atomic<uint64_t> NGDCaptureStopTicks { 0 };
static std::atomic<uint64_t> NGDPCMStartTicks { 0 };
static std::atomic<uint64_t> NGDPCMStopTicks { 0 };
static mach_timebase_info_data_t NGDTimebase;

static __attribute__((always_inline)) inline uintptr_t NGDStripCaller(void *address) {
#if __has_feature(ptrauth_calls)
    return (uintptr_t)ptrauth_strip(address, ptrauth_key_return_address);
#else
    return (uintptr_t)address;
#endif
}

#define NGD_CALLER() NGDStripCaller(__builtin_return_address(0))

static uint64_t NGDNanosecondsToTicks(uint64_t ns) {
    if (NGDTimebase.numer == 0) mach_timebase_info(&NGDTimebase);
    return (uint64_t)((long double)ns * NGDTimebase.denom / NGDTimebase.numer);
}

static double NGDTicksToSeconds(uint64_t ticks) {
    if (NGDTimebase.numer == 0) mach_timebase_info(&NGDTimebase);
    return (double)((long double)ticks * NGDTimebase.numer / NGDTimebase.denom / 1000000000.0L);
}

static double NGDRelativeSeconds(uint64_t ticks, uint64_t origin) {
    if (ticks == 0 || origin == 0) return -1.0;
    return ticks >= origin ? NGDTicksToSeconds(ticks - origin) : -NGDTicksToSeconds(origin - ticks);
}

static NSString *NGDPointerString(uintptr_t pointer) {
    return [NSString stringWithFormat:@"0x%llx", (unsigned long long)pointer];
}

static NSString *NGDFourCC(OSType value) {
    char chars[5] = {
        (char)((value >> 24) & 0xff), (char)((value >> 16) & 0xff),
        (char)((value >> 8) & 0xff), (char)(value & 0xff), 0
    };
    for (int i = 0; i < 4; i++) {
        if (chars[i] < 32 || chars[i] > 126) return [NSString stringWithFormat:@"0x%08x", value];
    }
    return [NSString stringWithUTF8String:chars] ?: @"";
}

static NSString *NGDImageForAddress(uintptr_t address) {
    if (!address) return @"";
    Dl_info info = {};
    if (dladdr((const void *)address, &info) == 0 || !info.dli_fname) return @"";
    NSString *path = [NSString stringWithUTF8String:info.dli_fname] ?: @"";
    return path.lastPathComponent ?: @"";
}

static NSString *NGDSymbolForAddress(uintptr_t address) {
    if (!address) return @"";
    Dl_info info = {};
    if (dladdr((const void *)address, &info) == 0 || !info.dli_sname) return @"";
    return [NSString stringWithUTF8String:info.dli_sname] ?: @"";
}

static NSString *NGDSourceKindName(uint32_t kind) {
    switch (kind) {
        case NGDAudioTapSourceKindAudioUnit: return @"AudioUnit";
        case NGDAudioTapSourceKindAudioQueue: return @"AudioQueue";
        case NGDAudioTapSourceKindMediaTap: return @"MediaProcessingTap";
    }
    return @"Unknown";
}

static NSDictionary *NGDASBDJSON(const AudioStreamBasicDescription &format) {
    return @{
        @"sampleRate": @(format.mSampleRate),
        @"formatID": NGDFourCC(format.mFormatID),
        @"formatFlags": @(format.mFormatFlags),
        @"bytesPerPacket": @(format.mBytesPerPacket),
        @"framesPerPacket": @(format.mFramesPerPacket),
        @"bytesPerFrame": @(format.mBytesPerFrame),
        @"channelsPerFrame": @(format.mChannelsPerFrame),
        @"bitsPerChannel": @(format.mBitsPerChannel),
        @"nonInterleaved": @((format.mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0),
        @"float": @((format.mFormatFlags & kAudioFormatFlagIsFloat) != 0),
        @"signedInteger": @((format.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0),
    };
}

// MARK: - 格式读写（seqlock）

static void NGDWriteSourceFormat(NGDSource *source, const AudioStreamBasicDescription *format) {
    if (!source || !format) return;
    for (int attempt = 0; attempt < 4; attempt++) {
        uint32_t version = source->formatVersion.load(std::memory_order_acquire);
        if (version & 1) continue;
        if (!source->formatVersion.compare_exchange_weak(version, version + 1,
                                                         std::memory_order_acq_rel,
                                                         std::memory_order_relaxed)) continue;
        source->format = *format;
        source->formatVersion.store(version + 2, std::memory_order_release);
        return;
    }
    source->configurationOverflow.fetch_add(1, std::memory_order_relaxed);
}

static bool NGDReadSourceFormat(NGDSource *source, AudioStreamBasicDescription *format) {
    if (!source || !format) return false;
    for (int attempt = 0; attempt < 4; attempt++) {
        uint32_t before = source->formatVersion.load(std::memory_order_acquire);
        if (before == 0 || (before & 1)) continue;
        AudioStreamBasicDescription copy = source->format;
        uint32_t after = source->formatVersion.load(std::memory_order_acquire);
        if (before == after && !(after & 1)) {
            *format = copy;
            return copy.mSampleRate > 0 && copy.mChannelsPerFrame > 0;
        }
    }
    return false;
}

static void NGDAppendFormatRecord(NGDSource *source, uint32_t scope,
                                 const AudioStreamBasicDescription *format) {
    if (!source || !format) return;
    uint32_t index = source->formatRecordCount.fetch_add(1, std::memory_order_relaxed);
    if (index >= kDKMaxFormatRecords) {
        source->configurationOverflow.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    NGDFormatRecord &record = source->formatRecords[index];
    record.version.store(1, std::memory_order_release);
    record.scope = scope;
    record.ticks = mach_continuous_time();
    record.format = *format;
    record.version.store(2, std::memory_order_release);
}

// MARK: - 源登记

static NGDSource *NGDFindSource(uintptr_t handle, uint32_t kind) {
    uint32_t count = NGDSourceCount.load(std::memory_order_acquire);
    for (uint32_t offset = 0; offset < count; offset++) {
        uint32_t i = count - offset - 1;
        if (NGDSources[i].kind == kind &&
            NGDSources[i].disposed.load(std::memory_order_relaxed) == 0 &&
            NGDSources[i].handle.load(std::memory_order_acquire) == handle) return &NGDSources[i];
    }
    return nullptr;
}

static NGDSource *NGDCreateSource(uintptr_t handle, uint32_t kind,
                                const AudioStreamBasicDescription *format) {
    if (!handle) return nullptr;
    NGDSource *existing = NGDFindSource(handle, kind);
    if (existing) {
        if (format) {
            NGDWriteSourceFormat(existing, format);
            NGDAppendFormatRecord(existing, UINT32_MAX, format);
        }
        return existing;
    }

    os_unfair_lock_lock(&NGDSourceLock);
    existing = NGDFindSource(handle, kind);
    if (existing) {
        os_unfair_lock_unlock(&NGDSourceLock);
        if (format) {
            NGDWriteSourceFormat(existing, format);
            NGDAppendFormatRecord(existing, UINT32_MAX, format);
        }
        return existing;
    }

    uint32_t index = NGDSourceCount.load(std::memory_order_relaxed);
    if (index >= kDKMaxSources) {
        NGDSourceOverflow.fetch_add(1, std::memory_order_relaxed);
        os_unfair_lock_unlock(&NGDSourceLock);
        return nullptr;
    }

    NGDSource *source = &NGDSources[index];
    source->sourceID = index + 1;
    source->kind = kind;
    source->componentType = 0;
    source->componentSubType = 0;
    source->componentManufacturer = 0;
    source->formatVersion.store(0, std::memory_order_relaxed);
    source->formatRecordCount.store(0, std::memory_order_relaxed);
    for (uint32_t i = 0; i < kDKMaxFormatRecords; i++) {
        source->formatRecords[i].version.store(0, std::memory_order_relaxed);
    }
    source->active.store(0, std::memory_order_relaxed);
    source->disposed.store(0, std::memory_order_relaxed);
    source->notifyInstalled.store(0, std::memory_order_relaxed);
    source->notifyStatus.store(0, std::memory_order_relaxed);
    source->hits.store(0, std::memory_order_relaxed);
    source->frames.store(0, std::memory_order_relaxed);
    source->firstCallbackTicks.store(0, std::memory_order_relaxed);
    source->previousCallbackTicks.store(0, std::memory_order_relaxed);
    source->callbackIntervalCount.store(0, std::memory_order_relaxed);
    source->callbackIntervalTicks.store(0, std::memory_order_relaxed);
    source->minCallbackIntervalTicks.store(0, std::memory_order_relaxed);
    source->maxCallbackIntervalTicks.store(0, std::memory_order_relaxed);
    source->estimatedDroppedFrames.store(0, std::memory_order_relaxed);
    source->lastTicks.store(0, std::memory_order_relaxed);
    source->lastStatus.store(0, std::memory_order_relaxed);
    source->contentionDrops.store(0, std::memory_order_relaxed);
    source->unsupportedBuffers.store(0, std::memory_order_relaxed);
    source->unreadableFrames.store(0, std::memory_order_relaxed);
    source->captureSlotMisses.store(0, std::memory_order_relaxed);
    source->formatMismatchBuffers.store(0, std::memory_order_relaxed);
    source->pcmCapacityDroppedFrames.store(0, std::memory_order_relaxed);
    source->configurationOverflow.store(0, std::memory_order_relaxed);
    source->probeTicks.store(0, std::memory_order_relaxed);
    source->probeBlocks.store(0, std::memory_order_relaxed);
    source->maxProbeTicks.store(0, std::memory_order_relaxed);
    source->captureSlot.store(-1, std::memory_order_relaxed);
    source->layoutNumberBuffers.store(0, std::memory_order_relaxed);
    source->layoutChannelsPerBuffer.store(0, std::memory_order_relaxed);
    source->layoutDataByteSize.store(0, std::memory_order_relaxed);
    source->layoutRenderFrames.store(0, std::memory_order_relaxed);
    source->liveRing.store(-1, std::memory_order_relaxed);
    source->liveWrite.store(0, std::memory_order_relaxed);
    source->liveTicks.store(0, std::memory_order_relaxed);
    source->liveSampleRate.store(0, std::memory_order_relaxed);
    source->liveEnergy.store(0, std::memory_order_relaxed);
    source->captureGuard.clear(std::memory_order_relaxed);
    if (format) {
        NGDWriteSourceFormat(source, format);
        NGDAppendFormatRecord(source, UINT32_MAX, format);
    }
    source->handle.store(handle, std::memory_order_release);
    NGDSourceCount.store(index + 1, std::memory_order_release);
    os_unfair_lock_unlock(&NGDSourceLock);
    return source;
}

// MARK: - 统计

static void NGDUpdateMax(std::atomic<uint64_t> &value, uint64_t candidate) {
    uint64_t current = value.load(std::memory_order_relaxed);
    while (candidate > current &&
           !value.compare_exchange_weak(current, candidate, std::memory_order_relaxed)) {}
}

static void NGDUpdateMinNonZero(std::atomic<uint64_t> &value, uint64_t candidate) {
    uint64_t current = value.load(std::memory_order_relaxed);
    while ((current == 0 || candidate < current) &&
           !value.compare_exchange_weak(current, candidate, std::memory_order_relaxed)) {}
}

static void NGDRecordProbeElapsed(NGDSource *source, uint64_t begin) {
    if (!source || begin == 0) return;
    uint64_t elapsed = mach_continuous_time() - begin;
    source->probeTicks.fetch_add(elapsed, std::memory_order_relaxed);
    source->probeBlocks.fetch_add(1, std::memory_order_relaxed);
    NGDUpdateMax(source->maxProbeTicks, elapsed);
}

static void NGDRecordCallbackActivity(NGDSource *source, uint32_t frames, OSStatus status) {
    if (!source || !NGDTapEnabled.load(std::memory_order_relaxed)) return;
    uint64_t now = mach_continuous_time();
    source->hits.fetch_add(1, std::memory_order_relaxed);
    source->frames.fetch_add(frames, std::memory_order_relaxed);
    source->lastTicks.store(now, std::memory_order_relaxed);
    source->lastStatus.store(status, std::memory_order_relaxed);
    if (!NGDCaptureActive.load(std::memory_order_relaxed)) return;

    uint64_t zero = 0;
    source->firstCallbackTicks.compare_exchange_strong(zero, now, std::memory_order_relaxed);
    uint64_t previous = source->previousCallbackTicks.exchange(now, std::memory_order_relaxed);
    if (previous == 0 || now <= previous) return;
    uint64_t interval = now - previous;
    source->callbackIntervalCount.fetch_add(1, std::memory_order_relaxed);
    source->callbackIntervalTicks.fetch_add(interval, std::memory_order_relaxed);
    NGDUpdateMinNonZero(source->minCallbackIntervalTicks, interval);
    NGDUpdateMax(source->maxCallbackIntervalTicks, interval);

    AudioStreamBasicDescription format = {};
    if (!NGDReadSourceFormat(source, &format) || format.mSampleRate <= 0 || frames == 0) return;
    uint64_t expected = NGDNanosecondsToTicks((uint64_t)llround((double)frames / format.mSampleRate * 1.0e9));
    if (expected == 0 || interval <= expected + expected / 2) return;
    uint64_t periods = interval / expected;
    if (periods > 1) {
        source->estimatedDroppedFrames.fetch_add((periods - 1) * frames, std::memory_order_relaxed);
    }
}

static void NGDRecordEvent(uint32_t kind, NGDSource *source, OSStatus status,
                          uintptr_t caller, uint64_t value1, uint64_t value2 = 0) {
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return;
    uint32_t index = NGDEventCount.fetch_add(1, std::memory_order_relaxed);
    if (index >= kDKMaxEvents) {
        NGDEventOverflow.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    NGDEvent *event = &NGDEvents[index];
    event->kind = kind;
    event->sourceID = source ? source->sourceID : 0;
    event->status = status;
    event->ticks = mach_continuous_time();
    event->caller = caller;
    event->value1 = value1;
    event->value2 = value2;
    event->ready.store(1, std::memory_order_release);
}

static void NGDHitSymbol(NGDSymbolIndex index, OSStatus status, uintptr_t caller) {
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return;
    NGDSymbolCoverage &coverage = NGDCoverage[index];
    uint64_t now = mach_continuous_time();
    if (coverage.hits.fetch_add(1, std::memory_order_relaxed) == 0) {
        coverage.firstTicks.store(now, std::memory_order_relaxed);
        coverage.firstCaller.store(caller, std::memory_order_relaxed);
    }
    coverage.lastTicks.store(now, std::memory_order_relaxed);
    coverage.lastCaller.store(caller, std::memory_order_relaxed);
    coverage.lastStatus.store(status, std::memory_order_relaxed);
}

// MARK: - PCM 写入（实时路径）

static int NGDClaimCaptureSlot(NGDSource *source, const AudioStreamBasicDescription &format) {
    int existing = source->captureSlot.load(std::memory_order_acquire);
    if (existing >= 0) return existing;
    for (uint32_t i = 0; i < kDKCaptureSlots; i++) {
        int expected = 0;
        int reservation = -(int)source->sourceID;
        if (NGDPCMSlots[i].sourceID.compare_exchange_strong(expected, reservation,
                                                           std::memory_order_acq_rel)) {
            NGDPCMSlots[i].format = format;
            NGDPCMSlots[i].sourceID.store((int)source->sourceID, std::memory_order_release);
            source->captureSlot.store((int)i, std::memory_order_release);
            return (int)i;
        }
        if (expected == (int)source->sourceID) {
            source->captureSlot.store((int)i, std::memory_order_release);
            return (int)i;
        }
    }
    return -1;
}

static bool NGDFormatCanConvert(const AudioStreamBasicDescription &format) {
    if (format.mFormatID != kAudioFormatLinearPCM || format.mChannelsPerFrame == 0 ||
        format.mSampleRate <= 0 || format.mSampleRate > kDKMaxSampleRate ||
        (format.mFormatFlags & kAudioFormatFlagIsBigEndian) ||
        !(format.mFormatFlags & kAudioFormatFlagIsPacked)) return false;
    if ((format.mFormatFlags & kAudioFormatFlagIsFloat) &&
        (format.mBitsPerChannel == 32 || format.mBitsPerChannel == 64)) return true;
    if ((format.mFormatFlags & kAudioFormatFlagIsSignedInteger) &&
        (format.mBitsPerChannel == 16 || format.mBitsPerChannel == 32)) return true;
    return false;
}

static bool NGDFormatsMatch(const AudioStreamBasicDescription &l,
                           const AudioStreamBasicDescription &r) {
    return fabs(l.mSampleRate - r.mSampleRate) < 0.001 && l.mFormatID == r.mFormatID &&
           l.mFormatFlags == r.mFormatFlags && l.mBytesPerFrame == r.mBytesPerFrame &&
           l.mChannelsPerFrame == r.mChannelsPerFrame && l.mBitsPerChannel == r.mBitsPerChannel;
}

static uint32_t NGDCaptureBuffer(NGDSource *source, const AudioBufferList *bufferList,
                                uint32_t frameCount) {
    if (!source || !bufferList || frameCount == 0 ||
        !NGDCaptureActive.load(std::memory_order_relaxed)) return 0;

    uint64_t now = mach_continuous_time();
    if (now < NGDPCMStartTicks.load(std::memory_order_relaxed) ||
        now >= NGDPCMStopTicks.load(std::memory_order_relaxed)) return 0;
    uint64_t begin = now;

    AudioStreamBasicDescription format = {};
    if (!NGDReadSourceFormat(source, &format) || !NGDFormatCanConvert(format)) {
        source->unsupportedBuffers.fetch_add(1, std::memory_order_relaxed);
        NGDRecordProbeElapsed(source, begin);
        return 0;
    }

    int slotIndex = NGDClaimCaptureSlot(source, format);
    if (slotIndex < 0 || !NGDPCMSlots[slotIndex].samples) {
        source->captureSlotMisses.fetch_add(1, std::memory_order_relaxed);
        NGDRecordProbeElapsed(source, begin);
        return 0;
    }
    if (!NGDFormatsMatch(NGDPCMSlots[slotIndex].format, format)) {
        source->formatMismatchBuffers.fetch_add(1, std::memory_order_relaxed);
        NGDRecordProbeElapsed(source, begin);
        return 0;
    }

    if (source->captureGuard.test_and_set(std::memory_order_acquire)) {
        source->contentionDrops.fetch_add(1, std::memory_order_relaxed);
        NGDRecordProbeElapsed(source, begin);
        return 0;
    }
    NGDActiveWriters.fetch_add(1, std::memory_order_relaxed);

    NGDPCMSlot &slot = NGDPCMSlots[slotIndex];
    uint64_t written = slot.frameCount.load(std::memory_order_relaxed);
    uint64_t available = written < kDKMaxPCMFrames ? kDKMaxPCMFrames - written : 0;
    uint32_t capacity = (uint32_t)std::min<uint64_t>(available, UINT32_MAX);
    if (capacity < frameCount) {
        source->pcmCapacityDroppedFrames.fetch_add(frameCount - capacity, std::memory_order_relaxed);
    }
    uint64_t unreadable = 0;
    uint32_t count = NGDAudioSignalDownmix(bufferList, &format, frameCount,
                                          slot.samples + written, capacity, &unreadable);
    if (unreadable) source->unreadableFrames.fetch_add(unreadable, std::memory_order_relaxed);
    slot.frameCount.store(written + count, std::memory_order_release);

    NGDRecordProbeElapsed(source, begin);
    NGDActiveWriters.fetch_sub(1, std::memory_order_relaxed);
    source->captureGuard.clear(std::memory_order_release);
    return count;
}

// MARK: - 实时电平旁路

// 音频渲染线程执行。认领一个环槽位；抢不到（已满 4 路）就永久放弃，返回 -1。
static int NGDClaimLiveRing(NGDSource *source) {
    int ring = source->liveRing.load(std::memory_order_acquire);
    if (ring >= 0) return ring;
    if (ring < -1) return -1;                    // -2 表示试过且抢不到

    uint32_t claimed = NGDLiveRingsClaimed.load(std::memory_order_relaxed);
    while (claimed < kDKLiveRings) {
        if (NGDLiveRingsClaimed.compare_exchange_weak(claimed, claimed + 1,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_relaxed)) {
            source->liveWrite.store(0, std::memory_order_relaxed);
            source->liveRing.store((int)claimed, std::memory_order_release);
            return (int)claimed;
        }
    }
    source->liveRing.store(-2, std::memory_order_release);
    return -1;
}

// 音频渲染线程执行。下混一块进栈上缓冲，再拷进滚动环，顺手更新能量与时刻。
// 与 NGDCaptureBuffer 各走各的：采集窗口开没开都不影响这里。
static void NGDPublishLive(NGDSource *source, const AudioBufferList *bufferList, uint32_t frameCount) {
    if (!source || !bufferList || frameCount == 0) return;
    if (!NGDLiveEnabled.load(std::memory_order_relaxed)) return;

    AudioStreamBasicDescription format = {};
    if (!NGDReadSourceFormat(source, &format) || !NGDFormatCanConvert(format)) return;

    int ring = NGDClaimLiveRing(source);
    if (ring < 0) return;

    float block[kDKLiveBlockFrames];
    uint32_t want = std::min(frameCount, kDKLiveBlockFrames);
    uint64_t unreadable = 0;
    uint32_t count = NGDAudioSignalDownmix(bufferList, &format, want, block,
                                          kDKLiveBlockFrames, &unreadable);
    if (count == 0) return;

    uint64_t write = source->liveWrite.load(std::memory_order_relaxed);
    uint32_t head = (uint32_t)(write & kDKLiveRingMask);
    uint32_t first = std::min(count, kDKLiveRingFrames - head);
    memcpy(NGDLiveRing[ring] + head, block, first * sizeof(float));
    if (count > first) memcpy(NGDLiveRing[ring], block + first, (count - first) * sizeof(float));
    source->liveWrite.store(write + count, std::memory_order_release);

    double sum = 0.0;
    for (uint32_t i = 0; i < count; i++) sum += (double)block[i] * block[i];
    float rms = (float)sqrt(sum / count);
    // 选源用的能量：上升立刻跟上，下降留一点惯性，避免在块间抖动时来回换源。
    float previous = source->liveEnergy.load(std::memory_order_relaxed);
    source->liveEnergy.store(rms > previous ? rms : previous + (rms - previous) * 0.25f,
                             std::memory_order_relaxed);
    source->liveSampleRate.store(format.mSampleRate, std::memory_order_relaxed);
    source->liveTicks.store(mach_continuous_time(), std::memory_order_release);
}

// MARK: - 只读旁路

// 音频渲染线程执行。四条 relaxed store，记最近一次 ioData 的真实排布。
static void NGDRecordBufferLayout(NGDSource *source, const AudioBufferList *data, uint32_t frames) {
    if (!source || !data || data->mNumberBuffers == 0) return;
    const AudioBuffer &first = data->mBuffers[0];
    source->layoutNumberBuffers.store(data->mNumberBuffers, std::memory_order_relaxed);
    source->layoutChannelsPerBuffer.store(first.mNumberChannels, std::memory_order_relaxed);
    source->layoutDataByteSize.store(first.mDataByteSize, std::memory_order_relaxed);
    source->layoutRenderFrames.store(frames, std::memory_order_relaxed);
}

// 音频渲染线程执行。只在 PostRender 且无错误时读一次 ioData，然后原样返回。
static OSStatus NGDRenderNotify(void *refCon,
                               AudioUnitRenderActionFlags *flags,
                               const AudioTimeStamp *timestamp,
                               UInt32 bus,
                               UInt32 frames,
                               AudioBufferList *data) {
    (void)timestamp;
    if (!flags || !(*flags & kAudioUnitRenderAction_PostRender)) return noErr;
    if (*flags & kAudioUnitRenderAction_PostRenderError) return noErr;
    if (bus != 0) return noErr;   // 输出单元的输出 element 固定为 0
    NGDSource *source = (NGDSource *)refCon;
    if (!source || !NGDTapEnabled.load(std::memory_order_relaxed)) return noErr;
    NGDRecordCallbackActivity(source, frames, noErr);
    if (data) {
        NGDRecordBufferLayout(source, data, frames);
        NGDPublishLive(source, data, frames);
        NGDCaptureBuffer(source, data, frames);
    }
    return noErr;
}

// 输出单元送进 RemoteIO 的客户端格式在 Input scope element 0。必须在非实时线程读好。
static void NGDCacheOutputUnitFormat(AudioUnit unit, NGDSource *source) {
    if (!unit || !source) return;
    AudioStreamBasicDescription format = {};
    UInt32 size = sizeof(format);
    if (AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Input, 0, &format, &size) != noErr) return;
    if (format.mSampleRate <= 0 || format.mChannelsPerFrame == 0) return;
    AudioStreamBasicDescription current = {};
    if (NGDReadSourceFormat(source, &current) && NGDFormatsMatch(current, format)) return;
    NGDWriteSourceFormat(source, &format);
    NGDAppendFormatRecord(source, kAudioUnitScope_Input, &format);
}

static void NGDInstallRenderNotify(AudioUnit unit, NGDSource *source, uintptr_t caller) {
    if (!unit || !source) return;
    int expected = 0;
    if (!source->notifyInstalled.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) return;
    OSStatus status = AudioUnitAddRenderNotify(unit, NGDRenderNotify, source);
    source->notifyStatus.store(status, std::memory_order_relaxed);
    if (status != noErr) source->notifyInstalled.store(0, std::memory_order_release);
    NGDRecordEvent(NGDEventRenderNotifyInstall, source, status, caller, (uintptr_t)unit);
}

static void NGDRemoveRenderNotify(AudioUnit unit, NGDSource *source) {
    if (!unit || !source) return;
    if (source->notifyInstalled.exchange(0, std::memory_order_acq_rel) == 0) return;
    AudioUnitRemoveRenderNotify(unit, NGDRenderNotify, source);
}

// MARK: - 原始函数

static OSStatus (*NGDOrigComponentNew)(AudioComponent, AudioComponentInstance *);
static OSStatus (*NGDOrigComponentDispose)(AudioComponentInstance);
static OSStatus (*NGDOrigUnitInitialize)(AudioUnit);
static OSStatus (*NGDOrigOutputStart)(AudioUnit);
static OSStatus (*NGDOrigOutputStop)(AudioUnit);
static OSStatus (*NGDOrigQueueNewOutput)(const AudioStreamBasicDescription *, AudioQueueOutputCallback, void *, CFRunLoopRef, CFStringRef, UInt32, AudioQueueRef *);
static OSStatus (*NGDOrigQueueNewOutputDispatch)(AudioQueueRef *, const AudioStreamBasicDescription *, UInt32, dispatch_queue_t, AudioQueueOutputCallbackBlock);
static OSStatus (*NGDOrigQueueStart)(AudioQueueRef, const AudioTimeStamp *);
static OSStatus (*NGDOrigQueueStop)(AudioQueueRef, Boolean);
static OSStatus (*NGDOrigQueueDispose)(AudioQueueRef, Boolean);
static OSStatus (*NGDOrigQueueEnqueue)(AudioQueueRef, AudioQueueBufferRef, UInt32, const AudioStreamPacketDescription *);
static OSStatus (*NGDOrigTapCreate)(CFAllocatorRef, const MTAudioProcessingTapCallbacks *, MTAudioProcessingTapCreationFlags, MTAudioProcessingTapRef *);

// MARK: - 包装

static OSStatus NGDHookComponentNew(AudioComponent component, AudioComponentInstance *instanceOut) {
    uintptr_t caller = NGD_CALLER();
    OSStatus status = NGDOrigComponentNew ? NGDOrigComponentNew(component, instanceOut) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    NGDHitSymbol(NGDSymbolComponentNew, status, caller);
    NGDSource *source = nullptr;
    if (status == noErr && instanceOut && *instanceOut) {
        source = NGDCreateSource((uintptr_t)*instanceOut, NGDAudioTapSourceKindAudioUnit, nullptr);
        AudioComponentDescription description = {};
        if (source && component && AudioComponentGetDescription(component, &description) == noErr) {
            source->componentType = description.componentType;
            source->componentSubType = description.componentSubType;
            source->componentManufacturer = description.componentManufacturer;
        }
    }
    NGDRecordEvent(NGDEventComponentNew, source, status, caller,
                  instanceOut ? (uintptr_t)*instanceOut : 0);
    return status;
}

static OSStatus NGDHookComponentDispose(AudioComponentInstance instance) {
    uintptr_t caller = NGD_CALLER();
    NGDSource *source = NGDTapEnabled.load(std::memory_order_relaxed)
        ? NGDFindSource((uintptr_t)instance, NGDAudioTapSourceKindAudioUnit) : nullptr;
    // 必须在实例失效之前摘掉 notify，否则回调指针悬空。
    if (source) NGDRemoveRenderNotify(instance, source);
    OSStatus status = NGDOrigComponentDispose ? NGDOrigComponentDispose(instance) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    NGDHitSymbol(NGDSymbolComponentDispose, status, caller);
    if (source && status == noErr) {
        source->active.store(0, std::memory_order_relaxed);
        source->disposed.store(1, std::memory_order_relaxed);
    }
    NGDRecordEvent(NGDEventComponentDispose, source, status, caller, (uintptr_t)instance);
    return status;
}

static OSStatus NGDHookUnitInitialize(AudioUnit unit) {
    OSStatus status = NGDOrigUnitInitialize ? NGDOrigUnitInitialize(unit) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    uintptr_t caller = NGD_CALLER();
    NGDHitSymbol(NGDSymbolUnitInitialize, status, caller);
    NGDSource *source = NGDFindSource((uintptr_t)unit, NGDAudioTapSourceKindAudioUnit);
    if (source && status == noErr) NGDCacheOutputUnitFormat(unit, source);
    NGDRecordEvent(NGDEventUnitInitialize, source, status, caller, (uintptr_t)unit);
    return status;
}

static OSStatus NGDHookOutputStart(AudioUnit unit) {
    OSStatus status = NGDOrigOutputStart ? NGDOrigOutputStart(unit) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    uintptr_t caller = NGD_CALLER();
    NGDHitSymbol(NGDSymbolOutputStart, status, caller);
    // 只有被 AudioOutputUnitStart 启动过的实例才是输出单元，也只有它值得挂旁路。
    NGDSource *source = NGDCreateSource((uintptr_t)unit, NGDAudioTapSourceKindAudioUnit, nullptr);
    if (source && status == noErr) {
        source->active.store(1, std::memory_order_relaxed);
        NGDCacheOutputUnitFormat(unit, source);
        NGDInstallRenderNotify(unit, source, caller);
    }
    if (source) source->lastStatus.store(status, std::memory_order_relaxed);
    NGDRecordEvent(NGDEventOutputStart, source, status, caller, (uintptr_t)unit);
    return status;
}

static OSStatus NGDHookOutputStop(AudioUnit unit) {
    OSStatus status = NGDOrigOutputStop ? NGDOrigOutputStop(unit) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    uintptr_t caller = NGD_CALLER();
    NGDHitSymbol(NGDSymbolOutputStop, status, caller);
    NGDSource *source = NGDFindSource((uintptr_t)unit, NGDAudioTapSourceKindAudioUnit);
    // notify 留着：同一只 unit 常被反复启停，重复装卸没有收益。
    if (source && status == noErr) source->active.store(0, std::memory_order_relaxed);
    NGDRecordEvent(NGDEventOutputStop, source, status, caller, (uintptr_t)unit);
    return status;
}

static OSStatus NGDHookQueueNewOutput(const AudioStreamBasicDescription *format,
                                     AudioQueueOutputCallback callback,
                                     void *userData,
                                     CFRunLoopRef runLoop,
                                     CFStringRef mode,
                                     UInt32 flags,
                                     AudioQueueRef *queueOut) {
    uintptr_t caller = NGD_CALLER();
    OSStatus status = NGDOrigQueueNewOutput
        ? NGDOrigQueueNewOutput(format, callback, userData, runLoop, mode, flags, queueOut)
        : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    NGDHitSymbol(NGDSymbolQueueNewOutput, status, caller);
    NGDSource *source = status == noErr && queueOut && *queueOut
        ? NGDCreateSource((uintptr_t)*queueOut, NGDAudioTapSourceKindAudioQueue, format) : nullptr;
    NGDRecordEvent(NGDEventQueueNew, source, status, caller, queueOut ? (uintptr_t)*queueOut : 0, flags);
    return status;
}

static OSStatus NGDHookQueueNewOutputDispatch(AudioQueueRef *queueOut,
                                             const AudioStreamBasicDescription *format,
                                             UInt32 flags,
                                             dispatch_queue_t queue,
                                             AudioQueueOutputCallbackBlock block) {
    uintptr_t caller = NGD_CALLER();
    OSStatus status = NGDOrigQueueNewOutputDispatch
        ? NGDOrigQueueNewOutputDispatch(queueOut, format, flags, queue, block) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    NGDHitSymbol(NGDSymbolQueueNewOutputDispatch, status, caller);
    NGDSource *source = status == noErr && queueOut && *queueOut
        ? NGDCreateSource((uintptr_t)*queueOut, NGDAudioTapSourceKindAudioQueue, format) : nullptr;
    NGDRecordEvent(NGDEventQueueNew, source, status, caller, queueOut ? (uintptr_t)*queueOut : 0, flags);
    return status;
}

static OSStatus NGDHookQueueStart(AudioQueueRef queue, const AudioTimeStamp *time) {
    OSStatus status = NGDOrigQueueStart ? NGDOrigQueueStart(queue, time) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    uintptr_t caller = NGD_CALLER();
    NGDHitSymbol(NGDSymbolQueueStart, status, caller);
    NGDSource *source = NGDFindSource((uintptr_t)queue, NGDAudioTapSourceKindAudioQueue);
    if (source && status == noErr) source->active.store(1, std::memory_order_relaxed);
    NGDRecordEvent(NGDEventQueueStart, source, status, caller, (uintptr_t)queue);
    return status;
}

static OSStatus NGDHookQueueStop(AudioQueueRef queue, Boolean immediate) {
    OSStatus status = NGDOrigQueueStop ? NGDOrigQueueStop(queue, immediate) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    uintptr_t caller = NGD_CALLER();
    NGDHitSymbol(NGDSymbolQueueStop, status, caller);
    NGDSource *source = NGDFindSource((uintptr_t)queue, NGDAudioTapSourceKindAudioQueue);
    if (source && status == noErr) source->active.store(0, std::memory_order_relaxed);
    NGDRecordEvent(NGDEventQueueStop, source, status, caller, (uintptr_t)queue, immediate);
    return status;
}

static OSStatus NGDHookQueueDispose(AudioQueueRef queue, Boolean immediate) {
    OSStatus status = NGDOrigQueueDispose ? NGDOrigQueueDispose(queue, immediate) : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    uintptr_t caller = NGD_CALLER();
    NGDHitSymbol(NGDSymbolQueueDispose, status, caller);
    NGDSource *source = NGDFindSource((uintptr_t)queue, NGDAudioTapSourceKindAudioQueue);
    if (source && status == noErr) {
        source->active.store(0, std::memory_order_relaxed);
        source->disposed.store(1, std::memory_order_relaxed);
    }
    NGDRecordEvent(NGDEventQueueDispose, source, status, caller, (uintptr_t)queue, immediate);
    return status;
}

// AudioQueue 走的是「入队即将播放的缓冲」，比 AudioUnit 旁路多一个缓冲的延迟，
// 但同样不改数据。这条是 AudioUnit 路径不成立时的兜底取样口。
static OSStatus NGDHookQueueEnqueue(AudioQueueRef queue,
                                   AudioQueueBufferRef buffer,
                                   UInt32 packetCount,
                                   const AudioStreamPacketDescription *packets) {
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) {
        return NGDOrigQueueEnqueue ? NGDOrigQueueEnqueue(queue, buffer, packetCount, packets)
                                  : kAudio_ParamError;
    }
    uintptr_t caller = NGD_CALLER();
    NGDSource *source = NGDFindSource((uintptr_t)queue, NGDAudioTapSourceKindAudioQueue);
    uint32_t frames = 0;
    if (source && buffer && buffer->mAudioData && buffer->mAudioDataByteSize > 0) {
        AudioStreamBasicDescription format = {};
        if (NGDReadSourceFormat(source, &format) && format.mBytesPerFrame > 0) {
            frames = buffer->mAudioDataByteSize / format.mBytesPerFrame;
            struct {
                UInt32 count;
                AudioBuffer buffer;
            } list = { 1, { format.mChannelsPerFrame, buffer->mAudioDataByteSize, buffer->mAudioData } };
            NGDCaptureBuffer(source, (const AudioBufferList *)&list, frames);
        }
    }
    OSStatus status = NGDOrigQueueEnqueue
        ? NGDOrigQueueEnqueue(queue, buffer, packetCount, packets) : kAudio_ParamError;
    NGDHitSymbol(NGDSymbolQueueEnqueue, status, caller);
    if (source) NGDRecordCallbackActivity(source, frames, status);
    if (NGDCaptureActive.load(std::memory_order_relaxed)) {
        NGDRecordEvent(NGDEventQueueEnqueue, source, status, caller,
                      buffer ? buffer->mAudioDataByteSize : 0, packetCount);
    }
    return status;
}

// 只登记「有没有人用 AVPlayer 的音频 tap」，不接管它的 process 回调。
static OSStatus NGDHookTapCreate(CFAllocatorRef allocator,
                                const MTAudioProcessingTapCallbacks *callbacks,
                                MTAudioProcessingTapCreationFlags flags,
                                MTAudioProcessingTapRef *tapOut) {
    uintptr_t caller = NGD_CALLER();
    OSStatus status = NGDOrigTapCreate ? NGDOrigTapCreate(allocator, callbacks, flags, tapOut)
                                      : kAudio_ParamError;
    if (!NGDTapEnabled.load(std::memory_order_relaxed)) return status;
    NGDHitSymbol(NGDSymbolTapCreate, status, caller);
    NGDSource *source = status == noErr && tapOut && *tapOut
        ? NGDCreateSource((uintptr_t)*tapOut, NGDAudioTapSourceKindMediaTap, nullptr) : nullptr;
    NGDRecordEvent(NGDEventMediaTapCreate, source, status, caller,
                  tapOut ? (uintptr_t)*tapOut : 0, flags);
    return status;
}

static void NGDInstallRebindings(void) {
    for (uint32_t i = 0; i < NGDSymbolCount; i++) {
        NGDCoverage[i].original = dlsym(RTLD_DEFAULT, NGDSymbolNames[i]);
        NGDCoverage[i].rebindResult = -1;
    }
#define NGD_RESOLVE(VAR, INDEX) VAR = reinterpret_cast<decltype(VAR)>(NGDCoverage[INDEX].original)
    NGD_RESOLVE(NGDOrigComponentNew, NGDSymbolComponentNew);
    NGD_RESOLVE(NGDOrigComponentDispose, NGDSymbolComponentDispose);
    NGD_RESOLVE(NGDOrigUnitInitialize, NGDSymbolUnitInitialize);
    NGD_RESOLVE(NGDOrigOutputStart, NGDSymbolOutputStart);
    NGD_RESOLVE(NGDOrigOutputStop, NGDSymbolOutputStop);
    NGD_RESOLVE(NGDOrigQueueNewOutput, NGDSymbolQueueNewOutput);
    NGD_RESOLVE(NGDOrigQueueNewOutputDispatch, NGDSymbolQueueNewOutputDispatch);
    NGD_RESOLVE(NGDOrigQueueStart, NGDSymbolQueueStart);
    NGD_RESOLVE(NGDOrigQueueStop, NGDSymbolQueueStop);
    NGD_RESOLVE(NGDOrigQueueDispose, NGDSymbolQueueDispose);
    NGD_RESOLVE(NGDOrigQueueEnqueue, NGDSymbolQueueEnqueue);
    NGD_RESOLVE(NGDOrigTapCreate, NGDSymbolTapCreate);
#undef NGD_RESOLVE

    struct ngd_rebinding bindings[NGDSymbolCount] = {
        { NGDSymbolNames[NGDSymbolComponentNew], (void *)NGDHookComponentNew, nullptr },
        { NGDSymbolNames[NGDSymbolComponentDispose], (void *)NGDHookComponentDispose, nullptr },
        { NGDSymbolNames[NGDSymbolUnitInitialize], (void *)NGDHookUnitInitialize, nullptr },
        { NGDSymbolNames[NGDSymbolOutputStart], (void *)NGDHookOutputStart, nullptr },
        { NGDSymbolNames[NGDSymbolOutputStop], (void *)NGDHookOutputStop, nullptr },
        { NGDSymbolNames[NGDSymbolQueueNewOutput], (void *)NGDHookQueueNewOutput, nullptr },
        { NGDSymbolNames[NGDSymbolQueueNewOutputDispatch], (void *)NGDHookQueueNewOutputDispatch, nullptr },
        { NGDSymbolNames[NGDSymbolQueueStart], (void *)NGDHookQueueStart, nullptr },
        { NGDSymbolNames[NGDSymbolQueueStop], (void *)NGDHookQueueStop, nullptr },
        { NGDSymbolNames[NGDSymbolQueueDispose], (void *)NGDHookQueueDispose, nullptr },
        { NGDSymbolNames[NGDSymbolQueueEnqueue], (void *)NGDHookQueueEnqueue, nullptr },
        { NGDSymbolNames[NGDSymbolTapCreate], (void *)NGDHookTapCreate, nullptr },
    };
    int result = ngd_rebind_symbols(bindings, NGDSymbolCount);
    for (uint32_t i = 0; i < NGDSymbolCount; i++) NGDCoverage[i].rebindResult = result;
}

} // namespace

// MARK: - 对外接口

void NGDAudioTapInstall(void) {
    bool expected = false;
    if (!NGDTapInstalled.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    mach_timebase_info(&NGDTimebase);
    NGDInstallRebindings();
}

void NGDAudioTapSetEnabled(BOOL enabled) {
    NGDTapEnabled.store(enabled, std::memory_order_release);
    if (!enabled) NGDCaptureActive.store(false, std::memory_order_release);
}

// MARK: - 实时电平旁路（读侧，主线程）

void NGDAudioTapSetLiveMeteringEnabled(BOOL enabled) {
    NGDLiveEnabled.store(enabled, std::memory_order_release);
}

BOOL NGDAudioTapIsLiveMeteringEnabled(void) {
    return NGDLiveEnabled.load(std::memory_order_acquire);
}

// 挑当前最响、且还在活动的那一路。
//
// beta2 实测红果同时跑两只 RemoteIO，其中一只全程恒零——取"第一路"会永远画不出东西，
// 必须按能量选。staleSeconds 之外的源不参选，否则一首歌停了还会锁在旧源上。
static NGDSource *NGDLoudestLiveSource(double staleSeconds, uint64_t now) {
    uint32_t count = NGDSourceCount.load(std::memory_order_acquire);
    NGDSource *best = nullptr;
    float bestEnergy = -1.0f;
    for (uint32_t i = 0; i < count; i++) {
        NGDSource *source = &NGDSources[i];
        if (source->liveRing.load(std::memory_order_acquire) < 0) continue;
        uint64_t ticks = source->liveTicks.load(std::memory_order_acquire);
        if (ticks == 0 || now < ticks) continue;
        if (NGDTicksToSeconds(now - ticks) > staleSeconds) continue;
        float energy = source->liveEnergy.load(std::memory_order_relaxed);
        if (energy > bestEnergy) { bestEnergy = energy; best = source; }
    }
    return best;
}

uint32_t NGDAudioTapCopyLatestSamples(float *out, uint32_t count, double *sampleRateOut) {
    if (!out || count == 0 || count > kDKLiveRingFrames) return 0;
    if (!NGDLiveEnabled.load(std::memory_order_acquire)) return 0;

    NGDSource *source = NGDLoudestLiveSource(0.25, mach_continuous_time());
    if (!source) return 0;
    int ring = source->liveRing.load(std::memory_order_acquire);
    if (ring < 0 || ring >= (int)kDKLiveRings) return 0;

    uint64_t before = source->liveWrite.load(std::memory_order_acquire);
    if (before < count) return 0;                 // 环还没填满一个分析窗

    uint32_t start = (uint32_t)((before - count) & kDKLiveRingMask);
    uint32_t first = std::min(count, kDKLiveRingFrames - start);
    memcpy(out, NGDLiveRing[ring] + start, first * sizeof(float));
    if (count > first) memcpy(out + first, NGDLiveRing[ring], (count - first) * sizeof(float));

    // 拷贝期间实时侧可能已经绕回来盖掉了我们正在读的那一段。复读游标判定：
    // 写入前进超过"环长 − 窗长"就说明被追上了，这一帧直接丢（60 Hz 下看不出来）。
    uint64_t after = source->liveWrite.load(std::memory_order_acquire);
    if (after - before > kDKLiveRingFrames - count) return 0;

    if (sampleRateOut) {
        double rate = source->liveSampleRate.load(std::memory_order_relaxed);
        *sampleRateOut = rate > 0 ? rate : 48000.0;
    }
    return count;
}

BOOL NGDAudioTapHasRecentAudio(double withinSeconds) {
    if (!NGDLiveEnabled.load(std::memory_order_acquire)) return NO;
    return NGDLoudestLiveSource(withinSeconds, mach_continuous_time()) != nullptr;
}

BOOL NGDAudioTapIsInstalled(void) {
    return NGDTapInstalled.load(std::memory_order_acquire);
}

BOOL NGDAudioTapIsCapturing(void) {
    return NGDCaptureActive.load(std::memory_order_acquire);
}

uint64_t NGDAudioTapCaptureStartTicks(void) {
    return NGDCaptureStartTicks.load(std::memory_order_acquire);
}

uint64_t NGDAudioTapCaptureStopTicks(void) {
    return NGDCaptureStopTicks.load(std::memory_order_acquire);
}

double NGDAudioTapSecondsFromTicks(uint64_t ticks) {
    return NGDTicksToSeconds(ticks);
}

double NGDAudioTapSecondsRelativeTo(uint64_t ticks, uint64_t origin) {
    return NGDRelativeSeconds(ticks, origin);
}

double NGDAudioTapCurrentCaptureSecond(void) {
    uint64_t start = NGDCaptureStartTicks.load(std::memory_order_acquire);
    return start ? NGDRelativeSeconds(mach_continuous_time(), start) : -1.0;
}

BOOL NGDAudioTapBeginCapture(double warmupSeconds, double recordSeconds) {
    if (!NGDTapInstalled.load(std::memory_order_acquire) ||
        !NGDTapEnabled.load(std::memory_order_acquire) ||
        warmupSeconds < 0 || recordSeconds <= 0 || recordSeconds > kDKMaxPCMSeconds) return NO;

    os_unfair_lock_lock(&NGDCaptureControlLock);
    if (NGDCaptureActive.load(std::memory_order_acquire)) {
        os_unfair_lock_unlock(&NGDCaptureControlLock);
        return NO;
    }

    NGDCaptureStartTicks.store(0, std::memory_order_release);
    NGDCaptureStopTicks.store(0, std::memory_order_release);
    NGDPCMAllocationFailures.store(0, std::memory_order_relaxed);
    BOOL anySlot = NO;
    for (uint32_t i = 0; i < kDKCaptureSlots; i++) {
        if (!NGDPCMSlots[i].samples) NGDPCMSlots[i].samples = (float *)calloc(kDKMaxPCMFrames, sizeof(float));
        if (NGDPCMSlots[i].samples) anySlot = YES;
        else NGDPCMAllocationFailures.fetch_add(1, std::memory_order_relaxed);
        NGDPCMSlots[i].sourceID.store(0, std::memory_order_relaxed);
        NGDPCMSlots[i].frameCount.store(0, std::memory_order_relaxed);
        NGDPCMSlots[i].format = {};
    }
    if (!anySlot) {
        os_unfair_lock_unlock(&NGDCaptureControlLock);
        return NO;
    }

    uint32_t sourceCount = NGDSourceCount.load(std::memory_order_acquire);
    for (uint32_t i = 0; i < sourceCount; i++) {
        NGDSource &source = NGDSources[i];
        source.captureSlot.store(-1, std::memory_order_relaxed);
        source.firstCallbackTicks.store(0, std::memory_order_relaxed);
        source.previousCallbackTicks.store(0, std::memory_order_relaxed);
        source.callbackIntervalCount.store(0, std::memory_order_relaxed);
        source.callbackIntervalTicks.store(0, std::memory_order_relaxed);
        source.minCallbackIntervalTicks.store(0, std::memory_order_relaxed);
        source.maxCallbackIntervalTicks.store(0, std::memory_order_relaxed);
        source.estimatedDroppedFrames.store(0, std::memory_order_relaxed);
        source.contentionDrops.store(0, std::memory_order_relaxed);
        source.unsupportedBuffers.store(0, std::memory_order_relaxed);
        source.unreadableFrames.store(0, std::memory_order_relaxed);
        source.captureSlotMisses.store(0, std::memory_order_relaxed);
        source.formatMismatchBuffers.store(0, std::memory_order_relaxed);
        source.pcmCapacityDroppedFrames.store(0, std::memory_order_relaxed);
        source.probeTicks.store(0, std::memory_order_relaxed);
        source.probeBlocks.store(0, std::memory_order_relaxed);
        source.maxProbeTicks.store(0, std::memory_order_relaxed);
    }

    uint64_t start = mach_continuous_time();
    uint64_t pcmStart = start + NGDNanosecondsToTicks((uint64_t)(warmupSeconds * 1.0e9));
    NGDCaptureStartTicks.store(start, std::memory_order_release);
    NGDPCMStartTicks.store(pcmStart, std::memory_order_release);
    NGDPCMStopTicks.store(pcmStart + NGDNanosecondsToTicks((uint64_t)(recordSeconds * 1.0e9)),
                         std::memory_order_release);
    NGDCaptureActive.store(true, std::memory_order_release);
    os_unfair_lock_unlock(&NGDCaptureControlLock);
    return YES;
}

void NGDAudioTapEndCapture(void) {
    NGDCaptureActive.store(false, std::memory_order_release);
    uint64_t zero = 0;
    NGDCaptureStopTicks.compare_exchange_strong(zero, mach_continuous_time(),
                                               std::memory_order_release,
                                               std::memory_order_relaxed);
}

void NGDAudioTapWaitForWriters(void) {
    for (int attempt = 0; attempt < 100 && NGDActiveWriters.load(std::memory_order_acquire) > 0; attempt++) {
        usleep(1000);
    }
}

uint32_t NGDAudioTapSlotCount(void) {
    return kDKCaptureSlots;
}

BOOL NGDAudioTapReadSlot(uint32_t index, NGDAudioTapSlot *out) {
    if (index >= kDKCaptureSlots || !out) return NO;
    int sourceID = NGDPCMSlots[index].sourceID.load(std::memory_order_acquire);
    uint64_t frames = NGDPCMSlots[index].frameCount.load(std::memory_order_acquire);
    if (sourceID <= 0 || frames == 0 || !NGDPCMSlots[index].samples) return NO;
    out->sourceID = (uint32_t)sourceID;
    out->kind = (uint32_t)sourceID <= kDKMaxSources ? NGDSources[sourceID - 1].kind : 0;
    out->samples = NGDPCMSlots[index].samples;
    out->frameCount = frames;
    out->sampleRate = NGDPCMSlots[index].format.mSampleRate;
    out->sourceFormat = NGDPCMSlots[index].format;
    return YES;
}

// ioData 的实测排布，以及它与缓存 ASBD 的说法是否一致。
//
// 两者本来就允许不一致：ASBD 来自 kAudioUnitScope_Input，描述的是红果推进去的格式；
// ioData 是 unit 填完之后交给硬件的那份。0.5.2-beta2 在设备上就是「ASBD 说交错、
// ioData 是 2 buffer × 1 声道」，按 ASBD 读会读错——下混因此改为只信这里的数字。
static NSDictionary *NGDBufferLayoutJSON(NGDSource *source) {
    uint32_t buffers = source->layoutNumberBuffers.load(std::memory_order_relaxed);
    if (buffers == 0) return @{};   // 窗口内没有回调，没量到

    uint32_t channels = source->layoutChannelsPerBuffer.load(std::memory_order_relaxed);
    uint32_t byteSize = source->layoutDataByteSize.load(std::memory_order_relaxed);
    uint32_t renderFrames = source->layoutRenderFrames.load(std::memory_order_relaxed);

    AudioStreamBasicDescription format = {};
    BOOL hasFormat = NGDReadSourceFormat(source, &format);
    uint32_t bytesPerSample = hasFormat ? format.mBitsPerChannel / 8 : 0;
    uint32_t stride = channels * bytesPerSample;
    uint32_t framesPerBuffer = stride ? byteSize / stride : 0;

    // ASBD 声称非交错就该是「每 buffer 一个声道」，声称交错就该是「单 buffer 承载全部声道」。
    BOOL declaredNonInterleaved = hasFormat &&
        (format.mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0;
    BOOL actualNonInterleaved = buffers > 1 || (channels == 1 && format.mChannelsPerFrame > 1);

    return @{
        @"numberBuffers": @(buffers),
        @"channelsPerBuffer": @(channels),
        @"dataByteSizePerBuffer": @(byteSize),
        @"framesPerBuffer": @(framesPerBuffer),
        @"renderFrames": @(renderFrames),
        @"coversRenderFrames": @(framesPerBuffer >= renderFrames),
        @"declaredNonInterleaved": @(declaredNonInterleaved),
        @"actualNonInterleaved": @(actualNonInterleaved),
        @"matchesDeclaredFormat": @(declaredNonInterleaved == actualNonInterleaved),
    };
}

NSArray<NSDictionary *> *NGDAudioTapBackendsJSON(void) {
    uint64_t captureStart = NGDCaptureStartTicks.load(std::memory_order_relaxed);
    NSMutableArray<NSDictionary *> *result = [NSMutableArray array];
    uint32_t count = NGDSourceCount.load(std::memory_order_acquire);
    for (uint32_t i = 0; i < count; i++) {
        NGDSource *source = &NGDSources[i];
        AudioStreamBasicDescription format = {};
        BOOL hasFormat = NGDReadSourceFormat(source, &format);

        NSMutableArray *history = [NSMutableArray array];
        uint32_t records = std::min<uint32_t>(source->formatRecordCount.load(std::memory_order_acquire),
                                              kDKMaxFormatRecords);
        for (uint32_t r = 0; r < records; r++) {
            NGDFormatRecord &record = source->formatRecords[r];
            uint32_t before = record.version.load(std::memory_order_acquire);
            if (before == 0 || (before & 1)) continue;
            uint32_t scope = record.scope;
            uint64_t ticks = record.ticks;
            AudioStreamBasicDescription recorded = record.format;
            if (record.version.load(std::memory_order_acquire) != before) continue;
            [history addObject:@{
                @"scope": @(scope),
                @"ticks": @(ticks),
                @"second": @(NGDRelativeSeconds(ticks, captureStart)),
                @"asbd": NGDASBDJSON(recorded),
            }];
        }

        uint64_t intervalCount = source->callbackIntervalCount.load(std::memory_order_relaxed);
        uint64_t intervalTicks = source->callbackIntervalTicks.load(std::memory_order_relaxed);
        uint64_t probeBlocks = source->probeBlocks.load(std::memory_order_relaxed);
        uint64_t probeTicks = source->probeTicks.load(std::memory_order_relaxed);
        [result addObject:@{
            @"sourceID": [NSString stringWithFormat:@"source-%02u", source->sourceID],
            @"kind": NGDSourceKindName(source->kind),
            @"handle": NGDPointerString(source->handle.load(std::memory_order_relaxed)),
            @"active": @(source->active.load(std::memory_order_relaxed) != 0),
            @"disposed": @(source->disposed.load(std::memory_order_relaxed) != 0),
            @"renderNotifyInstalled": @(source->notifyInstalled.load(std::memory_order_relaxed) != 0),
            @"renderNotifyStatus": @(source->notifyStatus.load(std::memory_order_relaxed)),
            @"componentType": NGDFourCC(source->componentType),
            @"componentSubType": NGDFourCC(source->componentSubType),
            @"componentManufacturer": NGDFourCC(source->componentManufacturer),
            @"format": hasFormat ? NGDASBDJSON(format) : @{},
            @"formatHistory": history,
            @"bufferLayout": NGDBufferLayoutJSON(source),
            @"hits": @(source->hits.load(std::memory_order_relaxed)),
            @"frames": @(source->frames.load(std::memory_order_relaxed)),
            @"lastStatus": @(source->lastStatus.load(std::memory_order_relaxed)),
            @"lastActivitySecond": @(NGDRelativeSeconds(source->lastTicks.load(std::memory_order_relaxed), captureStart)),
            @"firstCaptureCallbackSecond": @(NGDRelativeSeconds(source->firstCallbackTicks.load(std::memory_order_relaxed), captureStart)),
            @"captureSlot": @(source->captureSlot.load(std::memory_order_relaxed)),
            @"callbackTiming": @{
                @"intervalCount": @(intervalCount),
                @"meanMilliseconds": @(intervalCount ? NGDTicksToSeconds(intervalTicks / intervalCount) * 1000.0 : 0),
                @"minMilliseconds": @(NGDTicksToSeconds(source->minCallbackIntervalTicks.load(std::memory_order_relaxed)) * 1000.0),
                @"maxMilliseconds": @(NGDTicksToSeconds(source->maxCallbackIntervalTicks.load(std::memory_order_relaxed)) * 1000.0),
                @"estimatedDroppedFrames": @(source->estimatedDroppedFrames.load(std::memory_order_relaxed)),
            },
            @"contentionDrops": @(source->contentionDrops.load(std::memory_order_relaxed)),
            @"unsupportedBuffers": @(source->unsupportedBuffers.load(std::memory_order_relaxed)),
            @"unreadableFrames": @(source->unreadableFrames.load(std::memory_order_relaxed)),
            @"captureSlotMisses": @(source->captureSlotMisses.load(std::memory_order_relaxed)),
            @"formatMismatchBuffers": @(source->formatMismatchBuffers.load(std::memory_order_relaxed)),
            @"pcmCapacityDroppedFrames": @(source->pcmCapacityDroppedFrames.load(std::memory_order_relaxed)),
            @"configurationOverflow": @(source->configurationOverflow.load(std::memory_order_relaxed)),
            @"probeBlocks": @(probeBlocks),
            @"probeAverageMicroseconds": @(probeBlocks ? NGDTicksToSeconds(probeTicks / probeBlocks) * 1000000.0 : 0),
            @"probeMaxMicroseconds": @(NGDTicksToSeconds(source->maxProbeTicks.load(std::memory_order_relaxed)) * 1000000.0),
        }];
    }
    return result;
}

NSArray<NSDictionary *> *NGDAudioTapSymbolCoverageJSON(void) {
    uint64_t captureStart = NGDCaptureStartTicks.load(std::memory_order_relaxed);
    NSMutableArray<NSDictionary *> *result = [NSMutableArray arrayWithCapacity:NGDSymbolCount];
    for (uint32_t i = 0; i < NGDSymbolCount; i++) {
        NGDSymbolCoverage &coverage = NGDCoverage[i];
        struct ngd_rebinding_status rebinding = {};
        BOOL hasStatus = ngd_rebinding_status_for_name(NGDSymbolNames[i], &rebinding) == 0;
        uintptr_t firstCaller = coverage.firstCaller.load(std::memory_order_relaxed);
        uintptr_t lastCaller = coverage.lastCaller.load(std::memory_order_relaxed);
        [result addObject:@{
            @"symbol": [NSString stringWithUTF8String:NGDSymbolNames[i]] ?: @"",
            @"originalAvailable": @(coverage.original != nullptr),
            @"rebindResult": @(coverage.rebindResult),
            @"symbolPointerMatches": @(hasStatus ? rebinding.symbol_matches : 0),
            @"successfulRebindings": @(hasStatus ? rebinding.successful_writes : 0),
            @"protectionFailures": @(hasStatus ? rebinding.protection_failures : 0),
            @"rebound": @(hasStatus && rebinding.successful_writes > 0),
            @"hitCount": @(coverage.hits.load(std::memory_order_relaxed)),
            @"firstSecond": @(NGDRelativeSeconds(coverage.firstTicks.load(std::memory_order_relaxed), captureStart)),
            @"lastSecond": @(NGDRelativeSeconds(coverage.lastTicks.load(std::memory_order_relaxed), captureStart)),
            @"firstCallerImage": NGDImageForAddress(firstCaller),
            @"lastCallerImage": NGDImageForAddress(lastCaller),
            @"firstCallerSymbol": NGDSymbolForAddress(firstCaller),
            @"lastCallerSymbol": NGDSymbolForAddress(lastCaller),
            @"lastStatus": @(coverage.lastStatus.load(std::memory_order_relaxed)),
        }];
    }
    return result;
}

NSArray<NSDictionary *> *NGDAudioTapEventRows(void) {
    uint64_t captureStart = NGDCaptureStartTicks.load(std::memory_order_relaxed);
    uint64_t captureStop = NGDCaptureStopTicks.load(std::memory_order_relaxed);
    NSMutableArray<NSDictionary *> *rows = [NSMutableArray array];
    uint32_t count = std::min<uint32_t>(NGDEventCount.load(std::memory_order_acquire), kDKMaxEvents);
    for (uint32_t i = 0; i < count; i++) {
        NGDEvent *event = &NGDEvents[i];
        if (!event->ready.load(std::memory_order_acquire)) continue;
        [rows addObject:@{
            @"ticks": @(event->ticks),
            @"secondsSinceCaptureStart": @(NGDRelativeSeconds(event->ticks, captureStart)),
            @"withinCapture": @(captureStart > 0 && event->ticks >= captureStart &&
                                (captureStop == 0 || event->ticks <= captureStop)),
            @"event": [NSString stringWithUTF8String:NGDEventName(event->kind)] ?: @"",
            @"sourceID": event->sourceID ? [NSString stringWithFormat:@"source-%02u", event->sourceID] : @"",
            @"status": @(event->status),
            @"callerImage": NGDImageForAddress(event->caller),
            @"callerSymbol": NGDSymbolForAddress(event->caller),
            @"value1": @(event->value1),
            @"value2": @(event->value2),
        }];
    }
    return rows;
}

NSDictionary *NGDAudioTapCountersJSON(void) {
    uint64_t contentionDrops = 0, estimatedDroppedFrames = 0, capacityDroppedFrames = 0;
    uint64_t captureSlotMisses = 0, unsupportedBuffers = 0, unreadableFrames = 0;
    uint64_t configurationOverflow = 0, formatMismatchBuffers = 0;
    uint32_t callbackSources = 0, notifySources = 0;
    double maxProbeMicroseconds = 0;
    uint32_t count = NGDSourceCount.load(std::memory_order_acquire);
    for (uint32_t i = 0; i < count; i++) {
        NGDSource &source = NGDSources[i];
        maxProbeMicroseconds = std::max(maxProbeMicroseconds,
            NGDTicksToSeconds(source.maxProbeTicks.load(std::memory_order_relaxed)) * 1000000.0);
        contentionDrops += source.contentionDrops.load(std::memory_order_relaxed);
        estimatedDroppedFrames += source.estimatedDroppedFrames.load(std::memory_order_relaxed);
        capacityDroppedFrames += source.pcmCapacityDroppedFrames.load(std::memory_order_relaxed);
        captureSlotMisses += source.captureSlotMisses.load(std::memory_order_relaxed);
        unsupportedBuffers += source.unsupportedBuffers.load(std::memory_order_relaxed);
        unreadableFrames += source.unreadableFrames.load(std::memory_order_relaxed);
        configurationOverflow += source.configurationOverflow.load(std::memory_order_relaxed);
        formatMismatchBuffers += source.formatMismatchBuffers.load(std::memory_order_relaxed);
        if (source.firstCallbackTicks.load(std::memory_order_relaxed) != 0) callbackSources++;
        if (source.notifyInstalled.load(std::memory_order_relaxed) != 0) notifySources++;
    }
    return @{
        @"registeredSources": @(count),
        @"sourceOverflow": @(NGDSourceOverflow.load(std::memory_order_relaxed)),
        @"renderNotifySources": @(notifySources),
        @"callbackSourcesDuringCapture": @(callbackSources),
        @"eventCount": @(std::min<uint32_t>(NGDEventCount.load(std::memory_order_relaxed), kDKMaxEvents)),
        @"eventOverflow": @(NGDEventOverflow.load(std::memory_order_relaxed)),
        @"activeWritersAfterStop": @(NGDActiveWriters.load(std::memory_order_relaxed)),
        @"pcmAllocationFailures": @(NGDPCMAllocationFailures.load(std::memory_order_relaxed)),
        @"contentionDrops": @(contentionDrops),
        @"estimatedDroppedFrames": @(estimatedDroppedFrames),
        @"pcmCapacityDroppedFrames": @(capacityDroppedFrames),
        @"captureSlotMisses": @(captureSlotMisses),
        @"unsupportedBuffers": @(unsupportedBuffers),
        @"unreadableFrames": @(unreadableFrames),
        @"configurationOverflow": @(configurationOverflow),
        @"formatMismatchBuffers": @(formatMismatchBuffers),
        @"probeMaxMicroseconds": @(maxProbeMicroseconds),
    };
}

NSArray<NSString *> *NGDAudioTapCallerImageNames(void) {
    NSMutableOrderedSet<NSString *> *names = [NSMutableOrderedSet orderedSet];
    for (uint32_t i = 0; i < NGDSymbolCount; i++) {
        NSString *first = NGDImageForAddress(NGDCoverage[i].firstCaller.load(std::memory_order_relaxed));
        NSString *last = NGDImageForAddress(NGDCoverage[i].lastCaller.load(std::memory_order_relaxed));
        if (first.length) [names addObject:first];
        if (last.length) [names addObject:last];
    }
    return names.array;
}

NSString *NGDAudioTapSourceIDForPointer(uintptr_t pointer) {
    if (!pointer) return nil;
    uint32_t count = NGDSourceCount.load(std::memory_order_acquire);
    for (uint32_t offset = 0; offset < count; offset++) {
        uint32_t i = count - offset - 1;
        if (NGDSources[i].handle.load(std::memory_order_relaxed) != pointer) continue;
        if (NGDSources[i].disposed.load(std::memory_order_relaxed) != 0) continue;
        return [NSString stringWithFormat:@"source-%02u", NGDSources[i].sourceID];
    }
    return nil;
}
