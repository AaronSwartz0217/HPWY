# 开场 Logo、配乐与材质提示处理

更新：2026-10-24。用户本轮已授权实际运行验证，但自动审批仍限制为编译与资源检查，拒绝启动游戏；本次未实际运行游戏。已预留隔离的PresentationSmokeUser目录及开发验证入口，日常角色存档保持不变。

## 3秒开场

默认启动关卡仍是 `/Game/UI/MainMenu/Maps/L_MainMenu`。首页先用提供的黑底白线 Logo 显示开场：直接出现，前2秒保持，最后1秒淡出，总计3秒。按第一次界面Tick的实时时钟计时，避免素材加载时提前消耗开场时间。黑底覆盖整个窗口，图片等比居中，淡出结束后露出原首页。

这3秒禁止点击开始、网站、退出和Enter开始；不会提前进入地图。音乐从开场就开始。GameInstance 的 `ConsumeStartupLogo()` 保证每个新游戏会话显示一次，切图或重建首页控件不重复。直接打开测试场Play属于开发入口，跳过首页开场。

原图保存 `SourceArt/UI/MainMenu/StartupLogo_v1.png`，导入为 `/Game/UI/MainMenu/Textures/T_Home_StartupLogo`，由 `DA_HomeArt.StartupLogo` 引用。替换该字段即可更换开场图；原首页Logo与背景独立维护。原生控件提供 `IsStartupSplashActive()` 供UI联调。

## 全游戏配乐替换

采用用户指定源：

`数媒交互文化主题项目模板/Content/正式关卡/文物展示关卡/游戏配乐萌新写的多利亚练习小曲儿_音频.uasset`

该资源由UE5.5工程迁入UE5.6，可直接读取，不需转码。独立播放资产为 `/Game/Audio/Music/SW_ExhibitionTheme`，时长约128.405秒，已启用循环。依赖检查仅为 `/Script/Engine`，无缺失的项目包。源文件SHA256为 `5338e7b2b33f35f828e1921d0deeaffca93bdd865c742194c156de2e02c8cd43`。

分类保存的源副本位于 `/Game/Audio/Music/ExhibitionSource`；原项目文件不改动。旧Starter音乐保留但不再作为默认配乐。GameInstance继续使用同一个跨图2D音频组件，默认音量0.25、淡入1.5秒，首页与三地区共用。

`Scripts/configure_background_music.py` 现导入此配乐，不会再恢复成LanTu的Starter_Music01。若DefaultGame.ini曾手动指定旧 BackgroundMusic，需要更新为新的完整软路径。

## 黄色提示与根因

最新用户日志显示 glTF默认材质 `/InterchangeAssets/gltf/M_Default` 缺少Nanite使用标记，以及旧敌人蓝图废弃的Damage Actor输入引脚。处理脚本 `Scripts/repair_travel_presentation.py` 检查模型槽位、组件覆盖及多层材质实例父链，复制相关插件父级到项目，使用同外观的项目副本 `/Game/LanTuRebuild/Materials/Compatibility/M_GltfDefault_Nanite`；不会保存Engine插件材质或替换自定义美术材质。共更新262个项目材质实例，建立3个兼容主材质和6个插件预设实例副本；1个含透明材质的马车网格停用Nanite。关卡二进制未改写。具体改动资产见[材质兼容修复清单](材质兼容修复清单.json)，本机过程报告为 `Saved/UI/TravelPresentationRepair.json`。

编辑器工具 `UCC_AnimationSetupLibrary.RepairLegacyWidgetPins` 只移除无连线、已废弃且名为DamageActor的引脚，再编译；本次从迁入BP_Enermyfree移除1个。首页模式使用空的原生AHUD，避免HUD类为空时SpawnActor产生警告，仍不绘制战斗HUD或生成角色。

运行画面默认不叠加引擎开发文字。`UCC_AdventureGameInstance.bShowRuntimeDebugMessages=false` 在会话开始时关闭屏幕调试消息，最后一个会话结束后恢复编辑器原状态。**日志继续记录全部警告和错误**；存档失败、地图加载失败等玩家需要知道的结果仍通过正式UI显示。这项显示设置不能修复材质本身或阻止真正崩溃。

排查时可在DefaultGame.ini加入：

```ini
[/Script/CrashCourse.CC_AdventureGameInstance]
bShowRuntimeDebugMessages=True
BackgroundMusic=/Game/Audio/Music/SW_ExhibitionTheme.SW_ExhibitionTheme
```

也可临时在开发控制台执行 `EnableAllScreenMessages`，关闭用 `DisableAllScreenMessages`；重开游戏按配置执行。

## 游戏图标

通过内置image_gen生成古金／青绿器物魂印与中心墨滴，原图1254×1254、黑底PNG，无文字。按指定位置保存为 `C:/Users/29717/Desktop/物魄奇谭2d资源/物魄奇谭_游戏图标_v1.png`，项目同时保留 `SourceArt/UI/GameIcon/SpiritTales_Icon_v1.png`。[原图和提示词记录](../SourceArt/UI/GameIcon/README.md)。本次仅提供图标图片，未替换现有Logo或可执行文件图标设置。

## 验证入口

编译日志：`Saved/Logs/PresentationBuild.log`。资源报告：`Saved/UI/GameHomeSetup.json`、`BackgroundMusicSetup.json`、`TravelPresentationRepair.json`。

只读复查脚本为 `Scripts/verify_presentation_assets.py`，报告 `Saved/UI/PresentationAssetVerification.json`；不进入PIE、不保存美术，只检查地图中Nanite网格材质、BGM默认值／循环及开场贴图。

新增仅开发构建控制台命令 `CC.PresentationSmoke`。它要求独立单机进程、首页和隔离用户目录；检查开场显示／淡出、配乐循环／切图复用、休憩原装置进入小镇、N接口前往测试场及当前GameMode；截图在 `Saved/Screenshots/Presentation`，结果在 `Saved/UI/PresentationSmoke.json`。本轮该命令未执行，未生成运行截图或通过报告；启动申请被自动审批两次拒绝，理由为当前仅允许编译和资源检查。最终结论仅覆盖编译、资产读取／保存和静态引用检查，不能以屏幕文字隐藏推断实机效果已通过。

最终只读资源复查通过：3张地图加载成功，85组Nanite网格／材质组合未发现问题；指定配乐、循环、开场贴图和默认调试文字开关6项检查均为true。新进程复查日志未再出现M_Default、M_Transmission、M_MS_Base_Masked的Nanite标记警告。角色存档哈希与开始前一致。此结论不包含实际游玩、听音和3秒开场的渲染截图验证。
