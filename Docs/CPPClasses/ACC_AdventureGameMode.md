# ACC_AdventureGameMode

父类AGameModeBase。由AdventureGameInstance为三张地图选用。

默认角色、控制器、PlayerState和HUD复用当前Progression蓝图及BP_MuralHUD。InitGame关闭旧角色的AutoPossessPlayer；StartPlay保留旧传送装置美术，接入原生触发区，恢复玩家主角视角并通知GameInstance真实抵达。

休憩旧门户到小镇；小镇不生成回城点，迁入旧门户在小镇隐藏且禁用，返回只使用N地图行旅列表。出生点使用地图PlayerStart，美术可直接调整。运行时覆盖优先于迁入地图保留的旧WorldSettings.GameMode；编辑器资产中仍可能显示旧类。

编译记录见Saved/Logs/SprintTownBuild.log，运行测试由前端进行。[完整配置](../地图传送与小地图配置.md)。
