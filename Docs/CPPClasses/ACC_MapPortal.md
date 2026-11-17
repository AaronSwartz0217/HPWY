# ACC_MapPortal

父类AActor。碰触式地图传送门，单机角色进入Trigger后请求GameInstance切图。

- DestinationRegion：Rest/Town/Arena。
- Trigger：只Overlap Pawn的Box，不阻挡行走或导航。
- bShowMarker：是否显示占位底座；复用旧装置美术时为false。
- Marker / Label：占位底座与目标中文名称。
- ArmDelay：默认1秒，只用于出生时的防循环保护，不是进入传送门后的等待时长。
- TravelDelay：进入触发区后先播放特效，等待该秒数后才请求切图；通用传送门默认0，休憩→小镇原装置适配为8秒，与原蓝图Delay一致。
- DepartureEffect / DepartureEffectTransform：离场Niagara系统及相对传送门的变换。原装置优先复用原蓝图的Niagara组件模板与变换。
- IsDeparturePending：当前是否处于离场特效等待阶段。等待期间不会提前暂停世界或展示加载界面，重复触碰不叠加或重置计时。
- 到期后重新校验角色存活、是否仍在触发区、技能状态、目的地与保存结果；失效则不切图。销毁传送门或退出世界时清理定时器和离场特效。

原装置在运行时自动套接；新关卡可建派生蓝图直接摆放、调整Trigger并添加美术。门户不直接改VisitedRegions或打开任意字符串地图，目标由GameInstance目录校验。

休憩→小镇门户允许首次发现，其余目的地遵循解锁规则。入口有重复触碰节流与GameInstance单次传送锁。[完整配置](../地图传送与小地图配置.md)。
