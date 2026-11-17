# 角色经验后端（开发框架）

当前只保存**角色可用经验余额**，与游戏中可用于进化、购买加成的 Experience 对应。角色 ID 为主键；版本、更新时间和请求去重记录是保存所需的元数据。不保存形态、技能、商品等级、背包或账号信息。

实现：Node.js 24.14+（24.x）、原生 HTTP、内置 SQLite。无需 npm install、外部数据库或额外运行库。当前 Node 对内置 SQLite 会输出 ExperimentalWarning，已在本机 Node 24.14.0 验证。

## 本地启动

在项目的 Backend 目录执行 PowerShell：

```powershell
Copy-Item .env.example .env
node -e "console.log(require('node:crypto').randomBytes(32).toString('hex'))"
```

将输出的随机密钥填入 `.env` 的 `EXPERIENCE_API_TOKEN`，然后：

```powershell
npm.cmd start
```

默认监听 `http://127.0.0.1:3001`，数据库文件为 `Backend/data/experience.sqlite`。Ctrl+C 关闭；重启读取同一数据库，经验仍在。`.env` 与数据库已加入忽略规则，不应提交密钥或玩家存档。

本次交付没有安装系统服务或启动常驻后台进程。

## 接口

除健康检查外均要求：`Authorization: Bearer <EXPERIENCE_API_TOKEN>`。

| 方法 | 路径 | 用途 |
|---|---|---|
| GET | `/health` | 进程健康检查，无角色数据 |
| GET | `/v1/characters/{characterId}/experience` | 读取经验与版本，不存在返回 404 |
| PUT | `/v1/characters/{characterId}/experience` | 创建或保存经验快照 |

角色 ID 只允许字母、数字、下划线、短横线，长度 1～64。它是稳定角色标识，不是角色昵称；目前由可信调用方提供，尚未接入登录账号。

保存请求：

```json
{
  "experience": 120,
  "expectedVersion": 0,
  "requestId": "a-unique-save-request-id"
}
```

- 新角色使用 expectedVersion=0；已有角色必须先读取，使用返回的 version 保存。
- 经验必须为 0～2147483647 的整数，允许变小以记录消费后的余额。
- 每次新保存生成唯一 requestId。超时后重试同一请求，保留相同 requestId、经验值及 expectedVersion。
- 同一 requestId 换了内容返回 409；相同内容重试返回原结果，不会重复更新版本。
- 版本过期返回 409/version_conflict，不自动覆盖。调用方重新读取并决定如何处理，不能强制覆盖其他会话的新存档。

成功响应示例：

```json
{
  "characterId": "hero_001",
  "experience": 120,
  "version": 1,
  "updatedAt": "2026-10-23T11:00:00.000Z"
}
```

错误：400 参数或 JSON 无效；401 密钥无效；404 角色／路由不存在；405 方法不支持；409 版本或请求 ID 冲突；413 请求超过 8 KB；415 非 JSON；500 服务内部错误。错误响应不返回堆栈或密钥。

SQLite 使用事务、WAL 及版本比较，经验保存与请求去重同时提交。数据库包含 schema version，后续扩展时可添加迁移。

## UE 接入

新增 `CC_ExperienceBackendSubsystem`（GameInstance Subsystem）。默认不自动请求，不更改原来的本地存档流程。

启动 UE／游戏服务器**之前**，在相同终端设置：

```powershell
$env:SPIRIT_BACKEND_URL = 'http://127.0.0.1:3001'
$env:SPIRIT_BACKEND_TOKEN = '<与后端相同的私有密钥>'
# 从此终端启动 UnrealEditor 或可信游戏服务器，才能继承上述环境变量。
```

蓝图接法：

1. Get Game Instance Subsystem，选择 `CC Experience Backend Subsystem`。
2. 绑定 `On Completed`。建议用一个存档管理蓝图区分当前进行的是读取还是保存。
3. 角色进入游戏前调用 `Load Character Experience(CharacterId)`。只应对通过可信登录／开发配置确定的角色 ID 操作。
4. 成功时，将返回的经验传给 PlayerState → Progression → `Restore Experience From Backend`。新角色的后端 404 被适配为成功返回经验 0、版本 0，以便后续创建。
5. 读取完成后再开放经验奖励和消费。不要在战斗中用旧读取结果覆盖已经变化的余额。
6. 要保存时，取 Progression 的当前 Experience，调用 `Save Character Experience(CharacterId, Experience)`。
7. 保存成功只代表该次快照写入成功。如果保存期间又有收益或消费，应在成功后再保存最新余额；不要用保存回调中的旧余额覆盖游戏当前值。

接口一次只处理一个请求。首次保存前必须先读取；`request_in_flight` 表示当前有请求。网络超时或不确定的 5xx 保留待保存内容，调用 `Retry Pending Save` 重试原请求。重试未解决前不允许提交新的快照，以免重复／乱序写入。确定性的 4xx 清理待保存状态，下一次保存前需重新读取；409 的冲突解决由上层流程决定。

若应用在不确定请求后退出，重新启动时先读取后端当前值。没有后台自动重试队列、自动保存或断网缓存；这些策略留给后续存档管理流程。

## 信任边界与当前限制

- 这是可信游戏服务器／本地联调用的存档 API。共享密钥可以访问所有角色，不是玩家登录 token；不提供账号注册、登录或角色归属校验。
- UE 网络客户端会被 Subsystem 拒绝；仅在权威服务器或单机开发进程调用。发布客户端不能携带这个共享密钥。单机程序也不能作为防作弊的可信边界。
- 默认仅监听本机。部署公网前接入 HTTPS、玩家身份和角色归属校验，以及部署环境的限流和数据库备份。
- 经验发放和商品价格仍由游戏权威逻辑决定；此 API 保存快照，不自行计算击杀奖励或商城扣费。
- 本轮后端只存经验。原生／器物档位、属性商品等级仍在游戏／本地存档中；云端经验与本地购买记录尚未构成完整的原子存档，正式跨设备进度需要一起扩展后再启用。
- 不自动合并两台设备的消费。不要直接将两个余额相加；版本冲突必须由存档管理策略处理。
- 去重记录目前保留全部请求；正式长期服务需要制定保留和清理策略。

## 测试

```powershell
npm.cmd test
```

6 组测试运行真实本地 HTTP 服务，覆盖鉴权、首次创建、读取、消费余额、角色隔离、重复请求、并发冲突、输入限制、SQLite 重新打开后的持久化。测试使用临时数据库，不操作正式经验文件。

UE HTTP 适配器经编译检查；本轮未执行 UE 蓝图到后端的完整联调或公网部署。
