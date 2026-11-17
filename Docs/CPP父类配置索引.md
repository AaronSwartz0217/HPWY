# C++ 父类配置索引

覆盖当前 Source/CrashCourse/Public 中全部 69 个项目 C++ 类，每类一份独立文档。A／U 是 C++ 类名前缀，编辑器通常显示不带前缀的 CC 名称。类在 .h 声明，行为通常在 .cpp 实现；同一文件可能定义多个类，纯数据类也可能只有头文件。

2026-10-25 已完成全部69类的源码、脚本、蓝图和数据资产架构审计：[逐类使用结论与优化路线](Architecture/C++架构审计与优化路线.md)。区分当前主链、兼容依赖、编辑工具、备用入口、预留后端及5个清理候选；未删除或重设蓝图父类。

## 建议阅读顺序

v0.3 独立插件新增 [`UWFCLayoutAsset`](CPPClasses/UWFCLayoutAsset.md)，保存可复用的网格规划。编辑器模块 `SpiritWFCEditor` 提供填格和模型替换窗口，见[操作教程](../Plugins/SpiritWFC/Docs/网格场景编辑器完整教程.md)。

独立插件新增 3 类：[`AWFCMapGenerator`](CPPClasses/AWFCMapGenerator.md)、[`UWFCTileSet`](CPPClasses/UWFCTileSet.md)、[`UWFCBlueprintLibrary`](CPPClasses/UWFCBlueprintLibrary.md)。它们位于 `Plugins/SpiritWFC`，不计入上方原模块的 69 类；另有无 World 的普通 C++ 求解器 `FWFCSolver`。[插件使用与算法入口](../Plugins/SpiritWFC/README.md)。

1. [下一步开发方向](./下一步开发方向.md)。
2. [从零配置与接入步骤](./从零配置与接入步骤.md)。
3. 按下面索引查看类的用途、字段、接线和限制。
4. [动画状态机与技能表现配置](./动画状态机与技能表现配置.md) 与 [死亡布娃娃与经验粒子配置](./死亡布娃娃与经验粒子配置.md)。
5. [器物与技能配置指南](./器物与技能配置指南.md) 与 [后端说明](../Backend/README.md)。

## 类型怎么使用

| 类型 | 使用方式 |
|---|---|
| Actor／Character／Controller／State | 创建派生蓝图；游戏框架类再交给 GameMode 生成 |
| ActorComponent | 挂到蓝图 Actor，或配置已经自动创建的组件 |
| DataAsset | 创建 Data Asset 实例并填写引用，不拖场景 |
| GameplayAbility／GameplayEffect | 能力／效果类，由 ASC 授予和应用 |
| UserWidget／WidgetComponent | 创建控件蓝图／挂组件 |
| GameInstanceSubsystem | 引擎自动创建，使用 Get Game Instance Subsystem 获取 |
| SaveGame／异步代理／函数库 | 由接口创建或静态调用，不挂场景 |

## 角色与输入

| 类与配置文档 | 用途 |
|---|---|
| [ACC_PatrolEnemyCharacter](./CPPClasses/ACC_PatrolEnemyCharacter.md) | 可继承的范围巡逻、发现玩家、追击攻击、脱战返回父类 |
| [ACC_BaseCharacter](./CPPClasses/ACC_BaseCharacter.md) | 玩家和怪物的公共角色基类，提供存活状态、初始能力与属性初始化，构造时已经创建 Skills 组件 |
| [ACC_PlayerCharacter](./CPPClasses/ACC_PlayerCharacter.md) | 玩家 Pawn |
| [ACC_EnemyCharacter](./CPPClasses/ACC_EnemyCharacter.md) | 怪物角色，自身持有 ASC 和属性集，负责健康监听、停止移动与击飞落地状态 |
| [ACC_PlayerState](./CPPClasses/ACC_PlayerState.md) | 玩家持久于 Pawn 生命周期之外的运行时状态：创建 ASC、AttributeSet 和 Progression 组件 |
| [ACC_PlayerController](./CPPClasses/ACC_PlayerController.md) | 管理移动、视角、跳跃、四技能和器物交互的输入 |

