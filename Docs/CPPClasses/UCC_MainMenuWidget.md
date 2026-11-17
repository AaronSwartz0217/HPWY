# UCC_MainMenuWidget

父类 `UUserWidget`，游戏首页控件，与 Tab 菜单分开。

`ArtData` 默认软引用 `/Game/UI/MainMenu/DA_HomeArt`，加载背景、Logo 和 `WebsiteURL`。原生布局在 `RebuildWidget` 阶段创建，保证首次 Slate 根控件有效；已有 Designer 根控件时保留自定义布局。首页只有开始游戏、宣传网站、退出游戏。

宣传站地址为 `https://aaronswartz0217.github.io/Spirit-Tales/`。主页三个按钮保持单行、内容槽水平铺满，在1920×1080设计画布内等比缩放，避免“开始游戏”等文字拆行。

| 蓝图接口 | 作用 |
|---|---|
| `StartGame()` | 调用 GameInstance 的受校验主页进入流程，默认去休憩之地 |
| `OpenWebsite()` | 点击后打开 ArtData 中的 HTTP/HTTPS 网站，不弹出联系窗口 |
| `ExitGame()` | 调用引擎 QuitGame |

原生布局先显示3秒开场Logo：立即出现、保持2秒、淡出1秒。`ArtData.StartupLogo`引用黑底白线图，`IsStartupSplashActive()`供UI读取；通过GameInstance保证每次会话只显示一次。开场阶段禁用三个按钮和Enter开始，之后显示主页。另有全窗口加载遮罩，读取GameInstance的加载进度与错误；忙碌时禁用按钮。网站配置或浏览器启动失败在按钮下方显示文字。控件自身聚焦时Enter可开始；按钮获得焦点时采用按钮默认键盘行为。

派生 Designer 蓝图需要自行绑定上述三个接口，并按 GameInstance 的 `bTravelling / GetLoadingProgress / IsOpeningWorld / LastTravelError` 显示加载与反馈；原生私有子控件不会自动绑定 Designer 同名控件。

[主页配置](../游戏主页与背景音乐配置.md) · [索引](../CPP父类配置索引.md)
