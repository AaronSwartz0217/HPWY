# UCC_MeleeAttack

2026-10-25审计：未在当前检查范围检出该旧通知的外部原生或资产序列化引用，列为待核对的清理候选，尚未删除。它仍参与模块编译；DrawDebugSphereTraceMulti已用ENABLE_DRAW_DEBUG保护，修复Shipping无法编译的问题，不改扫击或事件逻辑。[审计说明](../Architecture/C++架构审计与优化路线.md)。

父类：`UAnimNotifyState`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Notifies/CC_MeleeAttack.h) · [实现](../../Source/CrashCourse/Private/Notifies/CC_MeleeAttack.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

旧敌人近战动画通知状态，在通知持续期间每帧沿 Socket 做球扫并发出 MeleeTraceHit 事件。

## 配置方式

在攻击动画／蒙太奇的通知轨添加 CC_MeleeAttack NotifyState，调整持续区间，填当前骨架存在的 SocketName。敌人攻击 GA 监听 CCTags.Events.Enemy.MeleeTraceHit 并做伤害结算。

## 参数与接口

- SocketName：默认 FX_Trail_01_R。
- SocketExtensionOffset：默认 40。
- SphereTraceRadius：默认 60。
- DrawDebugs：默认开启，正式表现可关闭。

## 最小验证

观察调试球扫覆盖武器命中区间；一次攻击同一玩家应只按设计次数扣血。

## 当前限制

当前每帧都会发送事件，去重应由攻击 GA 或后续通知状态管理实现。它只识别玩家角色，不是新模板的全阵营通用检测。
