# UWFCTileSet：WFC 模块规则数据资产

所属：`Plugins/SpiritWFC`，继承 `UDataAsset`。内容浏览器新建数据资产时选择 **WFC Tile Set**，无需为每块建筑创建 Actor 蓝图。

主要配置：`CellSize`、`Tiles`、`TraversableSockets`。每条模块定义包含唯一 ID、正权重、允许旋转、六面接口、楼层范围、可通行标志、静态网格、局部变换和材质覆盖。

边相同的接口才兼容；可匹配并不代表可通过，是否可通过由 TraversableSockets 和 Walkable 共同决定。上下支撑 Floor 与通行 Stair 要分开。最多 256 个旋转变体；空名字、重复 ID 和无效权重会被拒绝。

平面/三层资产例子和各字段完整含义见[插件配置与接入](../../Plugins/SpiritWFC/Docs/配置与接入.md)。
