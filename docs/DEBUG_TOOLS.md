# 调试工具

0.0.3 使用 DYKiller 同源导出引擎、扳手、类声明导出和音频旁路，替换旧用户调试流程。红果专属字体、玻璃、底栏与 Hook 状态由 NaiwaGuo 适配，不照搬抖音专属业务探针。

## 使用

开启“工具与诊断 → 调试工具”。扳手可拖动，空白触摸穿透；点击显示两种导出，长按打开设置。关闭时隐藏扳手并停止采样，已安装包装器透明转发。

音频需要开启后彻底退出并重启红果，覆盖后端创建。采集不主动播放/暂停视频、改变音量或路由，不打开麦克风录音。

## 页面 ZIP

| 文件 | 内容 |
| --- | --- |
| manifest.json | 插件/宿主/系统版本、设备模型、时间、文件数、完整性与错误。 |
| ui/screenshot.png | 可见应用窗口合成截图，排除调试自身。 |
| page/windows.json | 窗口尺寸、层级、显隐、根控制器及 key 状态。 |
| page/view-tree.json / .txt | 类名/地址、层级、frame/bounds、显隐/透明度、颜色、交互、裁剪；文字/字体、按钮选中状态、图片尺寸、滚动与材质。 |
| page/view-controllers.txt | 导航、Tab、子控制器和弹出关系。 |
| page/layers.json | 视图所属图层的坐标、透明度、圆角、mask 和变换；不是任意 CALayer 子树的完整递归转储。 |
| page/selected-view.json | 当前整页入口以窗口为目标；不是交互式元素拾取器。 |
| page/classes/*.h | 本页视图/图层/控制器及继承链中的运行时类声明：属性、成员变量名称/类型、协议和方法签名。没有方法实现源码。 |
| probe/tabbar.txt | 红果当前页面、玻璃、底栏材质/收缩/尺寸、字体设置、当前 Hook 实现位置。 |

## 音频专项 ZIP

固定五秒：**一秒稳定 + 三秒 PCM + 一秒收尾**。用户选择“播放中 / 已暂停 / 静音播放中”，期间保持状态。

- audio/session.json：前后 AVAudioSession 类别、模式、采样率、缓冲时长、延迟、声道、音量与路由类型。
- audio/backends.json：AudioUnit / AudioQueue / 已有媒体 tap 的登记、格式、生命周期与旁路统计。
- audio/timeline.jsonl：底层和 AVFoundation 事件时间线。
- audio/signal.jsonl：30 Hz RMS、峰值、频谱带数据。
- audio/media-snapshots.json：0、2.5、5 秒媒体状态；用户声明与实际观测分别记录。
- audio/glass-target.json：胶囊、搜索圆钮、祖先裁剪、安全区和屏幕几何。
- audio/diagnostics.json：符号与 ObjC Hook 覆盖、丢帧/溢出/格式异常、信号自检及警告。
- audio/pcm/*.wav 与对应 JSON：实际捕获的单声道 Float32 PCM，最多保留能量最高四路；无可读 PCM 不伪造文件。
- probe/audio-summary.txt：可读摘要，另带页面截图和红果探针。

## 能力边界与隐私

类声明不能还原方法实现，也不递归读取任意对象的私有数据值。可选 FLEXKing 只读 Provider 暴露当前状态/当前 IMP，不恢复历史 Hook 链。FLEXKing 不是必需依赖，未安装时跳过注册。

这不是网络抓包器或自动卡死分析器；主线程完全卡死时页面采集可能无法启动，需要复现过程或系统采样。系统媒体服务、共享缓存直连及宿主自有后端可能取不到 PCM；零命中不能据此推断宿主未调用。

结果只在本机生成，经系统分享面板由用户选择去向，不自动上传；分享完成或取消后清理临时文件，打包失败保留现场并报告原因。截图、文字和播放声音可能含私人内容，请先检查再分享，不把真实采集 ZIP 提交到公共仓库。

## 参考

- [DYKiller 调试目录](https://github.com/HierarchThurs/DYKiller/tree/bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4/src/Debug)
- [DYKiller 音频取样](https://github.com/HierarchThurs/DYKiller/blob/bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4/src/Shared/DKAudioTap.mm)
- [facebook/fishhook](https://github.com/facebook/fishhook)
