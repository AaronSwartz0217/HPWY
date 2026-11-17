# UCC_ProgressionSave

父类：`USaveGame`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ProgressionSave.h) · 本类仅在头文件定义，无独立 .cpp · [返回索引](../CPP父类配置索引.md)

## 用途

本地 SaveGame 数据容器，保存经验、原生档位、器物档位、当前形态和商品等级。

## 配置方式

由 Progression.SaveProgress 创建，不手动挂载，也无需创建子蓝图。读取走 Progression.LoadProgress，避免只还原经验而漏掉形态／商品等级。

## 参数与接口

- Version：存档格式版本，当前4；读取兼容版本1/2/3。
- NativeFormId：原生形态的稳定 ID，防止更换初始角色后把旧自然档位应用到墨水人。
- Experience、CopperCoins、ManualSkillIds、NativeTier、SelectedForm、FormTiers、UpgradeLevels、VisitedRegions：保存的数据。
- FormId／UpgradeId 是存档键，因此正式使用后保持稳定。

## 最小验证

保存后重启单机，在角色初始化后调用 LoadProgress；检查余额及形态档位、购买等级。

## 当前限制

Adventure框架启动恢复、切图前/抵达后自动保存，设置页仍可手动保存。兼容旧v1～v3；不支持网络客户端导入钱包，不保存临时效果或世界实体。

## 2026-10-24 数值与技能配置更新

新增 ManualSkillIds 数组，以稳定 SkillId 保存四槽手动选择。读取时仅恢复目录内、不重复的四个技能；旧存档没有字段时回退形态默认组。本次初始形态调整后保存版本为 2，读取仍兼容旧版本 1。角色存档经菜单手动保存 / 读取，键位存档独立。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。


## 初始角色版本迁移

版本 1 中的 NativeTier 属于旧自然路线。NativeForm 变为 InitialInk 时，读取将自然路线加入 Forms 并恢复旧档位，NativeTier 重置为 0；旧档选中原生时转为选中自然，选中青铜／陶塑时保持原选择。自然配置尚未列入目录时读取失败，当前余额不变。新档的 NativeFormId 明确为 InitialInk。规则详见 [墨水人与自然之力配置](../墨水人与自然之力配置.md)。

## 2026-10-24真实技能与铜币更新

Version=3，新增CopperCoins；旧v1/v2铜币从0开始，其余原字段保留。

详见[真实技能与铜币配置](../真实技能与铜币配置.md)。

## 地区扩展v4

新增VisitedRegions，以Rest/Town/Arena稳定Id保存访问记录。初始/测试区默认可传送，Town抵达后记录。与钱包、档位及技能共用SpiritTales.sav；不存在独立的地区数据库。详见[地图传送与小地图](../地图传送与小地图配置.md)。
