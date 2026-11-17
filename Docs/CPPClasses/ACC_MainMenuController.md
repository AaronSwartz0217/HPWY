# ACC_MainMenuController

父类 `APlayerController`，由 `ACC_MainMenuGameMode` 生成，独立于游戏内控制器。

`BeginPlay` 只为本地控制器创建 `MainMenuClass`，未指定时使用原生 `UCC_MainMenuWidget`。加入 viewport 后显示鼠标、设置 UIOnly 输入并聚焦首页，触发 GameInstance 的背景音乐确保入口。`EndPlay` 移除菜单，避免切图后残留首页。

`MainMenuClass` 是可替换 Widget 类，`GetMainMenu()` 是蓝图获取实例的接口。派生 Controller 后需由对应主页 GameMode 使用；当前原生主页模式默认采用此类。没有玩家移动、攻击或开发按键绑定。

[主页配置](../游戏主页与背景音乐配置.md) · [索引](../CPP父类配置索引.md)
