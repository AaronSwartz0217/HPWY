# UCC_ShieldEffect

父类：`UGameplayEffect`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ConfiguredAbility.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ConfiguredAbility.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

内置 GameplayEffect，作用属性：Shield。瞬时 Override 替换护盾，到期清除由 C++ ApplyEffect 的专门计时逻辑负责。

## 配置方式

通常由 CC_ConfiguredAbility 自动应用，不必逐项创建蓝图。自定义 GA 可选此 GE 类创建 Spec，由攻击者 ASC 生成上下文，再应用给目标 ASC。

## 参数与接口

- SetByCaller CCTags.Skill.Value：修改量，不要遗漏或使用其他 DataTag。
- 定时攻击／防御／速度效果另需 CCTags.Skill.Duration，单位秒。
- 这些类共用 CC_ConfiguredAbility.h/.cpp，不是每类独占源文件。

## 最小验证

使用内置 Shield 技能验证护盾替换、吸收和到期；直接应用 GE 时另行处理到期。

## 当前限制

直接在普通蓝图 ApplyGameplayEffect 应用此 GE 不会执行 C++ 护盾计时器；不能把瞬时 GE 当成自带限时护盾。
