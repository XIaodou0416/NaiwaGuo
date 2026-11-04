# 红果接口适配

下列接口从用户提供的红果 7.4.3 本地可执行文件的 Objective-C 元数据核对。仓库不分发原二进制或完整导出。

- SSShortVideoRightInteractionLikeView.model → SSShortVideoRightInteractionLikeModel.isLiked / setIsLiked:。
- SSShortVideoRightInteractionCollectView.isCollected / setIsCollected:，以及其 model 的 isCollected / setIsCollected:。
- 视图更新入口：setModel:、refreshUIWithModel:、refreshUIWithModel:animated:；运行时存在才挂钩。
- 搜索入口：SSVideoSeriesFeedViewController.simpleSearchViewDidClicked，无参数、void 返回；调用前核对签名，使用宿主原控制器，不猜 URL scheme。
- 黑角基线为成对 13×13 UIImageView；合集扩展覆盖额外容器与晚初始化。没有新的合集真机采集，不宣称所有宿主变体已验证。

底栏复用 UIKit 私有 platter/glassEffect/visualProvider 接口，均检查类、属性或 selector 可用性；不可用时保留常规展开材质/状态，不假装收缩已成功。
