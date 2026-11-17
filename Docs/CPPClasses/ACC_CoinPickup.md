# ACC_CoinPickup

可巡逻、碰触即领取的铜币Character父类。当前示例BP_CopperCoin已配置原五铢钱动画状态机、导航巡逻和拾取音频，可直接复制摆放。

- CapsuleComponent为根组件，半径24/半高42厘米，阻挡地形、Overlap角色；CharacterMovement使用Walking和重力处理落地与25厘米以内台阶。模型不承担地面碰撞。
- PickupSphere：默认90厘米，Pawn Overlap；CoinMesh显示五铢钱骨骼动画，不参与物理碰撞。它挂到胶囊体，当前相对Z=-42、Yaw=-90、缩放约0.30257，模型约80厘米高。
- CoinMesh.AnimationMode=AnimationBlueprint，AnimClass=ABP_WuzhuCoin_C。沿用原来的“New State Machine → 跑 → BS_WuzhuCoin”，speed读取Pawn实际速度，混合待机/移动。不要再改成SingleNode待机循环。
- bStartPatrolling默认true；PatrolRadius=600厘米、PatrolSpeed=100厘米/秒、MinPatrolWait=1秒、MaxPatrolWait=3秒。以出生位置为中心选择可达导航点，移动完成/失败后随机等待再尝试，不追击或攻击玩家。
- AIControllerClass=AIController，AutoPossessAI=PlacedInWorldOrSpawned；正式地图必须有覆盖摆放区域的NavMeshBoundsVolume，并构建导航。找不到导航时原地等待重试，不会穿墙直线漂移。
- 服务端StartPatrol/StopPatrol可给蓝图调用；停止会清理巡逻计时器及移动请求。销毁时清理计时器。
- CopperAmount=1、ExperienceAmount=5；PickupSound设置声音。
- 服务端TryCollect复核存活PlayerState、距离与bCollected，只结算一次；关闭碰撞、广播音频后销毁。
- 不通过旧积分蓝图，不要求E，不提供客户端增发RPC。CopperCoins由Progression复制给拥有者并保存至v3本地存档。

重建配置运行Scripts/configure_coin_patrol.py，仅更新铜钱资产和当前测试场铜钱；会恢复脚本中的默认参数。它同时处理已有蓝图从Actor父类迁入Character后的组件挂接。不要给此类配置敌人血量、战斗技能或击杀奖励。

配置与限制见[真实技能与铜币](../真实技能与铜币配置.md)。
