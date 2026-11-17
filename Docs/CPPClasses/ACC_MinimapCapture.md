# ACC_MinimapCapture

父类AActor，本地第二摄像机，由当前PlayerController创建。

- Capture：正交SceneCapture2D，固定朝北俯视并跟随角色位置。
- RenderTarget：运行时创建，供UI Image/材质使用。
- Resolution=256、RefreshRate=10Hz、ViewWidth=3000厘米、CaptureHeight=2500厘米。
- SetCaptureEnabled：隐藏地图时关闭Actor/组件捕获；不使用每帧或移动自动捕获。
- MinimapHide Actor标签：创建时排除屋顶、天空卡片等。

关掉光照、阴影、后处理、雾、反射和AO。使用FinalColorLDR保证普通UMG Image可显示的Alpha，无光照由ShowFlags控制。默认无需新材质或渲染目标资产。

需要改相机参数时建派生蓝图，在BP_ProgressionController.MinimapClass指定。非网络复制摄像机；本轮由前端验证实际画面和功耗。[完整配置](../地图传送与小地图配置.md)。
