# UCC_ProgressionComponent

父类：`UActorComponent`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_ProgressionComponent.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_ProgressionComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

玩家的经验钱包、原生／器物进化、商品购买和本地存档管理；已经由 PlayerState 创建。

## 配置方式

在 BP_ProgressionPlayerState 里选 Progression，不需要重复添加。Native Form 填 DA_Form_InitialInk，Artifact Forms 包含青铜／陶塑及自然 DA_Form_Native；神秘／墨化保留未开放，Upgrade Catalog 填允许购买的商品。所有形态先配完整三档四技能组。

## 参数与接口

- NativeForm：原生形态，始终可用；CurrentForm 为空表示选中原生，而不是没有配置。
- ArtifactForms：服务器允许交互解锁的目录。
- UpgradeCatalog：服务器允许购买的目录。
- Experience／NativeTier／CurrentTier／Forms／Upgrades：运行时只读数据，Tier 索引 0～2。
- Interact(Actor)、SwitchForm(Form)、Evolve()、Purchase(Upgrade)：服务器请求。Interact 只把元素之力加入 Forms 背包，不立即变身；背包明确调用 SwitchForm(Form) 才切换。SwitchForm 传空恢复初始墨水人。
- GetEvolutionCost 返回 -1 表示不可进化；CanEvolveCurrentForm 可查询形态／档位资格（不检查余额），初始墨水人固定不可进化；GetUpgradeLevel、GetSkillBonus 查询等级。
- AddExperience、RestoreExperienceFromBackend：只允许可信权威代码调用，后者仅用于读取完成后的经验恢复。击杀/经验包先生成粒子，ExperienceOrb 实际接触正确玩家后才调用 AddExperience；UI 不应自行预加余额。
- OnChanged：刷新状态；OnPurchaseFinished：成功或失败枚举。
- SaveProgress／LoadProgress：单机手动 SaveGame，默认 SpiritTales。
- RefreshAppearance：按当前阶段设置 Mesh/AnimClass；阶段 Mesh 已填时直接使用 Stage.MeshScale 作为最终组件相对缩放，不乘初始 Pawn 的缩放。阶段 Mesh 留空时回退缓存模型，缩放为缓存初始缩放 × Stage.MeshScale。因此同一形态的显示尺寸不再依赖玩家蓝图起始模型是否已归一化。

## 最小验证

初墨零经验时75生命／11攻击，持有经验每点+1%并四舍五入 → 与元素器物交互释放能力球、接触后信物入包 → 点击格子选择使用或直接进化 → 恢复初墨时按当前余额重新计算，器物档位独立保留；反复切换不能叠加永久数值或回满血。模型缩放仍读各档MeshScale。

## 当前限制

启动与切图保存沿用AdventureGameInstance，本地存档不等于后端自动同步。原生配置为空时可以回退原始能力，但不能购买原生进化。购买界面只发Purchase／Evolve／EvolveForm请求，不自行扣款。


## 2026-10-24 数值与技能配置更新

SkillCatalog 注册允许手动装备的技能。EquipSkill(Slot, Skill) / ResetSkills 由服务器校验目录、0–3 槽位、存活与冷却；同技能换槽会交换，不允许重复四槽。ManualSkills 属于 PlayerState，跨变身 / 进化保留。ApplyState 先解析 Stage.CombatBalance / StatProfileId 的生命与攻击，再叠加永久属性购买。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。

## 正式 UMG 查询与反馈接口

`GetActiveForm()` 返回实际有效路线，CurrentForm 为空时返回 NativeForm。
`GetStagePreview(Form, Tier, OutStage)` 返回是否可预览；Form 为空代表当前路线，预览自然路线明确传 DA_Form_Native（初始 NativeForm 现在指向墨水人）。Tier 是 0/1/2。结果生命和攻击已解析共享数值表，未叠加永久购买与临时增益；当前实战值应从 ASC 获取。

`OnSkillConfigurationFinished(ECC_SkillConfigurationResult)` 确认 EquipSkill / ResetSkills 的服务器结果：Success / InvalidState / OnCooldown / InvalidSlot / NotInCatalog / InvalidConfiguration。结果只用于操作反馈，最终四槽等 CurrentSlots 的复制通知到达后重绘。成长/形态/购买继续绑定 OnChanged 与 OnPurchaseFinished；后者不仅用于进化，反馈文字要按提交的动作区分。

[按步骤接入 UMG](../UI操作与UMG接入配置.md)。


## 初始墨水人和自然背包路线

初始 `NativeForm` 为 `DA_Form_InitialInk`（FormId=InitialInk），`CurrentForm` 为空时显示墨水人。自然 `DA_Form_Native` 的稳定 FormId 仍是 Native，列入 ArtifactForms 并在白色方块交互后存入 Forms。`bCanEvolve=false` 在服务器拒绝初始进化，GetEvolutionCost 返回 -1；恢复初始只会恢复第 0 档。

存档格式升级到 2，记录 NativeFormId。版本 1 的旧原生自然档位会迁入自然背包条目，已选器物、经验与购买保留；自然目录未就绪时拒绝迁移而不修改当前余额。详见 [墨水人与自然之力配置](../墨水人与自然之力配置.md)。

开发者专用隐藏 0/9 快捷键见 [开发隐藏按键](../开发隐藏按键.md)，不会显示于玩家界面。

## 2026-10-24真实技能与铜币更新

新增CopperCoins/AddCopperCoins、HasUnlockedForm/IsSkillUnlocked；EquipSkill和GAS施放校验RequiredFormId。存档v3兼容v1/v2，读取后清除未解锁配装；0/9均开启完整测试模式并补满铜币、解锁物品各999。

详见[真实技能与铜币配置](../真实技能与铜币配置.md)。

## 元素球领取更新（2026-10-24）

Interact现在只请求释放能力球；private CollectArtifactPower由球接触后调用，原子更新力量条目和经验，已拥有力量保留档位。UI在OnChanged时确认入包，不将释放请求Success视作已领取。详见[元素信物与能力球](../元素信物与能力球配置.md)。

## 跨地图进度交接

CreateSnapshot/RestoreSnapshot用于原生跨图交接；SaveProgress/LoadProgress复用同一序列化流程，当前格式v4兼容v1～v3。新头像初始化后由AdventureGameInstance恢复一次，切图前和真实抵达后保存。默认槽手动存读会同步会话快照；自定义测试槽不改变地区账本。0/9后暂停自动写档，手动存读恢复。见[地图传送配置](../地图传送与小地图配置.md)。

## 格子背包与初墨成长接口

GetOwnedFormTier(Form)返回已拥有档位（未拥有为-1）；GetFormEvolutionCost(Form)返回下一档费用；EvolveForm(Form)支持直接进化未使用信物，Evolve()仍作用于当前形态。未使用信物升级不会切换模型或技能；若当前使用初墨，扣款会重算其属性。

InitialInkBonusPerExperience默认0.01。GetExperienceGrowthMultiplier(Form)仅为InitialInk返回1+余额×系数，其他形态为1。ApplyState先解析共享数值、叠加永久属性，再对初墨的生命/攻击应用倍率并四舍五入；GetStagePreview不含永久购买加成，但包含余额成长。经验增发、信物奖励、后端恢复、交易扣款、变身、重生和读档均通过相应入口重算，保留血量比例。完整公式与边界见[格子背包与初墨成长](../格子背包与初墨成长配置.md)。
