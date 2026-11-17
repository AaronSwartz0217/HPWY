# UCC_WorldNavigationWidget

父类UUserWidget。右下角小地图、M/N提示、三地区快捷传送及读取阶段加载遮罩。

默认自动构造古风金边布局，ArtData默认为DA_MenuArt。M显隐整个舆图，N/ESC开关行旅列表。方形地图中心方向标记读取角色Yaw。地区先选中查看简介与解锁条件，再点“启程传送”；未解锁/当前地区/角色未就绪时不可确认。切图时停止捕获并显示加载状态。

行旅面板使用620×566设计尺寸，地区按钮568×58、确认按钮568×56。按钮、标题与地图提示固定单行；简介和错误反馈按分配宽度换行。按钮内容槽水平Fill、垂直Center，防止分辨率缩放时文字被挤到下一行并越界。

小地图锚定实际 viewport 右下角 `(1,1)`，对齐 `(1,1)`，向内16个UMG逻辑单位，面板334×376。它独立于居中的1920×1080行旅设计画布，超宽屏也贴右下角；摄像机画面仍为256×256。

- InitializeNavigation：控制器注入本地相机与RenderTarget。
- ToggleMinimap / IsMinimapVisible：显隐状态。
- SetTravelOpen / IsTravelOpen：控件状态；正式开关调用控制器SetTravelMenuOpen，统一输入锁与鼠标。
- OnNavigationVisibilityChanged：Designer蓝图更新自定义控件显隐。
- SelectRegion / GetSelectedRegion：待传送地区选择，不直接切图。

在Controller.WorldNavigationClass配置派生Designer蓝图。已有Designer根控件时不生成原生布局；自行使用GameInstance地区/加载接口和Controller.GetMinimapCamera.RenderTarget绘制。

本轮只做编译检查，运行UI验证由前端进行。[UI配置](../地图与选项UI配置.md) · [旅行机制](../地图传送与小地图配置.md)。
