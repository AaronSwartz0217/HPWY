# UCC_SearchForTarget

父类：`UCC_GameplayAbility`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/Abilities/Enemy/CC_SearchForTarget.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/Abilities/Enemy/CC_SearchForTarget.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

服务器自动运行的敌人搜索／接近／攻击能力，支持新四槽和旧 Enemy.Attack 路径。

## 配置方式

放入敌人的 StartupAbilities，确保 AIController 在激活前存在。已内置 ActivateOnGiven Asset Tag；搜索半径与延迟在 EnemyCharacter 配，不在技能资产中重复配置。

## 参数与接口

- 通过 Actor Tag Player 搜索存活玩家。
- 寻路后调用 RotateToTarget 并等待 GetTimelineLength。
- 新技能组按随机起点遍历四槽，检查目标距离并尝试激活。
- 找不到路径时，若至少一个技能在射程内仍允许尝试攻击。
- Enemy.EndAttack 事件触发下一轮搜索。

## 最小验证

部署有导航的关卡并配置敌人控制器，观察搜索、移动、攻击循环；替换技能组后应使用四槽。

## 当前限制

自定义怪物技能必须正确发送 EndAttack，否则循环可能停住；技能取消、死亡与任务清理的完整压力测试仍需补充。
