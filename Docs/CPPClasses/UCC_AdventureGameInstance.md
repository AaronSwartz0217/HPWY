# UCC_AdventureGameInstance

父类UGameInstance，DefaultEngine.ini的GameInstanceClass已注册。

负责三张地图的稳定Id/名称/软路径、运行时GameMode覆盖、首次抵达解锁、v4本地进度和跨图快照、资源预加载、MoviePlayer加载画面与失败恢复。仅对当前单机地区启用旅行流程。

- Regions：Rest/休憩之地、Town/小镇、Arena/山河社稷；Rest/Arena默认解锁。
- MainMenuMap：独立首页路径，默认 `/Game/UI/MainMenu/Maps/L_MainMenu`。
- ConsumeStartupLogo：同一游戏会话只允许一次首页3秒开场。
- bShowRuntimeDebugMessages：默认false，游戏会话期间隐藏屏幕调试文字，结束后恢复原状态，日志照常记录。
- IsMainMenuWorld / StartGameFromHome：主页进入休憩之地，恢复已有有效档或使用初始数据；坏档保留，加载失败恢复内存快照。
- BackgroundMusic / BackgroundMusicVolume / MusicFadeInSeconds：全局循环音乐软引用、音量和淡入时间。
- EnsureBackgroundMusic / SetBackgroundMusicVolume：确保唯一跨关卡 2D 音频组件、调整会话音量；Shutdown 时释放。
- VisitedRegions / CurrentRegion / IsRegionUnlocked：UI读取地区状态。
- RequestFastTravel：UI请求已解锁地区；真实抵达前不增加解锁记录。
- bTravelling / GetLoadingProgress / IsOpeningWorld / GetLoadingTitle / LastTravelError：加载界面入口。
- OverrideGameModeClass：首页返回MainMenu模式，三地区返回Adventure模式，无需重存美术关卡。
- RestoreSessionProgress / AdoptProgressSnapshot：原生存档交接；不会信任客户端钱包RPC。

Regions为Game配置属性，可由派生类/配置调整。更换路径需同步MapsToCook。初始房间和测试场默认解锁，小镇通过房间门户首次访问。地区与角色状态同存SpiritTales.sav v4；0/9开发会话暂停自动写档直到手动保存/读取。

本轮已通过UE5.6编辑器Development编译，运行测试由前端进行。[完整配置](../地图传送与小地图配置.md)。

[独立主页、配置网址与背景音乐](../游戏主页与背景音乐配置.md)。音乐配置可以写入DefaultGame.ini的 `/Script/CrashCourse.CC_AdventureGameInstance` 段；当前不增加音量设置页，也不持久化音量偏好。