## 器物与成长

| 类与配置文档 | 用途 |
|---|---|
| [ACC_FormArtifact](./CPPClasses/ACC_FormArtifact.md) | 可直接拖入场景的器物 Actor 父类，已创建 Mesh 和 Artifact 组件，默认复制并带 SpecialArtifact 分类标签 |
| [UCC_ArtifactComponent](./CPPClasses/UCC_ArtifactComponent.md) | 让已有 Actor 蓝图成为特殊器物 |
| [UCC_ProgressionComponent](./CPPClasses/UCC_ProgressionComponent.md) | 玩家的经验钱包、原生／器物进化、商品购买和本地存档管理；已经由 PlayerState 创建 |
| [ACC_ExperienceOrb](./CPPClasses/ACC_ExperienceOrb.md) | 服务器物理散射、吸附与逐颗领取的 Niagara 经验粒子 |
| [ACC_ExperiencePack](./CPPClasses/ACC_ExperiencePack.md) | E 交互产生 50 颗经验粒子的黑色场景包 |

## 数据资产

| 类与配置文档 | 用途 |
|---|---|
| [UCC_MenuArtData](./CPPClasses/UCC_MenuArtData.md) | 壁画菜单共用背景、Logo、四导航图片与缺图回退数据 |
| [UCC_FormDefinition](./CPPClasses/UCC_FormDefinition.md) | 初墨及元素形态的三档数据，包含预留路线；当前共6个资产 |
| [UCC_SkillDefinition](./CPPClasses/UCC_SkillDefinition.md) | 一个技能的身份、行为模板与参数；当前注册表44个定义，玩家可装备目录39项，另有独立普攻和敌人定义 |
| [UCC_SkillLoadout](./CPPClasses/UCC_SkillLoadout.md) | 一组四技能，可用于玩家原生／器物的每个阶段，也可用于怪物 |
| [UCC_UpgradeDefinition](./CPPClasses/UCC_UpgradeDefinition.md) | 经验商品定义，支持全局属性购买和按 SkillId 购买技能等级 |

## 技能执行

| 类与配置文档 | 用途 |
|---|---|
| [UCC_SkillComponent](./CPPClasses/UCC_SkillComponent.md) | 管理四槽能力的授予、撤销、激活与等级刷新，已经由 BaseCharacter 创建 |
| [UCC_ConfiguredAbility](./CPPClasses/UCC_ConfiguredAbility.md) | 20 种模板共用的权威服务器技能执行器，技能配置由授予 Spec 的 SourceObject 提供 |
| [ACC_SkillProjectile](./CPPClasses/ACC_SkillProjectile.md) | 新模板使用的投射物，包含球形碰撞、运动、追踪、穿透与同目标命中去重 |

## GAS 与旧能力

| 类与配置文档 | 用途 |
|---|---|
| [UCC_AbilitySystemComponent](./CPPClasses/UCC_AbilitySystemComponent.md) | 自定义 ASC，支持授予时按标签自动激活和按能力类修改等级 |
| [UCC_AttributeSet](./CPPClasses/UCC_AttributeSet.md) | 角色 GAS 属性及伤害结算，包含复制通知和属性初始化事件 |
| [UCC_GameplayAbility](./CPPClasses/UCC_GameplayAbility.md) | 教程原有 GA 基类，主要在激活时显示可选调试信息 |
| [UCC_Primary](./CPPClasses/UCC_Primary.md) | 旧玩家主攻击基类，提供命中检测配置和批量发送敌人受击事件的方法 |
| [UCC_HitReact](./CPPClasses/UCC_HitReact.md) | 旧受击能力，缓存朝向和攻击者方向，供蓝图选择前后左右动画 |
| [UCC_SearchForTarget](./CPPClasses/UCC_SearchForTarget.md) | 服务器自动运行的敌人搜索／接近／攻击能力，支持新四槽和旧 Enemy.Attack 路径 |
| [UCC_MeleeAttack](./CPPClasses/UCC_MeleeAttack.md) | 旧敌人近战动画通知状态，在通知持续期间每帧沿 Socket 做球扫并发出 MeleeTraceHit 事件 |
| [ACC_Projectile](./CPPClasses/ACC_Projectile.md) | 旧教程敌人投射物，命中玩家时调用旧伤害工具并触发 Impact 表现 |

