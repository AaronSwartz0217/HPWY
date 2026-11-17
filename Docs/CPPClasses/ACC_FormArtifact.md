# ACC_FormArtifact

父类：`AActor`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ArtifactComponent.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ArtifactComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

可直接拖入场景的器物 Actor 父类，已创建 Mesh 和 Artifact 组件，默认复制并带 SpecialArtifact 分类标签。

## 配置方式

直接使用 BP_Artifact_Nature（绿／自然）、BP_Artifact_01（蓝黄／青铜）、BP_Artifact_02（黄棕／陶塑），或创建以 CC_FormArtifact 为父类的蓝图。在 Mesh 指定静态网格、材质和碰撞，在 Artifact.Form 指定相应形态。

## 参数与接口

- Mesh：器物外观，当前示例为方块。
- Artifact：CC_ArtifactComponent，配置形态、启用状态和交互距离。
- 交互距离按物体表面判断。ColorBand显示辅色；构造和BeginPlay从Artifact.GetPowerColors统一着色，RefreshPowerPresentation可手动调用。

## 最小验证

在关卡摆三个实例，各引用对应Form；E后方块消失并出球，球接触后入包并增加50经验，背包使用才变身。

## 当前限制

这是便捷父类，不是所有可交互场景 Actor 的强制父类。更换标签不会更换形态。
