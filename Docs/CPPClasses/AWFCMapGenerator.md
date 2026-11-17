# AWFCMapGenerator：独立 WFC 地图生成器

所属：`Plugins/SpiritWFC`，继承 `AActor`。与原有 `Source/CrashCourse` 的 69 类分开维护。

推荐继承方式：使用 `BP_WFC_Planar`、`BP_WFC_ThreeLayers`，或创建基于 **WFC Map Generator** 的蓝图。

主要输入：`TileSet`、`Settings`、碰撞/导航开关；主要操作：`GenerateMap`、`ClearGeneratedMap`；输出：`LastResult`、`DisplayedResult`、实例数和 `OnGenerationFinished`。

修改种子后在编辑器点击 Generate Map 即可。默认不在构造脚本或 BeginPlay 自动重建。失败保留原地图；读取实际画面用 DisplayedResult。相同模块分组为 HISM 组件，组件仅由所属生成器清理。

完整字段、边界、楼梯和蓝图接线见[插件配置与接入](../../Plugins/SpiritWFC/Docs/配置与接入.md)，流程见[贪心算法](../../Plugins/SpiritWFC/Docs/WFC算法与贪心策略.md)。
