# ACC_ProgressionHUD

父类：`AHUD`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ProgressionHUD.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ProgressionHUD.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

显示经验、当前形态、四技能冷却和测试商品的 AHUD。

## 配置方式

GameMode.HUDClass 填 CC_ProgressionHUD。若要测试数字键和 F 键，再在控制器开启 ShowProgressionPanel。无需创建 Widget Blueprint。

## 参数与接口

- F1 进化、F2 原生、F3～F6 器物、F7～F12 商品。
- 数据来自 PlayerState.Progression 与 Pawn.Skills。
- 纯文字显示生命／最大生命、护盾、法力／最大法力、攻击、防御、移速；准星指向器物时显示交互提示，指向角色时显示目标生命。

## 最小验证

打开 L_HPWH 应看到测试文字，购买后余额和档位更新。

## 当前限制

不是正式可点击商城，未实现图标、鼠标交互、失败弹窗和美术布局；当前显示四个器物、六个商品。

## 关闭原型文字面板

`bShowDebugPanel`（Crash|UI）默认 true。正式 HUD 蓝图可继承此类并设 false，然后 BeginPlay 创建自定义 WBP_GameHUD、AddToViewport 一次。此开关仅控制原型 DrawHUD 面板，不隐藏角色 HealthDisplay 或 Tab 菜单。也可在正式 GameMode 选择自己的 HUD 类。

[按步骤接入 UMG](../UI操作与UMG接入配置.md)。

## 左下角拾取提示（2026-10-24）

当前 `BP_MuralHUD` 的 `PickupPromptTexture` 已指定 `/Game/UI/MuralMenu/Textures/Interaction/T_Prompt_Pickup`。使用提供的 E 拾取原图，进入有效交互范围自动显示在左下角，离开范围、被墙遮挡、打开菜单或角色死亡时隐藏；已消耗经验包不再触发。提示不受 `bShowDebugPanel` 开关影响，经验余额仍只在背包显示。

`IsPickupPromptVisible()` 与 E 键共用控制器 `FindInteractionTarget()`，可供未来 UMG 替换时读取。图片只提示可交互，不改变交互后存包、切换与进化流程。改键后保留提示图并额外显示实际按键；需完全统一图中文字时更换该纹理。

原图保存在 `SourceArt/UI/E_Pickup.png`；重复导入使用 `Scripts/configure_pickup_prompt.py`，不依赖临时剪贴板文件。
