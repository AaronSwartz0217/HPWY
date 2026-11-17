# UCC_FormDefinition

父类：`UDataAsset`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_GameplayData.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_GameplayData.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

一个形态的三档数据，原生和四种器物形态使用相同结构。

## 配置方式

创建 CC_FormDefinition Data Asset，设置稳定 FormId 与显示名称。Stages 固定三项，分别填初／中／高级的四技能 Loadout 和属性。原生填 PlayerState.Progression.NativeForm；器物同时填 ArtifactForms 与器物组件的 Form。

## 参数与接口

- FormId：存档键；DisplayName：显示名；Available=false 禁止交互／切换，加载存档也跳过。
- InventoryIcon：背包信物贴图，建议透明底方图；空值显示原生占位印记。InventoryName可单独配置背包名称；GetInventoryName为空时沿用DisplayName，InitialInk默认显示“初墨之力”。
- CanEvolve：默认 true。初始 DA_Form_InitialInk 为 false，不显示下一档费用，服务器拒绝其进化；固定三项 Stage 保持相同，只使用第 0 档。
- Stages[0] 初级、[1] 中级、[2] 高级。Cost 是进入该阶段的费用；初级交互解锁不读取其费用，原生初级免费。
- MaxHealth≥1、MaxMana≥0、MoveSpeed≥1；Attack 为实际攻击力（白板 15），Defense 用于减伤。
- Loadout：完整四技能组。
- Mesh：填写时使用该档模型；留空沿用缓存的初始网格体。
- MeshScale：该档填写 Mesh 时，它是 Mesh 组件的最终相对缩放，直接使用该值，不再乘初始 Pawn 的模型缩放；例如这批模型按 180 / 模型原始高度配置，使更换初始蓝图模型不会二次放大。该档 Mesh 留空时，才使用缓存初始模型缩放 × MeshScale，保留人形回退倍率。
- AnimClass：留空时原人形可使用缓存动画；不同骨架的自定义 Mesh 不套用人形动画，可由 Locomotion 组件选择模型专属 ABP。当前九档显式设置 Mesh/AnimClass。每档单独配置，不自动继承上一级。

## 最小验证

先只配一份原生与一份器物完成三档测试，再为其他三种器物填配置；进化应改变属性及阶段技能组。

## 当前限制

不自动修改胶囊体尺寸、模型相对位置、材质或碰撞。当前十一套模型已有各自重定向动画与 ABP，新增模型仍需提供骨架匹配的动画配置；三档不是自动插值，而是三份明确配置。




初始模型与白色自然方块的配置参见 [墨水人与自然之力配置](../墨水人与自然之力配置.md)。自然的 FormId 保持 Native，但它现在是需要通过方块取得并在背包使用的路线；初始墨水人使用独立 InitialInk ID。

InitialInk实际生命和攻击在解析Stage及永久加成后，按当前经验每点+1%并四舍五入；Stage内仍保留75/11.25原始基准，不手动写入成长后的数值。参数与信物配置见[格子背包与初墨成长](../格子背包与初墨成长配置.md)。
