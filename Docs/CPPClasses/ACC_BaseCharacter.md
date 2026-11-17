# ACC_BaseCharacter

父类：`ACharacter`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Characters/CC_BaseCharacter.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_BaseCharacter.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

玩家和怪物的公共角色基类，提供存活状态、初始能力、属性初始化和死亡布娃娃，构造时已经创建 Skills 组件。自身的 ASC／AttributeSet 接口默认返回空，派生类负责提供。

另已创建 HealthDisplay（CC_WidgetComponent），默认在头顶显示纯文字生命值，不必在每个角色蓝图重新拼接同样的组件。

## 配置方式

不直接拖入场景，也不把它作为最终玩家。玩家继承 CC_PlayerCharacter，怪物继承 CC_EnemyCharacter；在派生蓝图 Class Defaults 中配置继承下来的字段。

## 参数与接口

- Initialize Attributes Effect：必须指定有效 GE；当前测试推荐 CC_InitialAttributesEffect。未设置会触发初始化断言。GE 应实际填好 MaxHealth、MaxMana、Health、Mana，而不只是拥有一个空资产引用。
- Reset Attributes Effect：ResetAttributes 使用的 GE，示例同样引用初始化 GE。
- Startup Abilities：公共受击、死亡、AI 等能力；新四技能组不在这里重复填。
- Search Range：寻敌半径，默认 1000。
- Damage Number Vertical Offset：供表现使用的垂直偏移。
- On ASC Initialized：ASC 和属性集可用时通知监听方。
- Rotate To Target：蓝图事件，自行实现转向。
- Enable Death Ragdoll：默认开；Death To Ragdoll Delay 默认 0.65 秒，先播放 Death 动画再开启 Mesh 物理。
- Corpse Lifetime：默认 10 秒，只对敌人从死亡开始计时清理；设 0 保留尸体。
- Is Ragdoll Active：蓝图只读查询实际布娃娃状态。Mesh 必须具备匹配当前骨架且有有效碰撞体的 Physics Asset。
- Reset Attributes、Handle Respawn：蓝图可调用；前者重置属性，后者清理死亡定时器、关闭物理并恢复 Mesh 挂接/碰撞/移动，再恢复存活状态。完整流程见[死亡与经验配置](../死亡布娃娃与经验粒子配置.md)。

## 最小验证

在两个示例角色蓝图中确认属性 GE 有效；运行时应存在一个 Skills 组件，角色初始生命为正数。

## 当前限制

已实现死亡动画转物理、敌人十秒清理与复活时还原组件。玩家 HandleRespawn 会重套形态，但不负责自动补满生命、选择复活位置或无敌期；先按游戏规则 ResetAttributes，再调用 HandleRespawn。缺少有效 Physics Asset 时保留 Death 动画并输出提示。
