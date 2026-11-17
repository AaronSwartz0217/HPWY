# UCC_HitReact

架构审计：当前未检出外部C++或资源使用证据，列为清理候选。旧GA_HitReact不是本类子类；清理前仍需核对旧蓝图函数和动态加载。本轮未删除。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

父类：`UCC_GameplayAbility`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/Abilities/Enemy/CC_HitReact.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/Abilities/Enemy/CC_HitReact.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

旧受击能力，缓存朝向和攻击者方向，供蓝图选择前后左右动画。

## 配置方式

在派生 GA_HitReact 中监听相应受击 GameplayEvent，从 Payload 取得有效 Instigator 后调用 CacheHitDirectionVectors。使用工具库 GetHitDirection 决定动画。

## 参数与接口

- CacheHitDirectionVectors(Instigator)：缓存 AvatarForward 和 ToInstigator。
- AvatarForward、ToInstigator：只读蓝图向量。

## 最小验证

从不同方向攻击，观察受击动画是否按预期切换。

## 当前限制

调用前必须确认 Instigator 有效，原实现会直接访问它。新模板伤害不自动等价于旧蓝图全部受击表现，需明确接入事件。
