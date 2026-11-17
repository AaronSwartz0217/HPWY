# UCC_LocomotionLibrary

父类：`UDataAsset`。示例 `/Game/HPWH/Locomotion/DA_NPCLocomotion`。它是模型与对应动画的表，不拖入场景。

[头文件](../../Source/CrashCourse/Public/Characters/CC_LocomotionComponent.h) · [返回索引](../CPP父类配置索引.md)

## Entries 配置

| 字段 | 配置 |
|---|---|
| Mesh | 精确对应当前 SkeletalMesh，每个 Mesh 一条 |
| AnimClass | 该 Skeleton 专属、继承 CC_CharacterAnimInstance 的 ABP |
| Idle / Run | 待机、跑步循环 |
| Jump / Fall / Land | 起跳、空中下落循环、落地 |
| Death | 死亡，播放一次并保留末帧 |
| Jab / CrossJab / Uppercut | 三种攻击姿势，由 SkillDefinition.AttackPose 选择 |
| ReferenceSpeed | Run 动画的参考速度，默认 400 cm/s |

所有动画必须属于 Mesh 自己的 Skeleton。AnimClass 也必须使用同一 Skeleton。不能把 Quinn 的 ABP 直接套在 SOURCE 角色上；配套脚本已经分别烘焙重定向动画并生成 ABP。

## 当前模型

- 青铜：铜机械小兵、铜械战士、铜械将军、铜械降神。
- 草竹：竹路人、竹节人、竹拳击手、陶罐头；陶罐头按用户指定归草竹系列。
- 陶塑：陶童子、陶土行者、陶土僧人。

共 11 条模型记录。玩家自然/青铜/陶塑各三档共九形态使用其中九条，另外两个模型供 NPC 使用。路线见 [三条力量进化路线](../三条力量进化路线.md)。

## 配套工具

`retarget_npc_locomotion.py` 导出 Idle/Run，`retarget_that_actions.py` 参考 THAT 主角使用的动作导出其余片段，`fix_retarget_import_scale.py` 修正 SOURCE 导入骨架的根比例，`configure_character_animation.py` 填表、生成 ABP 并接入形态和测试场。

批量 IK 重定向必须使用完整编辑器 Python 模式 `-ExecutePythonScript=...`，不要用 `-run=pythonscript`。这些配置脚本会保存项目资产与测试场；编辑器有未保存修改时先保存并关闭。`configure_npc_art.py` 是旧待机/跑步设置脚本，会重建较少字段；不要在当前完整动画配置后单独重跑它。

手工配置新模型和技能表现见 [动画配置指南](../动画状态机与技能表现配置.md)。
