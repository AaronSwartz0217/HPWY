# UCC_ExperienceFXSetupLibrary

架构审计：本类被Python经验粒子配置脚本使用，属于在用的编辑工具；WITH_EDITOR隔离现有实现，不能仅因没有游戏内调用而删除。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

父类：`UBlueprintFunctionLibrary`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ExperienceFXSetupLibrary.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ExperienceFXSetupLibrary.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

编辑器资产生成工具，构建经验粒子的基础 Niagara System。无需创建实例或挂组件，也不放进角色运行时蓝图。

## 接口

`Build Experience Orb System(System, Sprite Material)` 接收已创建的 Niagara System 和 Sprite 材质，返回是否配置且编译成功。工具复制引擎 SingleLoopingParticle 模板到目标系统，建立 CPU 模拟、局部空间、单 Sprite、固定小包围盒，设置基本颜色/尺寸/寿命并请求编译。引擎模板本身不被修改。

游戏中的物理散射与吸附由 ExperienceOrb Actor 完成，系统内 Sprite 保持局部静止，确保每个可见粒子与一个领取碰撞对应。默认经验粒子系统在 `/Game/HPWH/Effects/NS_ExperienceOrb`。

## 使用与限制

仅在完整编辑器工具或项目生成脚本中调用；非编辑器构建返回 false。项目生成脚本需要 `-RenderOffscreen`，不要使用 NullRHI 或 Python commandlet；运行方式见[维护脚本说明](../死亡布娃娃与经验粒子配置.md)。它会移除目标系统现有的 Emitter 再生成，因此正式修改前复制到自用目录并改怪物/经验包引用，避免覆盖已有定制效果。

资产配置、运行路径和验证见[经验粒子指南](../死亡布娃娃与经验粒子配置.md)。