## 界面与工具

| 类与配置文档 | 用途 |
|---|---|
| [UCC_MuralMenuWidget](./CPPClasses/UCC_MuralMenuWidget.md) | 已生成 WBP_MuralMenu 的父类，全 viewport 主页、背包、技能、交易、设置与实际形态立绘/右上称号 |
| [UCC_OptionsWidget](./CPPClasses/UCC_OptionsWidget.md) | 设置页双列金边内容：六行改键、小地图开关、存读档和结果反馈 |
| [UCC_PowerTokenWidget](./CPPClasses/UCC_PowerTokenWidget.md) | 格子背包单件信物、可替换图标、档位/使用中/选中状态和选择事件 |
| [ACC_ProgressionHUD](./CPPClasses/ACC_ProgressionHUD.md) | 当前左下角拾取提示与可关闭的测试信息；正式BP_MuralHUD关闭调试面板 |
| [UCC_WidgetComponent](./CPPClasses/UCC_WidgetComponent.md) | 角色头顶等世界空间属性 UI 的 WidgetComponent，按 AttributeMap 自动寻找 CC_AttributeWidget 并绑定数值 |
| [UCC_AttributeWidget](./CPPClasses/UCC_AttributeWidget.md) | 单个血条／蓝条等属性控件的 UserWidget 父类 |
| [UCC_AttributeChangeTask](./CPPClasses/UCC_AttributeChangeTask.md) | 蓝图异步监听某个 GAS 属性变化 |
| [UCC_WaitGameplayEvent](./CPPClasses/UCC_WaitGameplayEvent.md) | 旧搜索 AI 使用的 GameplayEvent 异步代理，提供额外的 C++ StartActivation 入口 |
| [UCC_BlueprintLibrary](./CPPClasses/UCC_BlueprintLibrary.md) | 静态蓝图函数库，无需实例、组件或父蓝图 |

## 存档与后端

| 类与配置文档 | 用途 |
|---|---|
| [UCC_ProgressionSave](./CPPClasses/UCC_ProgressionSave.md) | 本地 SaveGame 数据容器，保存经验、原生档位、器物档位、当前形态和商品等级 |
| [UCC_ExperienceBackendSubsystem](./CPPClasses/UCC_ExperienceBackendSubsystem.md) | GameInstance 自动创建的经验 HTTP 适配器，不随 Pawn 更换而重建；后端目前只存经验 |

## 内置效果

| 类与配置文档 | 用途 |
|---|---|
| [UCC_InitialAttributesEffect](./CPPClasses/UCC_InitialAttributesEffect.md) | 测试角色生命、法力等默认属性初始化 |
| [UCC_DamageEffect](./CPPClasses/UCC_DamageEffect.md) | 内置 GameplayEffect，作用属性：IncomingDamage |
| [UCC_HealthEffect](./CPPClasses/UCC_HealthEffect.md) | 内置 GameplayEffect，作用属性：Health |
| [UCC_ManaEffect](./CPPClasses/UCC_ManaEffect.md) | 内置 GameplayEffect，作用属性：Mana |
| [UCC_ShieldEffect](./CPPClasses/UCC_ShieldEffect.md) | 内置 GameplayEffect，作用属性：Shield |
| [UCC_DefenseEffect](./CPPClasses/UCC_DefenseEffect.md) | 内置 GameplayEffect，作用属性：Defense |
| [UCC_AttackEffect](./CPPClasses/UCC_AttackEffect.md) | 内置 GameplayEffect，作用属性：Attack |
| [UCC_SpeedEffect](./CPPClasses/UCC_SpeedEffect.md) | 内置 GameplayEffect，作用属性：MoveSpeed |
| [UCC_CooldownEffect](./CPPClasses/UCC_CooldownEffect.md) | 内置 GameplayEffect，作用属性：无属性修改 |

