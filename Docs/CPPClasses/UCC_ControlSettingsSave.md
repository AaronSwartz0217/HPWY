# UCC_ControlSettingsSave

父类 `USaveGame`。由 PlayerController 自动创建，不创建蓝图或拖场景。

`CombatKeys` 固定六项，顺序是技能 1–4、普攻、交互。写入 `Saved/SaveGames/SpiritTalesControls.sav`；SetupInputComponent 读取后校验数量、重复键和模板保留键，无效时使用默认。修改 / 重置键位自动保存。

它只保存本机键位，不保存经验，也不访问后端。角色成长和技能装备仍由 UCC_ProgressionSave 负责。

[头文件](../../Source/CrashCourse/Public/Player/CC_ControlSettingsSave.h) · [控制器](./ACC_PlayerController.md) · [返回索引](../CPP父类配置索引.md)
