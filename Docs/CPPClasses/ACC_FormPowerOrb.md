# ACC_FormPowerOrb

父类：ACC_ExperienceOrb。由器物组件创建的一颗元素能力球，无需手动摆放。

- RewardForm：服务端设置并复制的力量资产。
- Recipient / ExperienceValue：继承字段，领取者与经验价值，默认50。
- Core / Accent / Glow：彩色主球、环绕辅色小球与无阴影点光。
- PowerColor / AccentColor：复制的主辅色；PowerMaterial需有Tint向量参数。
- SourceArtifact：服务端弱引用，未领取球销毁时恢复原方块。

沿用经验球的物理散射、加速吸附、存活与胶囊接触校验，接触后同时入包和加经验，不自动变身。默认0.45秒后吸附、60秒生命周期，可在派生蓝图调整。Niagara装饰使用继承的ParticleSystem，默认已有原生球体外观。

本轮已通过当前工程编译，运行联调由前端进行。[详细配置](../元素信物与能力球配置.md) · [父类](ACC_ExperienceOrb.md)。
