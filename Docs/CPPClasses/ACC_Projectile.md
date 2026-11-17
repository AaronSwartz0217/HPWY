# ACC_Projectile

架构审计：当前未检出外部C++、蓝图父类或检查范围内的序列化类名引用，列为清理候选，尚未删除。正式远程使用ACC_SkillProjectile；删除旧类前仍需核对动态加载与旧投射物资产。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

父类：`AActor`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/GameObjects/CC_Projectile.h) · [实现](../../Source/CrashCourse/Private/GameObjects/CC_Projectile.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

旧教程敌人投射物，命中玩家时调用旧伤害工具并触发 Impact 表现。

## 配置方式

仅维护旧蓝图时使用；新 20 技能用 CC_SkillProjectile。派生蓝图需提供根／碰撞、模型和 DamageEffect；Spawn 时正确指定 Owner 为攻击者。

## 参数与接口

- Damage：ExposeOnSpawn；应传正数伤害。
- DamageEffect：使用 SetByCaller Projectile 的伤害 GE。
- ProjectileMovement：运动组件。
- SpawnImpactEffects：蓝图命中特效事件。

## 最小验证

确认服务器碰撞扣血、攻击来源正确、投射物被销毁。

## 当前限制

旧 C++ 默认 Damage 为 -25，而旧工具会再次取负；不要使用这个默认值，需明确改为正数。未解决所有历史默认配置问题，避免将旧投射物当作新模板直接使用。
