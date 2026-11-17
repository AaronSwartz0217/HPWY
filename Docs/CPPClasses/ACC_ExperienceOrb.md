# ACC_ExperienceOrb

父类：`AActor`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ExperienceOrb.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ExperienceOrb.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途与接入

一颗有服务器碰撞和经验价值的 Niagara 经验粒子。怪物首次致死与经验包交互调用 `Spawn Burst` 生成，不需要手动往关卡摆放。根组件 Collision 是 Sphere，Particles 是 NiagaraComponent；视觉跟随服务器复制的 Actor 位置。

## 字段与接口

- Recipient：领取者 PlayerState，只读；重新查找其当前活角色，不依赖死去的怪物或旧 Pawn。
- Experience Value：这一颗的经验值；当前 10/50 波次均为 1。
- Phase：Scattering → Seeking → Collected，只读；收集后销毁。
- Particle System：默认 `/Game/HPWH/Effects/NS_ExperienceOrb`，支持替换。
- Scatter Duration：0.45 秒。散射阶段模拟物理/重力，只阻挡静态世界，不提前领奖。
- Initial / Maximum Seek Speed：350 / 1800 cm/s。Seek Acceleration：1600 cm/s²。
- Maximum Lifetime：60 秒，超时销毁且不入账。
- Spawn Burst(World Context, Recipient, Origin, Experience Amount, Particle System)：权威逻辑生成整波，返回生成 Actor 数量。1～128 时一颗 1 经验；更大数值最多生成 128 颗且保留总价值。
- Begin Seeking：服务器切换到吸附阶段，通常由 Tick 自动调用。
- Try Collect(Character)：验证服务器、Seeking 阶段、正确领取者、活着且与胶囊接触；成功时一次入账并销毁，重复调用返回 false。

## 最小验证

击杀一只怪，先看到十颗散射，随后向击杀者加速飞行；钱包逐颗变化，最终增加 10。超距或其他玩家不能用 Try Collect 领取它；人物死亡期间不向尸体入账。流程和测试见[经验粒子指南](../死亡布娃娃与经验粒子配置.md)。

## 限制

当前不保存未领取掉落；超过 60 秒会丢失。服务器负责物理、移动、领取与钱包，Niagara 负责显示；不能把 Niagara 碰撞回调自行接成加经验。大量同时死亡的 Actor 数量与多人移动平滑仍需压力测试。

## 元素球扩展（2026-10-24）

增加protected虚函数GrantReward，领取预占后再提交奖励，失败退回Seeking。普通经验球继续加经验；新ACC_FormPowerOrb复用移动与接触校验，同时发力量和50经验。此轮实现已通过当前工程编译，运行测试由前端进行。
