# UCC_CharacterAnimInstance

父类：`UAnimInstance`。玩家和怪物共用的动画状态数据父类；每个 Skeleton 使用自己的派生 AnimBlueprint。

[头文件](../../Source/CrashCourse/Public/Characters/CC_CharacterAnimInstance.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_CharacterAnimInstance.cpp) · [返回索引](../CPP父类配置索引.md)

## 配置

新建 Animation Blueprint，Parent Class 选择 `CC_CharacterAnimInstance`，Target Skeleton 选择该模型的 Skeleton。不要直接把这个原生类作为最终 AnimClass：需要派生 ABP 内的 AnimGraph 才有实际姿势输出。当前生成资源为 `/Game/HPWH/Locomotion/ABP_<模型名>`，由 `DA_NPCLocomotion.Entries[].AnimClass` 对应到模型。

AnimGraph 为 `CharacterLocomotion → DefaultSlot → Output Pose`。状态机包含 Idle、Run、Jump、Fall、Land、Death；生成后可在编辑器展开查看、调整过渡和动画。六状态规则分别读取下面六个互斥 bool。

## 自动更新的值

| 字段 | 来源与意义 |
|---|---|
| Speed | Character 实际水平速度，cm/s |
| RunPlayRate | Speed / 对应条目的 ReferenceSpeed，限制 0.25～2，用于 Run Sequence Player |
| bIdle | 活着、在地面、未处于落地短窗口、速度不超过 5 |
| bRun | 活着、在地面、未处于落地短窗口、速度超过 5 |
| bJump | 活着、CharacterMovement 正在下落模式、垂直速度大于 0 |
| bFall | 活着、正在下落模式、垂直速度不大于 0 |
| bLand | 活着、从空中变为在地面后的约 0.15 秒 |
| bDead | BaseCharacter.IsAlive() 为 false，优先于移动状态，并停止攻击 montage |

角色跳跃仍由 CharacterMovement 驱动；这个类只读取运动和存活状态，不给角色添加额外位移。NPC 即使通常不跳，在被 LaunchCharacter 抛起时也能进入 Jump/Fall/Land。

## 验证

运行平地场后按空格观察 Jump → Fall → Land → Idle/Run，移动时进入 Run。普攻与技能通过 DefaultSlot 播放动态 montage，不能删除该 Slot。自动场景验证见 [动画配置指南](../动画状态机与技能表现配置.md)。

目前是全身 Slot 和六状态基础移动，尚无转向、步态分层、受击方向、连招窗口、脚部 IK 或根运动攻击系统。
