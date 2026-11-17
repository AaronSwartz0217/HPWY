# UCC_ArtifactComponent

父类：`UActorComponent`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ArtifactComponent.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ArtifactComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

让已有 Actor 蓝图成为特殊器物。形态由 Form 数据资产引用决定，不按字符串标签猜测。

## 配置方式

打开现有物体 Actor 蓝图 → Add Component → CC Artifact Component。设置 Form、Interaction Distance、Enabled；模型碰撞响应 Visibility；联网 Actor 开启 Replicates。Form 也要列入玩家 Progression.ArtifactForms。

## 参数与接口

- Form：一份完整的器物形态配置。
- Interaction Distance：从角色位置到物体包围盒最近表面的最大距离，默认 250。
- Enabled：是否可交互。
- Can Interact(Character)：检查存活、形态完整性、距离及视线。
- On Interacted(Character)：服务器成功释放能力球后触发，此时尚未入包或增加经验。

## 最小验证

同一种Form可挂多个物体；E后物体隐藏并释放能力球，接触后力量入包并增加50经验；背包使用才变身。隔墙、超距或重复E拒绝。

## 当前限制

Actor需配置碰撞并开启Replicates。当前会话每个物体只释放一次，球属于发起交互的玩家，未领取超时则恢复物体。仅加SpecialArtifact标签无效；世界消耗状态暂不持久化。

## 交互范围更新（2026-10-24）

`InteractionDistance` 按玩家位置到物体可碰撞组件包围盒最近表面的距离计算，不再按 Actor 原点距离计算；默认 250 厘米。遮挡射线朝向可碰撞物体中心，仍不允许穿墙交互。E 键支持控制器的附近候选搜索，不必精确瞄准小方块。接触能力球后存入背包并增加50经验。

新增ExperienceReward、bConsumed、bUseFormPalette、PowerColor/AccentColor、GetPowerColors及可选PowerOrbSystem。完整说明见[元素信物与能力球](../元素信物与能力球配置.md)。
