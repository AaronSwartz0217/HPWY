# UCC_AnimationSetupLibrary

架构审计：虽然游戏运行代码没有调用，本类确实被Python动画配置和修复脚本使用，属于在用的编辑工具，不应作为无用类删除。后续可迁入Editor模块，保留脚本入口。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

新增编辑器兼容入口 `RepairLegacyWidgetPins(Blueprint)`：刷新节点，只移除名为DamageActor、无连线且已废弃的引脚，重新编译。返回移除数量，失败为-1；本轮用于迁入BP_Enermyfree，不进行全工程蓝图清理。见[开场与材质提示处理](../开场Logo与材质提示处理.md)。

父类：`UBlueprintFunctionLibrary`。编辑器生成工具，不挂到角色或关卡。

[头文件](../../Source/CrashCourse/Public/Characters/CC_AnimationSetupLibrary.h) · [实现](../../Source/CrashCourse/Private/Characters/CC_AnimationSetupLibrary.cpp) · [返回索引](../CPP父类配置索引.md)

## 接口

`BuildLocomotionGraph(Blueprint, Idle, Run, Jump, Fall, Land, Death)` 接收 Animation Blueprint 和六段动画，返回是否编译成功。运行时打包构建没有图编辑实现，返回 false；它不是游戏运行时创建 ABP 的接口。

Blueprint 应继承 `CC_CharacterAnimInstance`；六段动画及其 Target Skeleton 必须与当前模型一致。该函数会重建这个 Blueprint 的 AnimGraph，包含 CharacterLocomotion 六状态、目的状态 bool 过渡规则、0.12 秒过渡混合，以及输出前的 DefaultSlot。Idle/Run/Fall 循环，其余不循环；Run 使用 RunPlayRate。

## 使用方式

通常使用 `Scripts/configure_character_animation.py` 调用，无需手动搭每个模型的同样节点。当前 11 个模型各有一份独立 ABP，后续可以在编辑器继续调整。

生成器会替换目标 ABP 已有 AnimGraph 节点。正式美术调整前先复制 ABP，并将新副本填入 LocomotionLibrary 和相应 FormDefinition.Stage.AnimClass；不要在手工定制同一个 ABP 后再次运行生成脚本。

接口会验证六段动画的 Skeleton 与 ABP.TargetSkeleton 一致，不匹配时返回 false；它不负责跨骨架 IK 重定向、资源导入、角色输入、技能伤害或保存文件。导出、骨架比例修复、保存与角色引用设置由配套 Python 脚本完成，见 [动画配置指南](../动画状态机与技能表现配置.md)。


新增 DescribeBlueprintGraphs(UBlueprint*)：编辑器只读导出节点标题、Pin 默认值、资源引用与连线，用于核对旧技能；不修改或编译被检查的蓝图，打包版本返回空文本。

`IsNaniteBlendSupported(Material)`读取实例生效的BlendMode，仅Opaque／Masked返回true；编辑器资源修复脚本据此让含透明材质的网格使用传统渲染，不覆盖其材质外观。
