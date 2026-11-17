# ACC_ExperiencePack

父类：`AActor`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ExperiencePack.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ExperiencePack.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途与配置

可用 E 领取的场景经验包父类，含静态 Mesh 和 `50 XP [E]` 文字 Label。当前平地场用纯黑方块表示；复制已有经验包蓝图再放置即可。自建时给 Mesh 方块和黑色材质，碰撞响应 Visibility，保留 Actor 的 Replicates。

## 字段与接口

- Experience Amount：默认 50，一次发出 50 个每颗 1 经验的 Niagara 经验粒子。
- Experience Particle System：默认示例使用 NS_ExperienceOrb，留空由 SpawnBurst 使用默认路径。
- Interaction Distance：250 cm，按玩家到 Actor 原点距离校验，另检查视线。
- Repeatable：默认关闭；成功后 bConsumed=true，隐藏并关闭方块碰撞。
- Reuse Delay：默认 1 秒；启用 Repeatable 后控制两次成功领取间隔，防止同帧重复请求。
- Can Interact(Character)：查询存活、归属、距离、视线、消费与冷却状态。
- Interact(Character)：只给权威逻辑调用；成功产生经验粒子，不直接把 50 加入钱包。

E 键沿用 CC_PlayerController 的射线与 RequestInteraction 路径，不必在包蓝图单独 Enable Input 或绑定 E。不要同时调用 AddExperience，避免一份奖励记两次。

## 验证与限制

近距离 E 一次应有 50 颗飞向角色，全部接触后总计 +50；快速重复按键不额外发奖。默认每个实例整局只能消费一次；消费状态没有存档，重新进图会重置。详细配置和测试见[经验粒子指南](../死亡布娃娃与经验粒子配置.md)。

## 交互范围更新（2026-10-24）

黑色经验包与元素方块共用 E 键附近候选选择。`InteractionDistance` 默认 250 厘米，按可碰撞组件包围盒表面计算；遮挡、已消耗、重用冷却和存活状态继续校验。经验粒子仍在实际吸附接触后入账。
