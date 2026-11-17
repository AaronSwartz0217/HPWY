# ACC_PlayerCharacter

父类：`ACC_BaseCharacter`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Characters/CC_PlayerCharacter.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_PlayerCharacter.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

玩家 Pawn。带第三人称弹簧臂和相机，从 PlayerState 获取 ASC、属性集，并初始化形态成长。

## 配置方式

优先复制 /Game/HPWH/Blueprints/BP_ProgressionCharacter，保留既有模型与动画。若新建蓝图，父类选择 CC_PlayerCharacter，配置 Mesh、动画类、初始化和重置 GE；再把蓝图填进 GameMode 的 Default Pawn Class。

## 参数与接口

- CameraBoom：默认臂长 600，可在继承组件详情里调整。
- FollowCamera：跟随相机。
- BeginPlay 会校正相机到 CameraBoom 的 Socket 挂接，启用 Camera 通道球形检测（ProbeSize=16），支点抬高65cm，肩部偏移(0,45,15)。当前关闭位置延迟，让碰撞缩回及时生效；继承蓝图保留600cm臂长，运行时臂长下限200cm。
- 贴墙时允许弹簧臂继续缩短，避免强行保持最短距离造成穿墙。实际镜头距模型中心不足100cm时，仅对拥有者隐藏人物网格，恢复距离后显示；场景墙体需要阻挡 Camera 通道。人物和尸体忽略 Camera 通道。
- GetCameraBoom / GetFollowCamera：蓝图或C++读取实际组件。
- Skills：已经存在，玩家技能来源优先为 Progression 的 NativeForm 或当前器物阶段 Loadout。
- 玩家 Actor Tag Player 由 C++ 构造时加入，用于当前阵营判定。
- PossessedBy 为服务器初始化路径，OnRep_PlayerState 为客户端初始化路径；重写时保留父类调用。

## 最小验证

运行示例地图，确认使用 BP_ProgressionCharacter；人形四个数字键可以释放技能，F1 能购买原生进化。

## 当前限制

必须配套 CC_PlayerState 派生类。替换 Mesh／AnimClass 必须保持骨架兼容；当前没有自动重定向、第一人称切换或水墨变身效果。

## 固有疾跑

所有形态默认拥有Shift疾跑，不占四槽。SprintSpeedMultiplier=1.5、SprintMaxDuration=5、SprintCooldown=5，可在蓝图Skills/Sprint调整。SetSprintHeld成对请求，服务器开始／松开或超时结束后进入冷却；IsSprinting、CanSprint、GetSprintTimeRemaining、GetSprintCooldownRemaining和OnSprintStateChanged供后续UI读取。ResetSprintState仅服务器，用于复活和开发预设。

角色通过独立UCC_SprintEffect临时乘算MoveSpeed；结束只移除自己的效果。眩晕／定身、菜单、死亡或失去控制结束疾跑；销毁清理旧ASC效果，临时状态不存档。现有跑步动画按实际速度播放。详见[疾跑与小镇返回](../固有疾跑与小镇返回配置.md)。

## 死亡返回休憩之地

新增RestRespawnDelay，默认5秒；保持原死亡布娃娃，然后通过AdventureGameInstance回休憩之地并恢复永久成长。HandleRespawn清理回城计时器，0/9恢复存活时不会被旧计时器再次送走。仅在注册的单机地区生效。
