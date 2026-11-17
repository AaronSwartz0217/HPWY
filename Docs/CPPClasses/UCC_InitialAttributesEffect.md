# UCC_InitialAttributesEffect

父类：`UGameplayEffect`。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ConfiguredAbility.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ConfiguredAbility.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

为测试角色提供明确、可重复的完整初始属性，避免空的教程 GE 导致敌人生命为 0 或玩家法力为 0。

## 配置方式

在角色蓝图 Class Defaults 的 Initialize Attributes Effect 和 Reset Attributes Effect 选择 CC_InitialAttributesEffect。示例玩家、敌人与靶子已配置。无需为它额外建蓝图。

## 参数与接口

瞬时 Override：MaxHealth、Health、MaxMana、Mana 各为 100；Attack、Defense、Shield 为 0；MoveSpeed 为 500。先初始化最大值，再写当前值。

角色初始化后，玩家形态成长会再应用 NativeForm／当前器物阶段属性。希望不同怪物拥有不同基础属性时，应派生／配置自己的有效初始化 GE，而不是在 UI 填一个显示用常量。

## 最小验证

进入平地场，人物靶子和敌人头顶显示 HP 100 / 100，玩家法力显示 100 / 100；伤害后显示随真实 GAS 属性变化。

## 当前限制

这是固定的默认初值，不接受 SetByCaller，也不是存档恢复效果。它不负责复活位置、动画或清除全部持续状态。
