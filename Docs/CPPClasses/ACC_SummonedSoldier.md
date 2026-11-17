# ACC_SummonedSoldier

青铜召唤兵人的运行时Character，继承CC_EnemyCharacter，使用独立ASC/生命，并随召唤者采用友军/敌军阵营。由Skills.SummonSoldiers创建，通常不手动摆场景。2026-10-25修复追击停在攻击距离外、模型悬空及动作库缺失。

- InitializeSummon指定召唤者、来自技能Duration的生命周期（当前60秒）、青铜初级模型/ABP和50%召唤者最大生命。
- AI接管后重新保留召唤者Owner，召唤击杀的经验仍归玩家；自身ExperienceReward=0。
- 每0.2秒寻找1400厘米内可见敌人/跟随召唤者，每2秒近战一次，0.15秒前摇后重新检查距离、视线、存活与控制状态，造成玩家实时攻击25%的间接伤害，防御随玩家同步。墙后、隐身、已死及同阵营对象不作为攻击目标。
- 同阵营不互相攻击；最多3个，重新召唤替换旧兵人；玩家死亡或清理状态销毁。
- 寻路到达判定不再叠加双方胶囊半径；目标停止距离140厘米，小于190厘米攻击距离，避免停在攻击范围之外。使用导航追击；缺少导航/寻路失败时，仅在视线通畅处用CharacterMovement直接跟随，仍受胶囊碰撞与重力约束，绕障依赖导航。
- 胶囊默认与主角相同，半径42/半高96厘米；模型Z=-96、Yaw=-90。接入DA_NPCLocomotion的青铜模型动作条目，跑步、攻击复用现有状态机与拳击动作。落地由CharacterMovement负责，不使用模拟物理移动活体。

## 蓝图配置

可建立本类的派生蓝图，在主角Skills组件的 `SummonedSoldierClass` 指定；留空继续使用原生类。无需改输入映射、兵人不需要玩家控制器/PlayerState，也不会拥有玩家背包或存档。

| 字段 | 默认值与用途 |
|---|---|
| SoldierForm | DA_Form_Artifact_01，采用第0档的Mesh/Scale/AnimClass |
| Locomotion.Library | DA_NPCLocomotion，须有该Mesh的Idle/Run/Jab/AnimClass |
| MeleePresentation | DA_BasicAttack，只负责近战动作/特效；实际伤害由兵人参数计算 |
| DetectionRadius | 1400厘米，自动索敌半径 |
| AttackRange | 190厘米，中心距离；追击停止距离自动减50厘米 |
| AttackInterval / AttackWindup | 2秒 / 0.15秒 |
| AttackFraction / HealthFraction | 0.25实时攻击比例 / 0.5出生生命比例 |
| FollowDistance | 200厘米，无敌人时跟随召唤者 |
| MeshGroundOffset | 0厘米，模型特殊脚底原点的额外微调 |
| CurrentTarget / GetSummoner() | 供后续战友UI读取的目标与召唤者 |

出生点先向导航投影，再检测可行走地面、胶囊净空及与召唤者间视线，尝试近处替代位置。位置无效时放弃该名额，最多3名，避免强制生成进墙或悬崖。自定义派生类的胶囊尺寸同样参与出生净空检查。建议AttackRange大于双方碰撞半径之和。

验证边界：编译与只读默认资源检查；实际召唤、追击、贴地、击杀经验、复杂障碍和联机表现交由游戏内联调。资源检查脚本为 `Scripts/verify_summoned_soldier_assets.py`，不启动游戏、不创建游戏世界、不改存档。

本次UE5.6 Editor/Win64/Development编译、DLL链接及10项只读资源检查通过。日志 `Saved/Logs/SummonedSoldierBuild.log`，检查报告 `Saved/UI/SummonedSoldierAssetVerification.json`。手测重点：开阔地召出3名后，靠近敌人观察追击/拳击/扣血；敌人倒下后是否回跟；墙后敌人不被隔墙击中；斜坡和墙边不悬空/卡进模型；兵人击杀奖励是否吸附到玩家，重新召唤是否替换旧兵人。

配置见[真实技能与铜币](../真实技能与铜币配置.md)。
