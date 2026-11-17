# UCC_GameplayAbility

父类：`UGameplayAbility`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/Abilities/CC_GameplayAbility.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/Abilities/CC_GameplayAbility.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

教程原有 GA 基类，主要在激活时显示可选调试信息。

## 配置方式

维护已有 GA 蓝图时选它或其派生父类；新 20 技能配置优先用 CC_ConfiguredAbility。蓝图自行处理激活、消耗、应用效果和结束。

## 参数与接口

- Draw Debugs：是否显示激活调试信息。
- 普通 GameplayAbility 的策略、标签、触发器等仍按蓝图能力需要配置。

## 最小验证

激活旧技能时开调试应显示消息；完成后确认 EndAbility 被调用。

## 当前限制

不会自动补上新四槽的法力／冷却，也不会自动读取 SkillDefinition。
