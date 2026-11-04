# 安装 NaiwaGuo 0.0.4

GitHub 不提供 deb 或包含 deb 的整包。以下 deb 文件由本地构建生成；公开发布页提供源码、dylib/plist 与对应校验和。

- rootful：com.naiwaguo.native_0.0.4_iphoneos-arm.deb。
- rootless：com.naiwaguo.native_0.0.4_iphoneos-arm64.deb。
- 签名注入：本版 NaiwaGuo.dylib 与 NaiwaGuo.plist；本次不提供重签宿主 IPA。

覆盖旧版后完全退出红果，再重新打开。旧字体选项保留，修复直接生效；可在字体设置中恢复默认。底栏新增设置位于“液态外观 → 底栏外观与收缩”。

原版本号曾按要求从 0.0.1 重置；从旧 0.7.x 安装时包管理器可能显示为降级。无需删除红果或清空数据。

液态底栏需要 iOS 26+，自动收缩只在 iPhone 启用。请依据 SHA256SUMS.txt 核验；发布版尚未在用户真机确认。
