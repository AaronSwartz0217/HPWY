# UCC_AbilitySystemComponent

父类：`UAbilitySystemComponent`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/CC_AbilitySystemComponent.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/CC_AbilitySystemComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

自定义 ASC，支持授予时按标签自动激活和按能力类修改等级。

## 配置方式

玩家／怪物 C++ 已创建，无需额外挂载。需自动启动的公共能力配置 Asset Tag CCTags.CCAbilities.ActivateOnGiven；新四槽使用 SkillComponent 的技能定义与等级管理。

## 参数与接口

- SetAbilityLevel(AbilityClass,Level)：权威端直接设置匹配 Spec 等级。
- AddToAbilityLevel(AbilityClass,Level)：权威端增加等级。
- OnGiveAbility／OnRep_ActivateAbilities：处理自动激活。

## 最小验证

怪物授予 CC_SearchForTarget 后应自动开始搜索；公共被动可按同一标签测试。

## 当前限制

按 AbilityClass 查找不适合区分共用 CC_ConfiguredAbility 类的 20 个技能；新技能等级购买走 SkillId。现有等级 API 不自动检查商品价格或技能等级下限。
