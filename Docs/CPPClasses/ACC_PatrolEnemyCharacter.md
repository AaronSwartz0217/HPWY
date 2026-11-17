# ACC_PatrolEnemyCharacter

父类：`ACC_EnemyCharacter`。

[头文件](../../Source/CrashCourse/Public/Characters/CC_PatrolEnemyCharacter.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_PatrolEnemyCharacter.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

可继承的常见范围巡逻敌人父类。服务器在出生点附近随机巡逻，到点停留；发现存活玩家后追击并尝试现有四技能；玩家离开警戒区域、死亡、消失或持续丢失视线后返回巡逻中心，再继续巡逻。死亡时停止移动、攻击和定时更新。

## 配置方式

1. 创建以 `CC_PatrolEnemyCharacter` 为父类的 Actor Blueprint，或复制 `/Game/HPWH/Blueprints/BP_PatrolEnemy`。
2. 设置 Mesh、动画类及继承组件的位置。示例已复用初始玩家模型。
3. Skills → Default Loadout 指定完整四技能组；示例引用 DA_EnemyLoadout。
4. 不要把旧 CC_SearchForTarget 再放入 StartupAbilities。本父类拥有自己的巡逻／追击调度，并会拒绝启动旧搜索逻辑，避免抢占移动。
5. Actor 放在地面上的有效 NavMesh 内；关卡放 NavMeshBoundsVolume 覆盖巡逻与返回区域，按 P 检查可行走区域。
6. 默认 AIController 与 Auto Possess AI（Placed in World or Spawned）已经配置；默认初始属性 GE 也已配置为 CC_InitialAttributesEffect。

本父类没有硬编码模型或技能组，因此直接新建蓝图时仍需填 Mesh／AnimClass／DefaultLoadout。

## 参数与接口

### 巡逻参数

| 字段 | 默认值 | 意义 |
|---|---|---|
| Start Patrolling | true | BeginPlay 后自动启动；复活时按此选项恢复 |
| Patrol Radius | 600 | 以出生中心为圆心的巡逻半径，单位厘米 |
| Patrol Acceptance Radius | 60 | 到点接受半径，与寻路停止使用同一距离标准 |
| Min／Max Patrol Wait | 1／3 | 每个巡逻点随机停留秒数 |
| Patrol Speed | 180 | 巡逻与返回的基础移速 |
| Brain Interval | 0.2 | 服务器判断间隔，不开启逐帧 Tick |
| Move Retry Delay | 1 | 路径失败或导航未就绪时的重试间隔 |

### 战斗参数

| 字段 | 默认值 | 意义 |
|---|---|---|
| Detect Players | true | 是否主动发现带 Player Actor Tag 的存活角色 |
| Require Line Of Sight | true | 发起警戒及攻击要求可视射线；不是视锥角度感知 |
| Detection Radius | 900 | 从敌人当前位置发现玩家的距离 |
| Max Chase Distance | 1800 | 从出生中心计算的追击边界，实际不会小于 PatrolRadius |
| Lose Sight Delay | 3 | 丢失视线后继续警戒多久再返回 |
| Chase Speed | 400 | 追击基础移速 |
| Combat Distance | 180 | 追击接近目标时的停止距离；实际攻击还检查各技能射程 |
| Min／Max Attack Delay | 继承值 | 技能之间的尝试间隔 |
| Experience Reward | 10 | 继承的击杀奖励 |

### 蓝图操作

- `StartPatrol(ResetOrigin=false)`：启动或重启；ResetOrigin=true 时把当前位置设为新巡逻中心。
- `StopPatrol()`：停止 AI 更新与移动，并取消当前由技能组管理的能力。
- `ReturnToPatrolOrigin()`：脱战返回中心；到达后恢复巡逻等待。
- `IsInsidePatrolArea(Location)`：查询 XY 平面位置是否处于巡逻半径内。
- `OnPatrolStateChanged(PreviousState,NewState)`：服务器与复制到的客户端可用于纯文字状态、动画或调试显示。
- 复活前先恢复属性／位置，再调用继承的 `HandleRespawn()`；单独调用它不会替你补满生命或重置场景位置。

### 只读状态

PatrolOrigin、PatrolDestination、CurrentTarget、PatrolState。

状态为 Disabled、Patrolling、Waiting、Chasing、Attacking、Returning、Dead。返回阶段不会立即重新锁定玩家，避免在警戒边缘反复切换。

## 最小验证

1. 远离玩家时，在出生点 600 厘米范围内换点行走，到点后等待。
2. 玩家接近 900 厘米内且可见时，进入追击／攻击。
3. 玩家离开出生中心 1800 厘米之外，或持续失去视线，清除目标并返回。
4. 回到中心后继续巡逻；死亡后不再走动或施法。
5. 没有 NavMesh 时安全等待重试，不穿墙直走或不断提交新路径。

测试场中的 Patrol_Enemy_01～04 使用此父类。单位换算：100 厘米=1 米。

## 已完成验证

- UE 5.6 Development Editor 编译通过。
- `CrashCourse.AI.PatrolTransitions` 自动化通过：无导航安全等待、范围判断、发现目标、追击边界、到点等待、停止定时器、旧搜索隔离、死亡和复活。
- 实际 L_FlatTestArena 关卡自动验证通过：四个敌人均发生导航移动，接近后发现玩家，远离后清除目标，返回后恢复巡逻，生命归零后停止。
- 回归的经验／战斗和属性 UI 测试也通过，总计 3 项自动化测试。
- 结果文件：`Saved/PatrolSmoke.json`、`Saved/Automation/Patrol/index.json`。实际关卡测试命令为 `CC.PatrolSmokeTest`，仅供开发验证；它会移动玩家、令一个敌人死亡、写报告并退出进程，请在单独测试进程使用。
- 已统一 MoveTo 与状态转换的到达半径，避免引擎已停止移动但逻辑一直停在 Returning 的边界问题。复杂障碍、多敌人拥堵和双客户端仍需另行验收。

## 当前限制

- 随机巡逻，不是固定路线、路点顺序或行为树系统。
- 使用范围扫描与射线，不含视锥角度、声音感知、仇恨表或组队警戒。
- 技能采用随机起点检查四槽，没有复杂职业决策。
- 巡逻目标点限制在圆内；若导航路径绕障碍导致角色离开圆，更新逻辑会要求返回。不是 NavMesh 的硬裁剪边界。
- 技能组取消会结束尚在执行的能力；已经生成的独立投射物按自身寿命处理，不因脱战自动消失。
- 不自动治疗、重生、复位尸体或持久保存出生中心。

## 2026-10-24 数值与技能配置更新

继承 EnemyCharacter 的数值表配置。常规四槽仍近战；精英 DA_EliteLoadout 为三近战加粒子远程，TryCombatSkill 根据各自 Range 与冷却尝试施放，追击逻辑保持。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。