## 不是蓝图父类的文件

- CrashCourse.h/.cpp：模块入口，不需要创建蓝图。
- CCTags.h/.cpp：原生 GameplayTag 定义；配置标签见 Config/DefaultGameplayTags.ini。
- CrashCourse.Build.cs、Target.cs：构建规则，不是场景类。
- Private/Tests/CC_ProgressionTests.cpp：自动化测试。
- FCC_SkillSlot、FCC_FormStage、FCC_FormProgress、FCC_UpgradeProgress 等为结构体，随资产配置或运行状态使用。

## 验证范围

此前基础版本的 UE 5.6 Editor Development 编译成功，成长交易与伤害、属性控件生命周期、巡逻状态切换三个自动化测试均通过，0 警告、0 失败。真实渲染经验 smoke 通过：10/50 颗、60 个物理/吸附对象与单 Sprite Niagara、逐颗共 60 经验、布娃娃/复活/十秒尸体清理均通过，详情见[死亡与经验指南验证结果](./死亡布娃娃与经验粒子配置.md)。此前实际渲染动画 smoke、四 NPC 巡逻 smoke 和菜单/九档模型 smoke 已通过，动画细节见[动画指南验证结果](./动画状态机与技能表现配置.md)及 Saved 报告。上述为前轮基础功能记录。本轮壁画菜单、初墨与背包使用流程的编译和运行验证尚未完成，后续以 Saved/MuralMenuSmoke.json 等本轮报告为准。单进程通过不能替代双客户端联机、全部技能命中边界和保存服务联调验收。

## 菜单、动画状态机与表现

| 类 | 用途 |
|---|---|
| [UCC_ProgressionMenu](CPPClasses/UCC_ProgressionMenu.md) | Tab 菜单、背包经验、进化购买 |
| [UCC_LocomotionComponent](CPPClasses/UCC_LocomotionComponent.md) | 选取模型专属 ABP、播放攻击 montage 与复制施法/命中特效 |
| [UCC_LocomotionLibrary](CPPClasses/UCC_LocomotionLibrary.md) | 模型、专属 ABP、六状态及三种攻击动画对应表 |
| [UCC_CharacterAnimInstance](CPPClasses/UCC_CharacterAnimInstance.md) | 基础移动/跳跃/落地/死亡状态及跑步倍率的动画蓝图父类 |
| [UCC_AnimationSetupLibrary](CPPClasses/UCC_AnimationSetupLibrary.md) | 编辑器生成可继续修改的六状态图与 DefaultSlot 的函数库 |
| [UCC_ExperienceFXSetupLibrary](CPPClasses/UCC_ExperienceFXSetupLibrary.md) | 编辑器构建独立单 Sprite Niagara 经验粒子系统 |




## 数值与键位配置新增类

| 类 | 用途 |
|---|---|
| [UCC_CombatBalance](./CPPClasses/UCC_CombatBalance.md) | 白板数值、路线倍率、精英继承和体型配置 |
| [UCC_ControlSettingsSave](./CPPClasses/UCC_ControlSettingsSave.md) | 本机六项战斗键位存档 |

[数值与技能面板配置](./数值与技能面板配置.md) 包含精确数值、全部技能说明与按键配置步骤。

## 正式 UI 接入

