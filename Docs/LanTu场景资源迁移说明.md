# LanTu Rebuild 场景资源迁移

源目录：`C:/Users/29717/Documents/Unreal Projects/LanTu/Content/Rebuild`，源版本 UE 5.5。只从该目录的资源出发递归收集硬引用、软引用及地图外部资源，未复制整个 LanTu 工程，也未修改源项目资源。

迁入目录：`/Game/LanTuRebuild`（磁盘 `Content/LanTuRebuild`）。先在 Saved/LanTuMigrationStage 隔离副本中用 UE 5.6 的资源重命名接口分类并保存引用，再校验后复制到本工程，避免覆盖本工程 Characters、StarterContent 等同名资源。

## 分类与数量

| 分类目录 | 内容 | 资源包数 |
|---|---|---:|
| Models/StaticMeshes | 场景静态模型 | 2137 |
| Models/SkeletalMeshes | 依赖的骨骼模型 | 10 |
| Models/Rigs | 骨架、物理资产和 Control Rig | 11 |
| Effects | Niagara 与粒子特效 | 5 |
| Materials | 材质、材质实例、材质函数 | 314 |
| Textures | 贴图及渲染目标 | 576 |
| Animations | 动画、动画蓝图、混合空间、姿态 | 84 |
| Maps | 3 张地图、HLOD 配置和关卡序列 | 5 |
| Blueprints | 场景蓝图及其依赖 | 15 |
| UI | 场景依赖的控件 | 3 |
| Data | 枚举、结构和曲线等 | 13 |
| Audio | 声音 | 1 |
| 合计 | 转换后约 2.89 GB | 3174 |

各类内部保留原始子目录，避免不同资源包的同名模型／材质冲突。[迁移清单](LanTu资源迁移清单.json)记录每个资源的旧路径、新路径和类型。

## 地图入口

| 资源路径 | 编辑器加载 Actor 数 |
|---|---:|
| /Game/LanTuRebuild/Maps/cppTest | 10 |
| /Game/LanTuRebuild/Maps/初始房间/restartlife | 2128 |
| /Game/LanTuRebuild/Maps/古风小镇/古风小镇 | 150 |

源 Rebuild/restartlife 是重定向，实际地图位于初始房间目录，已随引用解析迁移。restart.ini 是源工程的世界分区转换配置，不是场景资源；原文保存在 Docs/LanTuMigration/SourceRebuild/restart.ini，未应用到当前关卡。

## 依赖与修复

- 启用资源依赖的 DatasmithContent、Niagara、ControlRig、ACLPlugin、Fab、InterchangeAssets。引擎／插件资源使用当前 UE 5.6 自带资源，不复制旧引擎文件。
- 源工程已有 3 个缺失模板模型路径：Heroes/Mannequin 下的 Manny、Quinn，以及 FirstPersonArms 下的 SKM_Mannequin。隔离副本使用源项目现有的 Manny、Quinn 和 SK_Mannequin_Arms 分别补齐，随后一起分类；这是现存对应模型的补位，不是恢复已丢失文件。具体旧路径及替代路径记录在清单 recoveredMissingReferences。
- 新目录不依赖旧的 /Game/Rebuild、/Game/WPQT 等项目资源路径；分类后的资源之间使用新的引用。

## 验证

隔离副本及本工程各启动全新 UE 5.6 进程检查：3174 个资源包存在，无缺失包，无分类目录外的项目资源引用；3 张地图均成功加载。复制文件逐包进行 SHA-256 对比；启用插件后 Editor Development 编译通过。

报告：Saved/LanTuMigrationVerify.json、Saved/LanTuMigrationCopy.json。地图检查为编辑器加载和依赖检查，没有替代每张地图的实际游玩、光照和特效表现验收。

GameDefaultMap现为初始房间restartlife（休憩之地），EditorStartupMap保留L_FlatTestArena供开发。三图运行时统一框架、门户和地图UI见[地图传送与小地图配置](地图传送与小地图配置.md)。

审计、分类及安装脚本位于 Scripts/audit_lantu_rebuild.py、classify_lantu_stage.py、install_lantu_classified.py。不要直接重复运行安装脚本覆盖现有目录；未来增量迁移应重新审计并处理变更。Saved/LanTuMigrationStage 是本次生成的隔离转换副本，和当前运行资源目录分开。
