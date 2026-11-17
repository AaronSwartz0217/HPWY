# ACC_EnemyCharacter

父类：`ACC_BaseCharacter`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Characters/CC_EnemyCharacter.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_EnemyCharacter.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

怪物角色，自身持有 ASC 和属性集，负责健康监听、停止移动与击飞落地状态。

## 配置方式

复制 BP_ProgressionEnemy；在 Class Defaults 设置初始化／重置 GE、AI Controller Class=AIController、Auto Possess AI=Placed in World or Spawned。Skills → Default Loadout 指定四技能组，Startup Abilities 放 CC_SearchForTarget。

## 参数与接口

- Experience Reward：服务器判定首次有效击杀后产生的经验总值，默认 10；以十颗每颗 1 经验的粒子掉落，接触领取玩家后才入账。
- Experience Particle System：Niagara 外观引用，示例 NS_ExperienceOrb；留空使用默认路径。
- 继承的 Death To Ragdoll Delay 默认 0.65 秒，Corpse Lifetime 默认 10 秒；尸体清理不影响已经生成的经验粒子。
- Acceptance Radius：移动接近范围，默认 500，近战怪应按技能距离缩小。
- Min／Max Attack Delay：每轮寻找的随机延时，默认 0.1～0.5 秒，最小值不应大于最大值。
- Search Range：继承寻敌范围。
- Rotate To Target：蓝图中实现转向目标，可用时间轴插值。
- Get Timeline Length：返回上面转向所需秒数，攻击会等待此时长。
- Is Being Launched：只读同步状态。

## 最小验证

给怪物四个不同技能，放入有地面和 NavMeshBoundsVolume 的关卡；确认寻敌、接近、转向、攻击、布娃娃和一次性经验掉落；全部吸附后钱包才增加 10。

## 当前限制

四槽 AI 当前是随机起点遍历与距离筛选，不是完整行为树；不自动重生。单次启动检查没有覆盖长时间 AI、动画和导航联动。

## 2026-10-24 数值与技能配置更新

新增 CombatBalance 与 StatProfileId。留空 ID 按当前骨骼网格体匹配；显式 ID 允许同模型普通怪与精英共存。BeginPlay 在初始化属性后、Skills.Initialize 前调用 ApplyCombatProfile。精英 ProfileMeshScale 由原始网格体缩放 × 1.2 算得并复制到客户端，不重复叠加。模型本体和动画资源不修改。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。
