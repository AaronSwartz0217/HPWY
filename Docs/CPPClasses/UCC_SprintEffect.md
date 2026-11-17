# UCC_SprintEffect

父类 `UGameplayEffect`。原生玩家固有疾跑专用效果，不需要创建 GE 蓝图或放进四技能 Loadout。

无限时长，由 `ACC_PlayerCharacter` 持有唯一活动句柄，最长5秒主动结束。`MoveSpeed` 使用 Multiplicitive 修饰，SetByCaller 的 DataName 为 `SprintMultiplier`，参数来自角色 `SprintSpeedMultiplier`，默认1.5。结束时移除自己的句柄，让 GAS 重新计算当前真实移速；不把旧速度写回，不移除其他速度效果。

持续／松开／冷却、死亡及切图清理由角色处理；此类本身不管理输入、计时和冷却，也不改变任何形态定义或永久数值。速度属性沿用 AttributeSet 到 CharacterMovement 的同步逻辑。

[疾跑操作与配置](../固有疾跑与小镇返回配置.md) · [角色父类](ACC_PlayerCharacter.md) · [索引](../CPP父类配置索引.md)
