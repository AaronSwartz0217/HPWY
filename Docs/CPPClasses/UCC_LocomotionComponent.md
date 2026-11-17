# UCC_LocomotionComponent

父类：`UActorComponent`。BaseCharacter 构造时已创建 Locomotion，玩家、巡逻怪物和靶子不需要再加第二个。

[头文件](../../Source/CrashCourse/Public/Characters/CC_LocomotionComponent.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_LocomotionComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 配置

Library 指定 `/Game/HPWH/Locomotion/DA_NPCLocomotion`。组件按当前 SkeletalMesh 精确查找 Entries 条目，每 0.05 秒确认角色正在使用条目的 AnimClass。三条力量路线的九档模型与场景 NPC 均使用模型专属 ABP；更换 Mesh 后自动选择对应动画。

填写 AnimClass 时使用 Animation Blueprint 模式，由 `CC_CharacterAnimInstance` 和 ABP 状态机处理待机、跑步、起跳、下落、落地和死亡。组件不会每帧重播动画，也不会在技能 montage 播放期间强制切成待机。ReferenceSpeed 用于跑步倍率。

AnimClass 为空但 Idle/Run 有效时仍保留旧 SingleNode 待机/跑步兼容路径。该路径没有状态机与 DefaultSlot，不能当作完整攻击、跳跃配置。Mesh 未登记时保留原来的动画系统。

## 技能表现接口

以下为内置执行器调用的 C++ 接口，不需要蓝图重复调用：

- `PresentSkill(Skill, TargetPoint)`：服务器发起 MulticastSkill，玩家和怪物按 AttackPose 从模型条目选 Jab/CrossJab/Uppercut，通过 DefaultSlot 播放动态 montage；同时在角色前方生成施法特效，GroundStrike/DamageZone 则在选定落点生成。
- `PresentImpact(Skill, Location)`：服务器广播命中位置并生成技能配置的 Niagara/Cascade 命中特效。
- `CancelPresentation()`：服务器可靠广播停止 montage；用于技能取消、技能组替换等流程。
- `GetCurrentEntry()`：按当前 Mesh 查对应条目。

技能和命中特效事件使用不可靠 Multicast，视觉效果不参与伤害判定。专用服务器跳过视觉生成。瞬时特效按 EffectLifetime 限时销毁；投射物拖尾由 SkillProjectile 自身持有与销毁。请不要再在 OnSkillExecuted 蓝图中重复播放同一套效果。

完整配置及回归方式见 [动画状态机与技能表现配置](../动画状态机与技能表现配置.md)。联网视觉链路已接入，但完整多人效果仍需实际双客户端验收。

交替拳击扩展：SkillDefinition.bAlternateAttackPose 启用服务器 Jab / CrossJab 轮换，Locomotion 将本次选定姿势复制给客户端。21 用此机制保留原 Primary 的左右拳；22 为原 Tertiary 的一秒疾跑，见[完整说明](../数值与技能面板配置.md)。
