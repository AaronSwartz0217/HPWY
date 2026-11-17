# UCC_SkillLoadout

父类：`UDataAsset`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_GameplayData.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_GameplayData.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

一组四技能，可用于玩家原生／器物的每个阶段，也可用于怪物。

## 配置方式

新建 CC_SkillLoadout Data Asset 或复制示例 Loadouts。Slots 固定四项，每项设置 Skill 与 Level。玩家把资产填到 Form.Stage.Loadout，怪物填到 Skills.DefaultLoadout。

## 参数与接口

- Slots[0..3] 对应界面技能 1～4。
- Skill 不可空、SkillId 不可为空且组内不能重复。
- Level 必须至少为 1；玩家还会叠加同 SkillId 已购买的等级。
- IsComplete 是 C++ 校验，不是供策划调用的蓝图按钮。

## 最小验证

把四个不同的 DA_Skill 填入槽位，验证四键可用；留空槽位时应拒绝应用该组。

## 当前限制

不同形态可共享同一技能组资产，编辑共享资产会影响所有引用它的形态。希望各阶段独立调整时复制资产。