当前入口为 [壁画菜单资源配置](./壁画菜单资源配置.md)：已生成 `/Game/UI/MuralMenu/WBP_MuralMenu`，Tab 打开全 viewport 菜单与四个功能页。器物交互只把元素存入背包，背包选择后可以切换形态、购买进化；浏览不改变当前形态。初墨生命75、攻击11（基础11.25按成长规则四舍五入），随持有经验强化，不可直接进化；自然由绿色信物方块释放能力球，接触后入包并获得50经验。立绘读取实际形态/档位，称号在右上方。

深度定制继续使用 [UI 操作与 UMG 接入配置](./UI操作与UMG接入配置.md)，包含替换菜单、血条/HUD、22 项技能、进化预览、事件绑定、键位与存档。

## 技能美术页

| 类与配置文档 | 用途 |
|---|---|
| [UCC_SkillGalleryData](./CPPClasses/UCC_SkillGalleryData.md) | 18项图标/分类/简介及真实ExecutableSkill引用 |
| [UCC_SkillGalleryWidget](./CPPClasses/UCC_SkillGalleryWidget.md) | 图标/详情、真实四槽装备与形态组恢复，受流派校验 |

## 本轮新增拾取与召唤类

| 类 | 作用 |
|---|---|
| [ACC_FormPowerOrb](CPPClasses/ACC_FormPowerOrb.md) | 元素信物的彩色能力球，接触后入包并发50经验 |
| [ACC_CoinPickup](CPPClasses/ACC_CoinPickup.md) | 铜钱Character父类，原状态机、贴地导航巡逻、碰触铜币/经验结算及音频 |
| [ACC_SummonedSoldier](CPPClasses/ACC_SummonedSoldier.md) | 青铜召唤兵人实体、归属、跟随/攻击与生命周期 |

## 地区、传送与小地图新增类

| 类 | 作用 |
|---|---|
| [UCC_AdventureGameInstance](CPPClasses/UCC_AdventureGameInstance.md) | 地区解锁、跨图快照、v4存档、异步读取与加载画面 |
| [ACC_AdventureGameMode](CPPClasses/ACC_AdventureGameMode.md) | 三图统一当前角色/GAS/HUD，接管旧门户与出生 |
| [ACC_MapPortal](CPPClasses/ACC_MapPortal.md) | 碰触式地区入口与回城点 |
| [ACC_MinimapCapture](CPPClasses/ACC_MinimapCapture.md) | 本地无光照跟随相机，隐藏后停止捕获 |
| [UCC_WorldNavigationWidget](CPPClasses/UCC_WorldNavigationWidget.md) | M小地图、N三地区列表与加载遮罩 |

[地图传送与小地图配置](地图传送与小地图配置.md)包括关卡路径、出生、解锁、性能参数和前端UI接口。新增实现已通过UE5.6编辑器Development编译，运行测试由前端进行。

## 独立游戏主页新增类

| 类 | 作用 |
|---|---|
| [ACC_MainMenuGameMode](CPPClasses/ACC_MainMenuGameMode.md) | 首页独立框架，不生成游戏角色 |
| [ACC_MainMenuController](CPPClasses/ACC_MainMenuController.md) | 本地创建首页，管理UI焦点和鼠标 |
| [UCC_MainMenuWidget](CPPClasses/UCC_MainMenuWidget.md) | 开始游戏、宣传网站、退出游戏及加载画面 |

[游戏主页与背景音乐配置](游戏主页与背景音乐配置.md)包括Logo/壁画/网址、启动地图、存档交接与全局音乐。GameInstance新增音乐确保与音量接口；小地图改为真实窗口右下角锚定。本轮仅编译和后台资源配置，不运行游戏或测试。

## 固有疾跑

| 类 | 作用 |
|---|---|
| [UCC_SprintEffect](CPPClasses/UCC_SprintEffect.md) | 独立临时MoveSpeed乘算效果，默认1.5倍 |

输入由ACC_PlayerController绑定左右Shift，ACC_PlayerCharacter维护最长5秒／结束后5秒冷却和UI读取接口；不增加配装槽。小镇回城交互物已移除，返回使用N地区列表。[操作配置](固有疾跑与小镇返回配置.md)。
