# UCC_ProgressionMenu

父类UUserWidget。仅在CC_PlayerController.ProgressionMenuClass未配置时按Tab创建，作为原生文字回退。当前正式配置指向/Game/UI/MuralMenu/WBP_MuralMenu，父类为UCC_MuralMenuWidget；本类不与正式菜单同时创建，也不应被当作当前主菜单。架构审计保留此回退入口，待正式UI稳定后再决定是否移除。

- 背包：显示 PlayerState.Progression 中的当前经验余额。
- 进化：显示当前力量、初／中／高级、下一档费用和余额，按钮发服务器 Evolve 请求。
- 余额不足禁用购买，服务器再次校验；结果文本显示成功、余额不足或满级。
- Tab、Esc、退出界面按钮均返回游戏，不退出程序。菜单打开时显示鼠标，拦截移动、视角、攻击、技能和测试购买快捷键；世界继续运行，不暂停怪物。
- NativeOnInitialized 创建控件树；NativeDestruct 解除购买结果监听。

输入和创建入口：[CC_PlayerController.cpp](../../Source/CrashCourse/Private/Player/CC_PlayerController.cpp)。后续可将布局替换为美术 UMG，但应保留服务器购买接口。

## 2026-10-24 数值与技能配置更新

新增技能页与设置页：全部技能目录、效果与数值简介、选槽装备 / 恢复形态默认、六项战斗按键重绑、本地存档按钮。顶部入口和配置控件固定，只有详情区域滚动。数值说明从 SkillDefinition 实时生成。KeySelector 更新显示使用重入保护，避免切换动作误写入键位。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。

## 正式菜单替换边界

此类继续作为原生文字回退菜单；它在 NativeOnInitialized 构建控件树，不适合作为设计器美术主菜单的父类。正式 WBP_GameMenu 直接继承 UserWidget，配置到 PlayerController.ProgressionMenuClass，并调用同一 Progression / Skills / 输入 API。现有文字菜单已监听 OnSkillConfigurationFinished，装备失败/成功不再靠提交请求时猜测。

[按步骤接入 UMG](../UI操作与UMG接入配置.md)。
