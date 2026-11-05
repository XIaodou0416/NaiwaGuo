# NaiwaGuo

红果短剧 iOS 原生界面定制插件。简洁设置、跟随系统浅深色、彩色功能图标与黄绿色控件点缀。

**当前版本：0.0.5 · 预发布，真机效果待补充。** 公开版本从 0.0.1 开始编号。基于红果 7.4.3 接口适配，目标为 com.phoenix.video / Eggplant。

[下载版本](https://github.com/XIaodou0416/NaiwaGuo/releases) · [安装说明](INSTALL.md) · [验证记录](docs/VALIDATION.md) · [免责声明](DISCLAIMER.md)

## 插件功能

| 模块 | 可以设置什么 |
| --- | --- |
| 播放布局 | 显示/隐藏头像、右侧动作、顶部标签与视频信息；调整位置和大小；竖屏全屏；宽屏与方形视频保留原尺寸和位置；系统状态栏交还宿主管理。 |
| 液态底栏 | DYKiller 同源原生胶囊与独立圆钮；普通/清透液态；上滑收缩到左侧、右侧或中间，下滑或点击展开；可显示红果账号头像。 |
| 底栏定制 | 文字、图标、图文三种显示；胶囊高度、长度、独立圆钮大小；图标、选中/未选中颜色及项目显隐；按键触动反馈开关；独立按钮可选搜索、首页、剧场、商城、我的或插件设置。 |
| 播放页液态 | 顶部频道、合集返回、倍速与更多、全集入口、右侧按钮、评论区、分享/长按/倍速面板及评论玻璃透明度。 |
| 图标定制 | 原版与四套动作图标；收藏/点赞按真实状态变化；支持相册图片和 SF Symbols，自定义图标优先。 |
| 字体与字号 | 原生文字缩放 50%–100%；选择已安装字体、导入 TTF/OTF/TTC（最大 50 MB）；一键恢复。 |
| 设置搜索 | 搜索功能后直达对应页面，开关项滚动定位并高亮。 |
| 调试工具 | DYKiller 同源可拖动扳手、整页 ZIP、五秒音频专项 ZIP；红果探针与可选 FLEXKing 只读接口。 |
| 作者与交流 | “请作者喝杯奶茶吧”玻璃赞赏卡片与原始赞赏码；QQ / TG 独立邀请卡片。 |

首次使用的功能开关默认关闭，开启后保留选择。原生液态兼容层自动检测系统：**iOS 26+ 自动启用，低版本不启用**，无需手动开关；液态玻璃与底栏收缩需要 iOS 26+，收缩限 iPhone。最低部署目标为 iOS 15；宿主版本、注入方式与系统私有接口会影响兼容性。字体覆盖 UILabel、UITextField、UITextView、YYLabel，不覆盖视频内文字、网页及 Compose/DanceUI 自绘文字。

## 功能界面

以下是 **0.0.5 在 iOS 模拟器中实际运行的设置界面**，不代表红果真机效果。底栏演示使用合成背景。

<table>
<tr><td align="center">设置 · 浅色</td><td align="center">设置 · 深色</td><td align="center">底栏与收缩</td></tr>
<tr><td><img src="docs/previews/home-light.png" width="230" alt="设置浅色界面" /></td><td><img src="docs/previews/home-dark.png" width="230" alt="设置深色界面" /></td><td><img src="docs/previews/dock-settings.png" width="230" alt="底栏材质与收缩设置" /></td></tr>
<tr><td align="center">字体与字号</td><td align="center">右侧图标</td><td align="center">调试导出菜单</td></tr>
<tr><td><img src="docs/previews/fonts.png" width="230" alt="字体与字号设置" /></td><td><img src="docs/previews/icons.png" width="230" alt="右侧图标预设设置" /></td><td><img src="docs/previews/debug-menu.png" width="230" alt="本页与音频专项导出" /></td></tr>
<tr><td align="center">赞赏 · 浅色</td><td align="center">赞赏 · 深色</td><td align="center">交流群邀请</td></tr>
<tr><td><img src="docs/previews/support-light.png" width="230" alt="赞赏玻璃卡片浅色" /></td><td><img src="docs/previews/support-dark.png" width="230" alt="赞赏玻璃卡片深色" /></td><td><img src="docs/previews/community.png" width="230" alt="QQ群与TG邀请卡片" /></td></tr>
</table>

[更多底栏、收缩与交流入口截图](docs/SCREENSHOTS.md)

### 真机效果

**等待作者后续提供真机截图或录屏。** 收到后补充机型、iOS、红果及插件版本，再更新验证结论。目前没有把模拟器截图标成真机效果。

## 0.0.5 更新

- 首次使用的功能开关默认关闭；已有选择保留。液态兼容层按系统版本自动启用，iOS 26+ 开启，低版本不启用。
- 移除“隐藏系统状态栏”入口与 Hook，清理旧设置，让宿主管理状态栏。
- 新增底栏按键触动反馈开关，默认关闭；开启后点击导航或搜索按钮轻震，滚动、收缩和自动刷新不触发。
- 修复选中/未选中颜色实时更新，覆盖文字、图标、图文、收缩圆钮和搜索图标；自定义图片保持原色。
- 宽屏与方形视频保留宿主原尺寸和位置，只扩展底部预留区域；修复横向切换频道后黑边恢复，尊重评论、分享等面板的原生缩放。
- 新增倍速面板液态、居中收缩、红果账号头像及六种独立按钮功能。

代码和模拟宿主验证不替代红果真机效果，触感与真实视频后端仍待确认。

## 调试工具怎么用

1. 开启 **工具与诊断 → 调试工具**，出现可拖动扳手；长按扳手可回到插件设置。
2. 出现问题时，点扳手 → **导出本页 zip**，保存窗口、视图/控制器树、图层、类声明、截图及红果状态。
3. 排查声音时，开启调试后**彻底退出并重启红果**，选择 **导出音频专项 zip**，选择“播放中 / 已暂停 / 静音播放中”，保持五秒。
4. 导出走系统分享面板。**截图、界面文字与三秒播放声音可能含私人内容，请先检查再分享。** 插件不自动上传诊断包。

字段与限制见 [调试工具说明](docs/DEBUG_TOOLS.md)。没有取得可读 PCM 时会写明实际观测和覆盖情况，不伪造 WAV。

## 安装与开发

GitHub 发布页提供 dylib/plist、源码和校验和，不上传 deb 或包含 deb 的整包。deb 可在本地构建。不要同时加载多个版本；升级后完全退出红果再打开。详见 [安装说明](INSTALL.md)。

构建需要 macOS、带 iOS 26 或更新 SDK 的 Xcode、Python 3；测试需要 iPhone 模拟器。

~~~sh
bash scripts/build.sh
bash scripts/test-simulator.sh
bash scripts/test-defaults.sh
bash scripts/test-panel-motion.sh
bash scripts/test-search-navigation.sh
bash scripts/test-native-ui.sh
bash scripts/test-debug-tools.sh
~~~

测试优先选择已启动的 iPhone 模拟器，也可设置 NG_SIM_DEVICE。测试自带原创图标，默认不需要宿主 IPA；NG_TEST_IPA 仅供本机可选图标读取。构建与测试输出在 dist/，不提交宿主二进制和个人采集包。

## 参考项目与致谢

| 项目与链接 | 对应使用 |
| --- | --- |
| [HierarchThurs/DYKiller](https://github.com/HierarchThurs/DYKiller) | 固定提交 bb11ab1d9cba9733d6720f0e2746dec21d6c6fe4；液态底栏、自动收缩、尺寸/内容规则、调试导出、类声明、音频旁路和信号分析。MIT。 |
| [facebook/fishhook](https://github.com/facebook/fishhook) | 音频 C 符号重绑定，使用 DYKiller 随附的隔离版本，保留原版权和分发条件。 |

赞赏与交流群布局参考作者提供的 NaiwaEase 0.0.22 本地交接包；没有已确认的公开项目链接。使用本项目的原始赞赏码与群链接，未引入其授权或账号逻辑。

原文件、对应模块、改动及完整许可见 [第三方声明](THIRD_PARTY_NOTICES.md)。引用不表示参考项目作者为本项目背书。

## 请作者喝杯奶茶吧

自愿支持开发，不是购买功能，也不构成更新期限或兼容性承诺。插件内展示相同的原始赞赏码。

<img src="assets/sponsor-code.jpg" width="250" alt="作者提供的原始赞赏码" />

- [QQ 交流群](https://qm.qq.com/q/w9rb2DnAoE)
- [TG 通知群](https://t.me/beans0416)
- [TG 交流群](https://t.me/beans1011)

## 免责声明与许可

本项目是独立、非官方界面定制工具，与红果短剧及其运营主体没有官方合作或背书关系。请遵守适用法律、平台规则和第三方素材许可。软件按现状提供，不保证所有设备和版本可用；诊断包由使用者自行决定是否分享，请勿公开账号信息或私人内容。

详见 [免责声明](DISCLAIMER.md)。原创代码采用 [MIT License](LICENSE)，第三方代码遵循各自许可；本说明不额外收窄 MIT 授予的软件使用权。
