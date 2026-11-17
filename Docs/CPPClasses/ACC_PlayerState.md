# ACC_PlayerState

父类：`APlayerState`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Player/CC_PlayerState.h) · [实现](../../Source/CrashCourse/Private/Player/CC_PlayerState.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

玩家持久于 Pawn 生命周期之外的运行时状态：创建 ASC、AttributeSet 和 Progression 组件。

## 配置方式

复制 BP_ProgressionPlayerState，设置 Progression 的 Native Form、Artifact Forms、Upgrade Catalog；在 GameMode 的 Player State Class 指定此蓝图。不要在 Pawn 上另建第二份玩家 ASC 或 Progression。

## 参数与接口

- Progression：经验、形态和购买配置入口。
- AbilitySystemComponent：玩家复制模式 Mixed。
- AttributeSet：属性实例，由 C++ 创建，无需额外挂载。
- ASC 的 OwnerActor 为 PlayerState，AvatarActor 为实际角色。

## 最小验证

进入游戏后从 PlayerController 获取 PlayerState，Cast 到派生蓝图，应能读取 Progression.Experience。

## 当前限制

PlayerState 不会自行把数据保存到磁盘或后端；跨关卡、重连、账号绑定仍要单独编排生命周期。
