# 第三方代码与署名

## DYKiller

- 项目：[HierarchThurs/DYKiller](https://github.com/HierarchThurs/DYKiller)。
- 固定提交：[bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4](https://github.com/HierarchThurs/DYKiller/commit/bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4)。
- Copyright (c) 2026 Hierarch。MIT 完整文本：[third_party/DYKiller-LICENSE](third_party/DYKiller-LICENSE)。

底栏参考 src/Features/TabBar/DKGlassTabBar.xm、DKTabBarCollapse.xm、DKGlassTabBarSizeSettings.xm 和 src/Settings/DKAdjustRange.m；对应 src/NGDYDock.inc、NGDock.inc、NGDockAppearanceSettings.inc。保留原生材质、收缩 provider、62/21/94/4 点初值、12 点间距、48/12 点阈值、0.1 速度投影与尺寸范围；适配红果入口和原生搜索。

调试移植 src/Debug 页面采集/导出/扳手、音频探针/高层 Hook、媒体状态与可选 FLEXKing Provider；src/FLEX 类声明和 ZIP；src/Shared/DKAudioTap、DKAudioSignal。对应 src/Debug/NGD*。修改包括隔离类名/符号、产品及偏好键、Logos 转为保留 ARC 初始化语义的 Objective-C 包装。红果专属探针位于 src/NGDebugHostBridge.inc，未照搬抖音业务探针。

## fishhook

- 原项目：[facebook/fishhook](https://github.com/facebook/fishhook)。
- Copyright (c) 2013, Facebook, Inc. All rights reserved.
- 使用 DYKiller 随附 src/Shared/ThirdParty/fishhook.c/.h 修改版，进一步隔离 ngd_ 符号，位于本项目 src/Debug/fishhook.c/.h。
- 保留分发条件和免责声明：[third_party/fishhook-LICENSE](third_party/fishhook-LICENSE)。其许可独立于本项目 MIT。

## NaiwaEase 界面参考

赞赏与交流页面的卡片布局参考作者提供的 NaiwaEase 0.0.22 本地交接包，尚无已确认的公开仓库链接。使用 UIKit 重新实现规则玻璃赞赏卡、三张交流群邀请卡和系统分享，并保留本项目赞赏码与链接；不包含参考包的授权服务、账号逻辑、赞赏素材或混淆代码。

## 素材

不包含宿主 App 二进制、原始个人采集、账号凭据或第三方字体。测试图标及演示背景由本项目生成，公开截图标注为模拟器预览。赞赏码由项目作者提供，软件许可不授予冒用其收款身份的权利。引用不表示参考项目作者背书。
