# ACC_SkillProjectile

父类：`AActor`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_SkillProjectile.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_SkillProjectile.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途与配置

新模板的原生投射物，支持球形碰撞、运动、追踪、穿透、同目标命中去重，以及 Niagara 拖尾和命中特效。由内置能力直接生成，不手工放入关卡。

在 SkillDefinition 配 ProjectileSpeed、Range、Count、ProjectileNiagara、ImpactNiagara/ImpactParticles、EffectScale。当前占位拖尾使用 LanTuRebuild 的 MixedVFX Projectile_01。

## 接口与生命周期

- Collision 半径 16，Pawn 重叠，其他默认阻挡；忽略来源 Actor 的运动碰撞。
- Movement 无重力、朝向速度方向，启用 Actor 运动复制。
- `Initialize(Skill, Damage, Target)` 是 C++ 初始化接口。
- Homing 用目标根组件追踪；Piercing 用 Count 作命中数，不能重复打同一个 Actor。
- 生命周期取 Range/ProjectileSpeed，限制在 0.1～20 秒。
- Definition 复制后 OnRep_Presentation 创建附着 Niagara；BeginPlay 也覆盖服务器/单机初始化路径，避免重复生成。专用服务器不创建视觉效果。
- 命中角色时先由权威逻辑结算并广播命中特效；碰到阻挡场景时在命中点播放效果并销毁。附着拖尾随投射物清理，独立命中特效按 EffectLifetime 清理。

## 最小验证与限制

对存活敌方目标发射，检查弹体拖尾、命中反馈、扣血和销毁；穿透不能重复伤害一个 Actor，撞墙应销毁。渲染模式的 CC.AnimationSmokeTest 检查实际附着 Niagara 组件；完整命中行为继续用战斗与多人实测验收。

模板仍生成固定的原生 CC_SkillProjectile。只创建此类的蓝图子类不会被自动选中；若要网格弹体、自定义碰撞与专属逻辑，需扩展 ProjectileClass 配置字段后接入。
