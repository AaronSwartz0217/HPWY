# Unity 特效迁移试验

源项目：桌面 `SHADER (2)`，`Assets/特效`。
目标项目：桌面 `GASCrashCourse-main/GASCrashCourse-main`，UE 5.6。

## 已完成

- 扫描特效目录：628 个 prefab、415 个材质、44 个 shader、197 张 PNG、284 张 TGA、90 个 FBX、87 个 WAV。此为文件数量，含变体和副本，不等于独立特效数。
- 选择 `VFX_Shockwave_01_White_1s.prefab`，按 GUID 提取依赖并记录 SHA-256。
- 导入 5 张贴图和 1 个原始 FBX。
- 在 UE 重建近似动态材质，包含噪声、UV 扰动、遮罩、膨胀和淡出。
- 保存 `M_Shockwave_Prototype`、`BP_Shockwave_Prototype` 和预览地图。
- 重新启动 UE 加载并检查材质统计。现有技能 DataAsset 哈希保持一致。

## 查看

在 UE 内容浏览器打开 `/Game/UnityVFXProbe/Maps/L_Shockwave_Probe`。
蓝图：`/Game/UnityVFXProbe/Blueprints/BP_Shockwave_Prototype`。
材质：`/Game/UnityVFXProbe/Materials/M_Shockwave_Prototype`。

预览材质 `LoopPreview=1`，一秒播放、一秒间隔。若以后接入运行时，需创建动态材质实例，设置 `LoopPreview=0`、`StartTime=当前游戏时间`、`Duration=1`，并在播放结束后销毁特效 Actor。当前没有自动接入任何技能。

## 可行性与范围

贴图、模型和声音适合复用；ShaderLab/URP shader、Unity 粒子模块、脚本和 prefab 需要在 UE 材质、Niagara 或蓝图中重新实现。当前只是一个网格特效重建样本，不是全目录转换器，也不是完成还原的 Niagara 特效。

源效果依赖粒子自定义顶点流。本次使用简化的时间参数替代，没有完整还原原粒子曲线、深度淡化和自定义流，尚未验证与 Unity 原版的画面一致性。

源材质保存的 `_TextureSample5` 引用了缺失 GUID `0325fcc4a3af92d42bf31cd760509112`；所读取 shader 的实际采样路径未使用该属性。该缺项仍保留在清单中，不宣称源依赖完整。

## 重跑和记录

1. 用普通 Python 运行 `Scripts/prepare_unity_vfx_probe.py`，只读取 Unity 项目，将依赖副本放到本项目 `SourceArt/UnityVFXProbe`。
2. UE 编辑器 Python 运行 `Scripts/import_unity_vfx_probe.py`；脚本会重建专属测试材质和预览地图，不应在其中存放手动编辑内容。
3. UE 编辑器 Python 运行 `Scripts/verify_unity_vfx_probe.py`，重新加载资产并校准预览比例。原网格导入半径约 10000 cm，测试 Actor 使用 0.03 倍缩放。

首次自动视口截图没有得到可用画面。后续改用 UE SceneCapture2D 输出实际渲染图 `Saved/UnityVFXProbe/Shockwave_UE_Render.png`：为便于观察，在单独复制的截图材质中把时间固定为 0.35 秒，调整截图场景比例和网格方向；没有改动原始动态材质，也没有保存截图场景的临时变换。

截图可见白色弧状片段，尚未形成完整冲击波。它验证了该网格和重建材质能参与实际渲染，不能证明动态过程或原版画面还原成功。画面较暗、弧光不完整，是当前样本待解决的问题。

截图脚本：`Scripts/capture_unity_vfx_probe.py`。该脚本通过重建材质的副本 `M_Shockwave_Snapshot` 固定时间，使用 UE 场景捕获直接输出 PNG，没有使用生成式图片或后期合成。

清单：`SourceArt/UnityVFXProbe/manifest.json`。
结果：`Saved/UnityVFXProbe/result.json`。
日志：`Saved/Logs/UnityVFXProbe.log`、`Saved/Logs/UnityVFXProbeVerify.log`。

项目的 SmartFBX 插件在 FBX 导入时额外生成了默认材质和法线贴图，位于测试目录下 `Meshes/SmartMaterials`；试验网格实际绑定的是重建的冲击波材质。

## 迁移心得

这次试验最有价值的结论，是先把特效拆成“素材、外观逻辑、运动逻辑、游戏触发”四部分，再分别处理。贴图和 FBX 能带过去，效果成立所需的时间曲线、顶点数据和触发时机则要重新接起来。

### 1. 从一个有代表性的效果做通，避免直接批量导入

目录中有 628 个 prefab，但其中包含颜色变体和汇总副本。按文件数估算工作量会失真。应先识别共用的 Shader、贴图和网格，再按冲击波、刀光、法阵、护盾等实际类别选样本。一个类别的母材质和 Niagara 模板做通后，颜色、大小和速度变体才适合批量生成。

### 2. prefab 的依赖比名字更可靠

Unity 通过 GUID 引用材质、贴图和模型。按 GUID 递归提取依赖，能比人工寻找同名文件更清楚地确认资源来源。本次发现了一个缺失贴图 GUID，但它出现在材质保存的旧属性里，所读取 Shader 的实际采样路径没有使用它。因此，遇到缺项要继续检查使用路径，不能一律判定整个效果不可迁移，也不能把缺项静默忽略。

### 3. 最容易低估的是自定义粒子数据

原冲击波 Shader 的亮度、侵蚀等值来自 Unity 粒子自定义顶点流，单独搬运 Shader 公式无法得到完整效果。本次用时间参数做了近似替代。后续要还原，就需要把这些参数对应到 Niagara 粒子属性或动态材质参数，并逐项核对初始值和随时间变化的曲线。

### 4. 单位和坐标轴要先校准

本次 FBX 导入后的最大半尺寸约为 10000 cm，直接采用最初的预览缩放导致画面无法正确观察。之后按实际 bounds 归一化比例，并在截图场景中调整网格朝向。以后每批模型先检查尺寸、轴向、枢轴和 UV，再调整发光强度，能避免把尺寸或视角问题误判成 Shader 问题。

### 5. 编译通过与视觉通过要分开验收

材质编译、蓝图绑定、关卡加载和视觉还原是不同的检查项。此次已经确认前面几项，但没有 Unity 同机位、同一播放时刻的对照画面，所以不能宣称一比一还原。正式迁移时应固定播放阶段、相机、背景和曝光，至少比较起始、中段和消散三个阶段，再检查实际技能触发。

这次实际截图进一步说明：即使材质有可用的编译统计，外观仍可能只剩局部弧光。接下来应先核对网格 UV 和原粒子传入的遮罩偏移、侵蚀、亮度数据，再校准运动曲线；在这些项没有核对前，不能把当前缺口简单归因于某一张贴图或某一条公式。

### 6. 接入 GAS 应放在效果样本验收之后

当前只在独立目录验证素材与材质。下一步应先完成 Niagara 的生成、尺寸、颜色、消散与销毁，再由项目的技能表现入口或 GameplayCue 触发，并检查挂点、方向、重复播放和联机表现。当前样本尚未完成这些集成。

建议的后续顺序：先完成这个冲击波的动态验收，再迁移一个法阵和一个护盾，用三个样本覆盖常见材质与运动方式；随后建立可复用模板，再处理整个目录。
