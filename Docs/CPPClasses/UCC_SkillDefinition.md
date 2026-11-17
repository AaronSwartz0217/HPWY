# UCC_SkillDefinition

父类：`UDataAsset`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_GameplayData.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_GameplayData.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途与创建

一个技能的身份、行为参数和表现配置。20 个模板是这类的 20 份资产；普攻另有 DA_BasicAttack。Content Browser → Miscellaneous → Data Asset → CC_SkillDefinition，或复制现有资产。先保留稳定 SkillId，修改 DisplayName，选择 Behavior、参数与 Presentation。通常 AbilityClass 留空。

## 规则参数

- SkillId 唯一且稳定，购买技能等级用它对应；DisplayName 可改。
- Behavior：Melee、Combo、Sweep、Dash、Projectile、Spread、Homing、Piercing、Burst、GroundStrike、DamageZone、LineStrike、Heal、AreaHeal、Shield、DefenseBuff、AttackBuff、SpeedBuff、Slow、Knockback。
- PowerMode：Flat 为固定点数；AttackMultiplier 为实际攻击力倍率。有效强度 `max(0, Power + (技能等级 - 1) × PowerPerLevel)`，倍率模式再乘施放时攻击力。
- Description：用途简介；GetNumericDescription(Attack, Level) 自动补充实际数值、冷却、消耗、距离等说明，技能面板直接调用。01 固定 .75 倍攻击，PowerPerLevel=0。
- ManaCost、Cooldown：内置消耗与槽位冷却；Range、Radius、Angle：选点/近战范围、区域半径/直线宽度、扇形/散射角。
- Duration、Interval、Count：持续时间、跳伤间隔/落地延迟、连击/散射/穿透数量，按行为使用。
- ProjectileSpeed、KnockbackForce、SlowFraction：弹速、击退力、减速比例。
- AbilityClass：可覆盖执行 GA。普通自定义 GA 不会自动获得内置模板的消耗、冷却、表现和结束流程；自行接线时需明确责任。

## Presentation 字段

| 字段 | 用途 |
|---|---|
| AttackPose | 从当前模型的 LocomotionLibrary 条目选择 Jab / CrossJab / Uppercut |
| AnimationPlayRate | 动态 montage 播放倍率 |
| Windup | 成功激活后到执行模板行为的前摇，0～3 秒；取消技能会取消等待 |
| CastParticles / CastNiagara | 施法时 Cascade / Niagara |
| ImpactParticles / ImpactNiagara | 命中时 Cascade / Niagara |
| ProjectileNiagara | 投射物附着 Niagara，投射物销毁时清理 |
| EffectScale | 特效缩放，至少 0.01 |
| EffectLifetime | 瞬时施法/命中特效保留时间，0.1～10 秒，避免循环资源残留 |

当前普攻前摇 0.12 秒，模板占位技能约 0.16～0.22 秒。AnimationPlayRate 不自动改 Windup；修改攻击速度后按画面命中位置重新调整。GroundStrike 仍会加上 Interval 的落地延迟，持续技能仍按自己的周期执行。

玩家与怪物复用同一套规则和表现配置。完整动画和现有占位特效位置见 [动画状态机与技能表现配置](../动画状态机与技能表现配置.md)。

## 最小验证与限制

放入一份四槽不重复 SkillId 的 Loadout，验证动作、命中、法力、冷却与中断。改显示名不应破坏技能升级引用。攻击动画片段不直接扣血，不要额外接旧伤害通知造成重复。

当前没有技能图标、音效、自定义投射物类字段。攻击只提供三种共享姿势，需特定技能专属动作可继续扩展数据。Dash 是带碰撞检查的瞬时位移，不是根运动冲刺。

交替拳击扩展：SkillDefinition.bAlternateAttackPose 启用服务器 Jab / CrossJab 轮换，Locomotion 将本次选定姿势复制给客户端。21 用此机制保留原 Primary 的左右拳；22 为原 Tertiary 的一秒疾跑，见[完整说明](../数值与技能面板配置.md)。

## 2026-10-24真实技能与铜币更新

新增RequiredFormId、DesignMechanic及各机制参数。新设计17个定义加共享原粒子形成18图标技能，目录共39项。详见真实技能文档参数表。

详见[真实技能与铜币配置](../真实技能与铜币配置.md)。
