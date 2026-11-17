# UCC_ExperienceBackendSubsystem

架构审计：这是有意保留的后端适配层；当前没有原生自动调用或资产序列化调用证据。它不会自动保存游戏经验，也不负责本地存档；数据库未启动不能解释当前本地SaveGame的保存触发缺口。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

父类：`UGameInstanceSubsystem`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Backend/CC_ExperienceBackendSubsystem.h) · [实现](../../Source/CrashCourse/Private/Backend/CC_ExperienceBackendSubsystem.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

GameInstance 自动创建的经验 HTTP 适配器，不随 Pawn 更换而重建；后端目前只存经验。

## 配置方式

不要往角色加组件。按 Backend/README 设置并启动服务，在启动 UE 的进程环境设置 SPIRIT_BACKEND_URL 和 SPIRIT_BACKEND_TOKEN；蓝图 Get Game Instance Subsystem → CC Experience Backend Subsystem。绑定 OnCompleted 后显式发请求。

## 参数与接口

- LoadCharacterExperience(CharacterId)：先读版本；新角色 404 适配为成功返回 0／版本 0。
- SaveCharacterExperience(CharacterId,Experience)：用缓存版本与新 requestId 保存，第一次必须先读。
- RetryPendingSave：超时或不确定错误后重试完全相同的保存。
- OnCompleted(bSuccess,CharacterId,Experience,Version,Error)：结果；由上层记录正在读还是保存。
- RequestInFlight、HasPendingSave：只读运行状态。
- 可信读取完成后调用 Progression.RestoreExperienceFromBackend；保存回调不要拿旧快照覆盖游戏当前余额。

## 最小验证

用测试角色 hero_001 读取／保存，重启后端再读应一致；过期版本报 HTTP 409；密钥不要输出到日志。

## 当前限制

没有自动登录、角色归属验证、自动保存或断网队列。仅允许权威服务器／单机开发调用，共享密钥不得随发布客户端分发。当前未完成 UE 蓝图端到端联调。详细命令见 [后端说明](../../Backend/README.md)。
