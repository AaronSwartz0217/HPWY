# UCC_AttributeSet

父类：`UAttributeSet`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/CC_AttributeSet.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/CC_AttributeSet.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

角色 GAS 属性及伤害结算，包含复制通知和属性初始化事件。

## 配置方式

由 PlayerState／EnemyCharacter 创建，不单独拖入场景。初始值通过角色的 InitializeAttributesEffect 设置；形态及永久加成通过 Progression 重新计算基础值。

## 参数与接口

- Health、MaxHealth、Mana、MaxMana：生命和法力。
- Attack：实际攻击点数，白板为 15。技能通过 PowerMode 决定固定值或攻击倍率，不再在 IncomingDamage 中额外乘攻击百分比。Defense：系数 100/(100+Defense)。
- Shield：先吸收伤害。MoveSpeed：变化后同步到 CharacterMovement.MaxWalkSpeed。
- IncomingDamage：正数伤害元属性，应用后清零，不是角色可长期保存的属性。
- OnAttributesInitialized：初始化通知。
- 同一轮死亡只发一次 KillScored，并为有效玩家击杀生成一波经验粒子，不直接 AddExperience；生命恢复为正后重新允许下一轮奖励。
- 粒子由击杀玩家 PlayerState 领取，不绑定敌人尸体；重复致死不会重复生成。

## 最小验证

检查正伤害进入 IncomingDamage 后按攻击、防御、护盾顺序结算；旧 GE 直接改 Health 不走该完整链路。

## 当前限制

只负责属性／奖励触发；Death 动画转布娃娃和敌人尸体清理由 CC_BaseCharacter 管理，正式复活时间与位置由上层规则决定。直接修改 Blueprint 显示变量不会修改 GAS 数值。
