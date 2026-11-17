# UCC_Primary

架构审计：当前未检出外部C++或资源使用证据，列为清理候选。名称相近的/Game/THAT/Player/GA_Primary实际继承CC_GameplayAbility，不能按名字判断继承关系；本类尚未删除。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

父类：`UCC_GameplayAbility`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/Abilities/Player/CC_Primary.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/Abilities/Player/CC_Primary.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

旧玩家主攻击基类，提供命中检测配置和批量发送敌人受击事件的方法。

## 配置方式

维护旧 GA_Primary 时保留此父类；命中检测、伤害 GE 与动画流程仍由蓝图调用。旧控制器回退路径依赖 Primary Asset Tag。

## 参数与接口

- HitBoxRadius：100；HitBoxForwardOffset：200；HitBoxElevationOffset：20，供蓝图逻辑读取。
- SendHitReactEventToActors：向命中 Actor 发送 Enemy.HitReact 事件。

## 最小验证

确认旧技能的动画与命中调用链仍有效；迁移到新技能组时避免同时授予两份主攻击。

## 当前限制

该函数只发送受击事件，不直接扣血。不要把它当作新的通用近战伤害类。
