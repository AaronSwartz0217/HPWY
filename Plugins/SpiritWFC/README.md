# Spirit WFC 地图生成插件

独立 UE 5.6 Runtime 插件，提供平面与最多三层立体地图的**模块式 WFC**。源码与资源均集中在本文件夹，项目只需启用 `SpiritWFC`；不依赖 CrashCourse、GAS、角色、UI、HTTP 或存档模块。

## 立即使用

**v0.3 网格编辑器：** 打开 `/SpiritWFC/Settlement/Maps/L_WFC_GridEditorExample`，选中生成器，在 **工具 → WFC 网格场景编辑器** 中填格、替换房屋和计算；[完整操作教程](Docs/网格场景编辑器完整教程.md)。支持按草图补齐、严格按图与完全自动生成，规划可另存为数据资产。

想看改进后的街区效果，优先打开 `/SpiritWFC/Settlement/Maps/L_WFC_Settlement_Planar` 或 `L_WFC_Settlement_ThreeLayers`。它们有连通街道、广场、完整房屋与变高建筑；[改进内容与参数](Docs/街区生成改进与配置.md)。下面 `/Samples` 保留为基础算法教学样例。

1. 编译并打开工程。内容浏览器勾选“显示插件内容”，进入插件资源路径 `/SpiritWFC/Samples`。
2. 打开 `Maps/L_WFC_Planar`（12×12×1）或 `Maps/L_WFC_ThreeLayers`（8×8×3）。`L_WFC_ModelGallery` 单独展示导入的 12 个建筑网格。
3. 选中地图中的 `WFC Generator`，在详情面板展开 `WFC`，修改 `Settings.Seed`，点击 **Generate Map**。无需开始游戏。
4. 新场景直接拖入 `Blueprints/BP_WFC_Planar` 或 `BP_WFC_ThreeLayers`，点击同一按钮。`Clear Generated Map` 只删除该生成器拥有的实例组件。
5. 换自己的模块：复制 `Data/DA_WFC_Planar` 或 `DA_WFC_Volume`，设置静态网格、六面接口和格子尺寸，再把它赋给生成器的 `Tile Set`。

默认不在构造脚本或开始游戏时重建；可显式开启 `Generate On Begin Play`。生成是同步操作，建议先使用示例尺寸，再逐步增加规模。

## 目录

| 目录 | 内容 |
|---|---|
| `Source/SpiritWFC/Public` | 蓝图/程序接口、数据结构与生成器父类 |
| `Source/SpiritWFC/Private` | 独立求解器、HISM 实例生成、算法检查 |
| `Content/Samples/Data` | 平面与立体模块规则数据资产 |
| `Content/Samples/Blueprints` | 可直接拖入场景的两个生成器蓝图 |
| `Content/Samples/Meshes` | 从已下载建筑资源转换的 12 个静态网格 |
| `Content/Samples/Materials` | 自建简洁占位材质，便于区分地面、道路、建筑、楼梯 |
| `Content/Samples/Maps` | 两个生成示例与模型展示图 |
| `Docs` | 接入配置、贪心算法、来源与许可、验证记录 |

原始 FBX 与可复现转换文件放在项目 `SourceArt/ProceduralGeneration/WFC`，工具脚本放在 `Scripts/WFC`。只携带插件即可使用已导入示例；重新制作 FBX 需要上述源素材目录。

## 阅读入口

- [从零到生成地图完整教程](Docs/从零到生成地图完整教程.md)：按编辑器操作顺序完成平面、三层、换模、蓝图和导航接入。
- [父类、参数与蓝图接线](Docs/配置与接入.md)
- [WFC、贪心策略与失败处理](Docs/WFC算法与贪心策略.md)
- [模型来源与许可证](Docs/开源模型来源与许可证.md)
- [本轮验证范围与结果](Docs/验证记录.md)

## 边界

这是生成框架与建筑样例，不是已接入主线的随机副本。局部接口匹配、楼层支撑和格子可通行图已经检查；网格实际碰撞、角色净高、楼梯入口以及 NavMesh 路径仍须做场景验收。示例的道路网格使用同一地板模型配色区分，尚未制作带道路线条的正式美术模块。

算法返回失败不会覆盖上一次成功显示的地图。`Last Result` 是最新尝试，`Displayed Result` 是当前可见地图；调用者应判断状态后接入出生点、传送、敌人和存档。
