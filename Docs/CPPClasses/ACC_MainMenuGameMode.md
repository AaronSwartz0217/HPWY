# ACC_MainMenuGameMode

父类 `AGameModeBase`，独立首页 `/Game/UI/MainMenu/Maps/L_MainMenu` 使用的模式。

构造时指定 `ACC_MainMenuController`，默认Pawn为空，HUD使用不绘制战斗UI的原生AHUD；避免空HUD类导致SpawnActor警告。`RestartPlayer` 不生成角色。由 `UCC_AdventureGameInstance.OverrideGameModeClass` 在首页路径返回此类，三个游戏地区另走 Adventure 模式。页面本身不激活战斗或保存角色进度。

可派生蓝图调整 Controller，但当前运行时入口返回原生主页模式，采用派生模式时还需同步 GameInstance 的返回类型。不要直接把游戏框架的 Pawn/HUD 填到首页。

[主页配置](../游戏主页与背景音乐配置.md) · [索引](../CPP父类配置索引.md)
