# UCC_CooldownEffect

父类：`UGameplayEffect`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ConfiguredAbility.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ConfiguredAbility.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

内置 GameplayEffect，作用属性：无属性修改。持续，通过动态授予的槽位标签阻止重复激活。

## 配置方式

通常由 CC_ConfiguredAbility 自动应用，不必逐项创建蓝图。自定义 GA 可选此 GE 类创建 Spec，由攻击者 ASC 生成上下文，再应用给目标 ASC。

## 参数与接口

- SetByCaller CCTags.Skill.Duration：有效持续秒数。
- Spec.DynamicGrantedTags：由执行器加对应 Slot1～Slot4 冷却标签；没有标签时不能阻止槽位。

## 最小验证

通过对应技能测试属性／冷却变化；有持续期的效果到期后应移除。

## 当前限制

不要在自定义 GA 和内置执行器中重复应用，避免双次扣费、伤害或增益。
