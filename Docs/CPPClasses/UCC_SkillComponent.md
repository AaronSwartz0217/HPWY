# UCC_SkillComponent

父类：`UActorComponent`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_SkillComponent.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_SkillComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

管理四槽能力的授予、撤销、激活与等级刷新，已经由 BaseCharacter 创建。

## 配置方式

不重复挂载。怪物设置 DefaultLoadout；玩家由形态 Progression 应用技能组，NativeForm 为空时才回退 Skills.DefaultLoadout／原始技能。通过角色 Skills 引用调用蓝图接口。

## 参数与接口

- DefaultLoadout：默认技能组。
- BasicAttack：四技能之外的独立普攻定义，测试玩家已配置 DA_BasicAttack；ActivateBasicAttack() 请求普攻。独立冷却，不占 Slots[0..3]。
- SummonedSoldierClass：召唤兵人派生蓝图类型；留空使用原生ACC_SummonedSoldier。出生时按该类胶囊尺寸找落地点，保留最多3名、重新召唤替换、死亡/清理移除的规则。配置见[兵人父类](ACC_SummonedSoldier.md)。
- CancelActiveSkills()：权威端取消技能组管理的能力，供巡逻敌人死亡、停止和脱战时使用；不撤销已授予的技能。
- CurrentLoadout：当前使用的技能组，只读复制数据。
- ActivateSlot(Slot,Target)：Slot 0～3，Target 可空；非权威端返回 true 仅代表发出请求，不代表技能最终成功。
- GetCooldownRemaining(Slot)：查询槽位冷却。
- GetCooldownRemaining(4) 查询内部普攻通道；普通技能索引仍是 0～3，ActivateSlot 不接受第五技能槽。
- ApplyLoadout、Initialize、RefreshLevels：C++ 内部接口，当前没有标注 BlueprintCallable。

## 最小验证

形态切换前后检查技能组和技能数；冷却期间换形态仍应保留对应槽冷却。

## 当前限制

冷却当前按槽位共享，不是按 SkillId 共享；同一技能跨不同槽位的冷却策略尚需完善。原技能恢复识别旧三槽及 Enemy.Attack 标签，不会自动管理任意自定义公共能力。

## 2026-10-24 数值与技能配置更新

CurrentSlots 为复制给客户端的实际四槽；CurrentLoadout 保留共享模板资产引用。ApplyLoadout 合并 PlayerState.Progression.ManualSkills 后授予四个能力，不创建无法网络引用的临时 DataAsset。HUD 与 AI 从 CurrentSlots 读取。BasicAttack 始终独立。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。

## 正式 UMG 刷新接口

`CurrentSlots` 使用 RepNotify。`OnLoadoutChanged()` 在服务端 ApplyLoadout 成功后和客户端 OnRep_CurrentSlots 到达时广播。UI 绑定后立即读取一次 CurrentSlots；不要从共享模板 CurrentLoadout.Slots 绘制当前手动选择。等级显示为 Slot.Level + Progression.GetSkillBonus(SkillId)。冷却通过 GetCooldownRemaining(0..3) 查询，独立普攻用 4；按短间隔计时器更新倒计时，组件未新增每帧冷却事件。

[按步骤接入 UMG](../UI操作与UMG接入配置.md)。

## 2026-10-24真实技能与铜币更新

新增按键释放、真实施放记录、流派状态与召唤管理；LastExecutedSkill/LastExecutedSlot可用于调试。状态包括控制、金身、正面防御、隐身和吸血；死亡/开发重置清理。

详见[真实技能与铜币配置](../真实技能与铜币配置.md)。


## 输入衔接与中断

`Skills.InputBufferSeconds` 默认 0.18 秒，可在角色蓝图的 Skills 组件调整为 0–0.35 秒；0 关闭缓冲。仅玩家角色启用，AI 不会自动补发攻击。

前台动作占用或冷却即将结束时，只保存最后一次合法输入；真正激活才扣费。缓存过期、死亡、受眩晕、换配装、改键、失去角色控制、打开背包／旅行菜单时清除，避免稍后误放。缺蓝、未解锁和冷却尚长的输入不会被排队。普通技能的轻点松手仍保留有效缓存；蓄力技能在松手后不会从缓存延迟起蓄。

前摇、连拳和蓄力使用前台动作锁。已进入持续阶段的回复、重力场和其他设计效果不会因资产沿用 Melee 类型而锁住后续攻击。`CancelPendingInput()` 取消待释放输入及前台动作，保留已经生效的后台持续效果；`CancelActiveSkills()` 仍供死亡、眩晕等需要完全打断的场景使用。

定身阻止 Dash 位移，保留静止攻击。冷却、法力和伤害资产参数未重配。服务端仍负责技能执行，未新增客户端预测。`GetQueuedSkillSlot()` 用于服务端状态调试。

新逻辑在 `Private/Progression/CC_SkillInput.cpp`。回归见 `Saved/Logs/SkillMechanicsFinalTests.log`；实际键位、派系技能及菜单打断烟测见 `Saved/SkillMechanicsSmokeUser/Saved/RealSkillsSmoke.json`。
