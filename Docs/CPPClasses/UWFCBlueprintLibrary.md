# UWFCBlueprintLibrary：WFC 数据求解接口

所属：`Plugins/SpiritWFC`，继承 `UBlueprintFunctionLibrary`。不放入场景，不需要蓝图子类。

- `SolveWFC(TileSet, Settings)`：纯布局生成，返回状态、完整格子、种子、次数、传播步数、连通数和耗时；不加载网格，不创建场景。
- `ValidateWFC(TileSet, Settings, Cells, Error, WalkableComponents)`：独立复核存储或编辑过的布局，蓝图返回 bool 及两个输出参数。

UE Python 名称为 `WFCBlueprintLibrary.solve_wfc/validate_wfc`。Python 会把带输出参数的 bool 成功结果映射为 `(Error, WalkableComponents)`，失败为 `None`；蓝图仍使用显式 bool 返回值。

底层是无 World 的 `FWFCSolver`，可用于种子筛选、设计预览及后续服务器逻辑。当前同步执行，尚未接入 UI 加载进度、现有地图传送或角色存档。

详见[完整接入文档](../../Plugins/SpiritWFC/Docs/配置与接入.md)。
