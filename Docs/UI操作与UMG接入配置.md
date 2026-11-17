# UI 操作与 UMG 接入配置

核对版本：2026-10-24，Unreal Engine 5.6。工程：`CrashCourse.uproject`。

当前已生成 `/Game/UI/MuralMenu/WBP_MuralMenu`（父类 `CC_MuralMenuWidget`），并配置到 `BP_ProgressionController.ProgressionMenuClass`。Tab 打开全 viewport 壁画主页，背包、技能、交易、设置四个独立功能页已经接入同一玩法接口。先按 [壁画菜单资源配置](./壁画菜单资源配置.md) 调整图片和形态绑定；本轮编译、5 项核心自动测试、184 项菜单实测检查及改键/粒子命中检查均通过。

本文保留蓝图深度定制步骤：下文 `WBP_GameMenu`、`WBP_GameHUD`、各 `WBP_*Page` 和页面刷新函数是另建 Designer 界面的建议命名，不代表又生成了这些资源。现有 `WBP_MuralMenu` 由 C++ 构建布局；仅替换图片无需重做页面。玩法、购买和装备继续调用 C++ 接口，UI 展示状态、收集输入和显示结果。

## 1. 先跑通现有操作

打开 `/Game/HPWH/Maps/L_FlatTestArena`，确认 World Settings 的 GameMode Override 为 `BP_ProgressionGameMode`，再 Play：

| 操作 | 当前默认行为 |
|---|---|
| WASD / 鼠标 / 空格 | 模板移动、镜头、跳跃 |
| J | 独立近战普攻，不占四技能槽 |
| 鼠标左键 | 战技面板展开时使用选中技能／道具；收起时普攻 |
| 鼠标右键 / 滚轮 | 展开战技／循环选择技能1～4及道具5 |
| 1 / 2 / 3 / 4 | 施放当前四槽技能 |
| E | 面向附近器物或经验包交互 |
| 绿色自然信物 | E后释放能力球，接触后力量入包并+50经验；背包“切换形态”后使用 |
| 绿色方块 | 青铜之力存入背包，不当场变身 |
| 黄色方块 | 陶塑之力存入背包，不当场变身 |
| 黑色方块 | 一次放出 50 颗经验粒子，接触后逐颗入账 |
| 击杀敌人 | 掉落 10 颗经验粒子，全部领取共 10 经验；尸体 10 秒后清理 |
| Tab | 打开壁画主页，进入背包、技能、交易、设置；背包可切换/进化 |
| Tab / 退出界面 | 关闭菜单，返回游戏 |

经验是所有路线共用的货币。初级进入中级花费 **100**，中级进入高级花费 **1000**，累计 **1100**。切换路线保留各自档位；手动装备的四技能跨变身、进化保留，点击“恢复当前形态默认技能”才回到该档默认组。

初始是 `DA_Form_InitialInk` 初墨，白板 100 生命 / 15 攻击的 75%，即 **75 生命 / 11.25 攻击**；`CanEvolve=false`，不可直接买中级或高级。自然、青铜、陶塑都需要对应方块交互后存入 `Forms`，背包选择只浏览，“切换形态”按钮才实际使用。背包可查看所选力量当前/下一档属性和费用；只有切到该力量后才能进化，选择别的条目不会自动变身。神秘和墨化保留但不开放。

右侧立绘与右上竖版称号读取**当前实际形态与档位**的 `Stage.MenuPortrait / MenuTitle / MenuDisplayName`，不跟随下拉框预览条目换图。菜单打开时玩家操作被拦截，**世界和怪物仍继续运行**，当前没有自动暂停。

## 2. 当前入口与深度定制资源组织

已有资源为 `/Game/UI/MuralMenu/WBP_MuralMenu`、`DA_MenuArt`、`BP_MuralHUD` 与 `Textures/Menu、Portraits、Titles`。`BP_MuralHUD.ShowDebugPanel=false`，世界血条保持原接法。

以下是**后续自行制作 Designer 页面**的建议结构，可按项目需要采用；不要为使用当前壁画菜单重复创建整套资源：

```text
/Game/UI/
  Core/WBP_GameMenu           菜单总容器，父类 UserWidget
  Core/WBP_GameHUD            常驻屏幕 HUD，父类 UserWidget
  Core/BP_GameHUD             HUD 蓝图，父类 CC_ProgressionHUD
  Pages/WBP_InventoryPage     背包与经验
  Pages/WBP_EvolutionPage     路线、三档和进化预览
  Pages/WBP_SkillPage         全技能目录、详情、四槽装备
  Pages/WBP_UpgradePage       永久属性与技能等级商品
  Pages/WBP_SettingsPage      键位、本地存读档
  Entries/WBP_SkillEntry      单个技能条目
  Entries/WBP_SkillSlot       单个技能槽
  Entries/WBP_UpgradeEntry    单个商品条目
  World/WBP_WorldHealth      世界血条，父类 CC_AttributeWidget
  Art/                       图标、字体、背景、按钮样式
```

`WBP_GameMenu` 用 `WidgetSwitcher` 切页。背包、进化、技能、属性、设置按钮只调用自己的 `SetActiveWidgetIndex`；“退出界面”调用控制器关闭接口，不是退出游戏。

现有玩法资产继续在这些位置配置：

| 内容 | 资源 / 配置位置 |
|---|---|
| 玩家与输入入口 | `/Game/HPWH/Blueprints/BP_ProgressionController` |
| 钱包、路线与目录 | `BP_ProgressionPlayerState` 的继承组件 `Progression` |
| 形态三档 | `/Game/HPWH/Forms/DA_Form_*` |
| 共用数值 | `/Game/HPWH/Balance/DA_CombatBalance` |
| 默认四技能组 | `/Game/HPWH/Loadouts/DA_Loadout_*` |
| 玩家可选技能 | `/Game/HPWH/Skills/DA_Skill_01` ～ `DA_Skill_22` |
| 独立普攻 | `/Game/HPWH/Skills/DA_BasicAttack` |
| 永久商品 | `/Game/HPWH/Upgrades/DA_Upgrade_*` |
| 普通怪 / 精英技能组 | `DA_EnemyLoadout` / `DA_EliteLoadout` |

## 3. 把正式菜单接到 Tab

当前已把 `BP_ProgressionController → Progression Menu Class` 配为 `WBP_MuralMenu`，其 `Art Data` 为 `DA_MenuArt`。Tab 默认打开 Overview 壁画主页，导航进入四个全屏功能页。改背景/按钮用 `DA_MenuArt`，改某档立绘/右上称号用各 Form 的 Stage；无需重新接 Tab。

若要完全另建 Designer 菜单，保留下列步骤作为深度定制流程：

1. 创建 Widget Blueprint `WBP_GameMenu`，父类选 **UserWidget**。不要选 `CC_ProgressionMenu`：后者初始化时构建原生文字控件树，适合作为回退原型。
2. 在 `WBP_GameMenu` Class Defaults 开启 `Is Focusable`。放置导航按钮、`WidgetSwitcher`、反馈文字和关闭按钮。
3. 打开 `BP_ProgressionController → Class Defaults → Crash | Menu → Progression Menu Class`，选择 `WBP_GameMenu`。
4. 保留现有 Input Mapping Contexts、移动和战斗 InputAction 引用。Tab 的开关已经由 C++ 绑定，不在蓝图再绑一份。
5. 关闭按钮：`Get Owning Player → Cast To CC_PlayerController → Close Progression Menu`。
6. 在 `WBP_GameMenu` 覆写 `On Preview Key Down`：读取 `In Key Event → Get Key`；若正在用 `InputKeySelector` 录入按键，交给其处理；否则遇到 Tab 调用 `Close Progression Menu` 并返回 `Handled`，其他返回 `Unhandled`。

控制器负责 `CreateWidget`、以 ZOrder 100 添加菜单、鼠标显示、UI 焦点和移动/视角输入锁；关闭时恢复 Game Only 输入。不要在菜单里重复 `CreateWidget`、重复锁输入或只调用 `Remove From Parent`，否则控制器无法及时执行完整关闭流程。

`ProgressionMenuClass` 留空会回退原有文字菜单，可用来排查是美术蓝图问题还是玩法问题。`Get Progression Menu Widget` 仅用于读取当前实例；关闭后该引用为空，重新打开会创建新实例。

### 控制器可用接口

| 接口 | 蓝图用途 |
|---|---|
| `OpenProgressionMenu()` / `CloseProgressionMenu()` | 按钮、外部交互入口明确打开或关闭 |
| `ToggleProgressionMenu()` | 已有 Tab 入口；重复调用会反转状态 |
| `IsProgressionMenuOpen()` | HUD、提示等查询菜单状态 |
| `GetProgressionMenuWidget()` | 获取当前自定义或回退菜单实例 |
| `OnMenuVisibilityChanged(bool bOpen)` | 常驻 HUD 隐藏交互提示、显示背景遮罩等 |
| `GetCombatKey(int32 Action)` | 技能角标、普攻和交互键文字 |
| `SetCombatKey(int32 Action, FKey Key)` | 应用一个按键，返回是否接受 |
| `ResetCombatKeys()` / `OnCombatKeysChanged()` | 恢复默认 / 通知全部键位文字刷新 |
| `RequestInteraction(AActor* Target)` | 统一器物和经验包交互入口 |

## 4. 初始化引用、事件与销毁

建议菜单和屏幕 HUD 各保存这四个引用：`ControllerRef`（CC_PlayerController）、`ProgressionRef`（CC_ProgressionComponent）、`CharacterRef`（CC_BaseCharacter）、`SkillsRef`（CC_SkillComponent）；属性显示另保存 `ASCRef`。

### 引用链

```text
Get Owning Player
  → Cast To CC_PlayerController → ControllerRef
  → Get Player State → Cast To CC_PlayerState
    → Get Progression → ProgressionRef

Get Owning Player → Get Pawn → Cast To CC_BaseCharacter
  → CharacterRef
  → Get Skills → SkillsRef

CharacterRef → Get Ability System Component（AbilitySystemBlueprintLibrary）
  → ASCRef
```

玩家 ASC 实际由 PlayerState 持有，通过角色的 `IAbilitySystemInterface` 仍能获取。不要 Get All Actors Of Class 后拿第一个 PlayerState/怪物的 ASC；网络环境下会读错玩家。

建议新建蓝图函数 `ResolveReferences`、`BindDataEvents`、`RefreshAll`、`ReleaseBindings`。这些名字由 UI 蓝图自己实现，并非额外 C++ API。

1. `Event Construct → ResolveReferences`，依次 `Is Valid`。PlayerState、Pawn 或 ASC 尚未准备好时显示“数据初始化中”，用短间隔重试或等对应事件，不立即把空数据当成零经验/空目录。
2. `ControllerRef → Bind Event to OnPossessedPawnChanged`，角色更换时先解绑旧 `CharacterRef/SkillsRef`，再获取新引用。
3. `CharacterRef → Bind Event to OnASCInitialized`。收到 ASC 初始化时重绑属性监听并主动读取初始值；若进入界面时 ASC 已有效，也立即执行一次初始化。
4. 对 `ProgressionRef` 绑定 `OnChanged`、`OnPurchaseFinished`、`OnSkillConfigurationFinished`；对 `SkillsRef` 绑定 `OnLoadoutChanged`；对控制器绑定 `OnCombatKeysChanged`。常驻 HUD 按需要绑定 `OnMenuVisibilityChanged`。
5. 绑定后立即调用 `RefreshAll`。事件只覆盖以后的变化，不能代替首帧读取。
6. 页面子控件由总容器传递相同引用，避免每个技能条目再创建一套全局监听。

`Event Destruct → ReleaseBindings`：使用 `Unbind Event from ...` 解除**本控件所绑定的事件**；对保存的每个属性监听代理调用 `EndTask`；清理重试与冷却定时器；清空引用。不要 `Unbind All Events`，它可能同时清掉其他 HUD 的订阅。

若使用重试定时器，成功绑定后立即清除，并设置有限重试/明确报错，避免关闭界面后继续回调。防止重复 `Construct` 绑定的做法是：每次重绑前执行 `ReleaseBindings`，记录当前绑定源，重新读取后再订阅。

### 刷新来源

| 通知 / 查询 | 应刷新内容 |
|---|---|
| `Progression.OnChanged` | 经验、路线、档位、购买等级、商品按钮、技能等级说明 |
| `Progression.OnPurchaseFinished(Result)` | 本次进化 / 商品 / 切换请求的结果提示 |
| `Progression.OnSkillConfigurationFinished(Result)` | 装备或恢复默认请求的服务器结果提示 |
| `Skills.OnLoadoutChanged` | 从 `CurrentSlots` 重新画四槽名称、等级和详情 |
| `Controller.OnCombatKeysChanged` | 所有技能、普攻和交互按键角标 |
| GAS 属性变化 | 血量、法力、攻击、防御、护盾、移速；攻击变化同时刷新技能强度 |
| `Skills.GetCooldownRemaining(Slot)` | 技能冷却剩余秒数；可用 0.05～0.1 秒定时更新可见 HUD |

购买和装备的 Client RPC 结果与属性/数组复制可能先后到达。收到 Success 可显示“请求成功”，**数据内容仍以复制到本地的字段及相应事件为准**，不要自己把档位加一、经验扣掉或重排四槽。HUD 也必须对 `CurrentSlots.Num != 4` 和属性暂未就绪作保护。

## 5. 常驻 HUD 与血条

### 屏幕 HUD 创建一次

已有 `BP_MuralHUD` 只关闭旧调试文字面板，不自动创建新的屏幕 HUD。头顶血条继续使用现有组件。以下步骤适用于继续增加常驻 HUD：

1. 创建 `BP_GameHUD`，父类 `CC_ProgressionHUD`，Class Defaults 中将 `Show Debug Panel`（`bShowDebugPanel`）关闭，避免文字原型和正式 HUD 重叠。
2. `BP_GameHUD → Event BeginPlay → Get Owning Player Controller → Is Local Controller → Create Widget(WBP_GameHUD, Owning Player=该控制器) → Add To Viewport(ZOrder=0)`，保存实例。创建前判断实例是否有效，避免重复添加。
3. 将实际 GameMode 的 `HUD Class` 改为 `BP_GameHUD`。Default Pawn / Player Controller / Player State 保持现有正确派生类。
4. 角色重生或换 Pawn 只重绑数据，不重复往视口堆 HUD。HUD 结束时移除实例并释放监听。

### 屏幕生命、法力与属性

屏幕 `WBP_GameHUD` 直接 `AddToViewport` 不会获得世界血条组件的自动订阅，即使其父类选 `CC_AttributeWidget` 也一样。推荐在屏幕 HUD 内显式监听：

```text
ASC 有效
  → Listen For Attribute Change(ASCRef, Attribute=CC_AttributeSet.Health)
    → 保存 AsyncTask 输出为 HealthTask
    → OnAttributeChanged → RefreshHealth
  → Listen For Attribute Change(ASCRef, Attribute=CC_AttributeSet.MaxHealth)
    → 保存 AsyncTask 输出为 MaxHealthTask
    → OnAttributeChanged → RefreshHealth
  → 立即调用 RefreshHealth
```

`RefreshHealth` 内调用 `Get Float Attribute From Ability System Component`，分别选择 Health、MaxHealth，检查 `Successfully Found Attribute`；若最大值大于 0，`Clamp(Current / Max, 0, 1) → ProgressBar.SetPercent`，否则百分比设 0。`SetText` 显示“当前 / 最大”。初始值需要主动读取，因为 `Listen For Attribute Change` 不自动推送初值。

法力同理监听 Mana / MaxMana；攻击、防御、护盾和移速分别监听 Attack / Defense / Shield / MoveSpeed。读取当前生效数值使用 `Get Float Attribute ...`，不要用 Base 版本代替，否则会漏掉临时强化。攻击显示保留两位小数，避免 37.5 被取整；伤害简介同样保留小数。

销毁或更换 ASC 时对每个保存的代理调用 `EndTask`，不能只清空变量。不要每帧重新创建监听代理。

### 角色头顶血条

现有 `HealthDisplay.Space=Screen`：它是附着在角色上的 `CC_WidgetComponent`，以屏幕空间显示头顶血条，和 `AddToViewport` 的全屏 HUD 是不同入口。保留现有 Screen 空间即可；如美术确需场景中的三维血条，再改为 World 并调整朝向与尺寸。

1. 新建 `WBP_WorldHealth`，父类 **CC_AttributeWidget**。设计器放 `ProgressBar` 和 `TextBlock`，勾选 Is Variable。
2. Class Defaults 配置 `Attribute=Health`、`MaxAttribute=MaxHealth`。
3. 实现 `Event On Attribute Change(NewValue, NewMaxValue, OldValue)`：用同样的零值保护设置 Percent 和文字；也可直接读取已计算的 `ValuePercent`。
4. 在敌人/玩家蓝图中选已有继承组件 **HealthDisplay（CC_WidgetComponent）**，Widget Class 指向 `WBP_WorldHealth`，设置显示空间、位置和 Draw Size。`Attribute Map` 默认 Health → MaxHealth。
5. 该组件已处理 ASC 初始化、当前值和最大值监听、首帧刷新及结束解绑，不再手工为它加第二份属性监听。
6. 运行时更改 WidgetClass 或 AttributeMap 后调用 `HealthDisplay.RefreshBindings()`。

世界血条显示精英的实际最大生命即可。精英 1.2 倍体型由怪物数值档案应用，不在 Widget 中再次放大角色；可按需要调整血条挂点高度。

## 6. 背包：经验钱包与已存元素

当前壁画背包已经显示初墨及 `Progression.Forms` 中已获得的元素之力。器物 `Interact` 只添加解锁记录，不改当前使用形态；重复获取不重复添加。点击列表只预览，点击“切换形态”才发 `SwitchForm`。背包“进化”按钮展示所选力量当前/下档属性、费用：所选力量不是当前形态时先提示切换，不能误升当前另一条路线。初墨不可直接进化。

自制 `WBP_InventoryPage` 的元素列表按 `NativeForm` 初墨 + 已解锁 `Forms` 构造，每行保存 Form 资产引用。切回初墨传 `SwitchForm(None)`；使用自然、青铜、陶塑均传对应 Form。

经验文字：`ProgressionRef.Experience → ToText(Integer) → Text_Experience.SetText`。初始化刷新一次，之后由 `OnChanged` 更新。粒子飞出是待领取表现，玩家实际吸附接触后服务器才增加钱包；不要由 UI 预加 10/50。当前尚未接普通物品格或道具库存接口，已存元素不是普通物品数量。

## 7. 进化页：解锁、三档预览与购买

### 路线列表与数据

当前壁画菜单将进化主入口放在**背包**，交易页继续购买永久属性等商品。以下三档与服务器接线适用于背包进化区，也可用于后续单独制作的 `WBP_EvolutionPage`。

`Progression.NativeForm` 现在是初墨，始终可选但不可进化。自然、青铜、陶塑均在 `Progression.ArtifactForms`，过滤无效及 `bAvailable=false` 后查 `Forms` 解锁记录。目录存在不代表已获得，不能仅凭资产名含 Native 就认作免费自然路线。

| 显示路线 | Form 资产 | 稳定 FormId | 初 → 中 → 高模型 |
|---|---|---|---|
| 初墨 | `DA_Form_InitialInk` | InitialInk | 初始墨水人，无直接三档进化 |
| 自然之力 | `DA_Form_Native` | Native | 竹路人 → 竹节人 → 竹拳击手 |
| 青铜之力 | `DA_Form_Artifact_01` | Artifact_01 | 铜机械小兵 → 铜械战士 → 铜械将军 |
| 陶塑之力 | `DA_Form_Artifact_02` | Artifact_02 | 陶童子 → 陶土行者 → 陶土僧人 |

当前路线：`Get Active Form()`；当前档：`CurrentTier`。初墨的 `NativeTier` 固定 0；自然、青铜、陶塑的已购档位均取 `Forms` 对应项的 `Tier`。索引统一 **0=初级、1=中级、2=高级**。

切回初墨：`Switch Form(Form=None)`。切自然、青铜、陶塑：`Switch Form(该行的 Form)`。只接受已解锁、已开放、配置完整的形态。下拉框选择只改变预览，不能在 OnSelectionChanged 直接调用 SwitchForm。

### 三档预览接线

对页面所选 Form 循环 Tier 0～2：

```text
Get Stage Preview(SelectedForm, Tier, OutStage)
  → Return Value=true
    → Break CC_FormStage
      → MaxHealth / Attack / MaxMana / Defense / MoveSpeed / Cost / Mesh / Loadout
    → 更新对应档位卡片
  → false：显示“配置不可用”，关闭购买按钮
```

此接口会解析共享 `CombatBalance / StatProfileId`，不要直接读取 Stage 的备用 MaxHealth / Attack 来画卡片。**GetStagePreview 的 Form=None 表示当前路线**；要预览自然，明确传 `DA_Form_Native` 的资产引用，它现在属于 ArtifactForms。`SwitchForm(None)` 则表示切回 NativeForm 初墨。

预览值是基础形态值，**不含永久购买和临时 Buff**。界面标注“基础数值”；当前实际值从 ASC 读取，单独显示“当前生效”。如需展示“进化后含永久购买”，按 `Upgrades` 的等级和商品 `BonusPerLevel` 计算展示增量，但不要把 UI 算出的数值回写 ASC。

| 路线 | 初级 HP / ATK | 中级 HP / ATK | 高级 HP / ATK |
|---|---:|---:|---:|
| 自然 | 100 / 15 | 250 / 37.5 | 450 / 67.5 |
| 青铜 | 140 / 21 | 300 / 45 | 500 / 75 |
| 陶塑 | 160 / 15 | 350 / 37.5 | 700 / 67.5 |

白板生命 100、攻击 15。倍率对应自然 100%/250%/450%，青铜 140%/300%/500%；陶塑生命 160%/350%/700%，攻击 100%/250%/450%。上述表为当前配置快照，正式 UI 应读数据接口，不把数值写死在控件。

### 进化按钮与反馈

`Get Evolution Cost()` 返回当前使用路线的下一档费用，初→中为 100，中→高为 1000；初墨不可进化，返回 -1；其他 -1 情况包括封顶或配置不可用。进化按钮在 Cost≥0 且 Experience≥Cost 时可点，点击只调用 `Evolve()`。

`Evolve()` 没有 Form 参数，作用于**当前使用形态**。浏览别的路线时，显示“先切换到该路线”，待切换结果及状态到达后才开放该路线购买，避免在陶塑预览页误升级当前青铜。

`OnPurchaseFinished` 同时用于 Evolve、Purchase、SwitchForm 和器物解锁流程。UI 保存本次 `PendingRequest`（如 Evolve / Switch / Upgrade），请求期间禁用重复提交，再按类型写“进化成功 / 已切换 / 属性购买成功”。不能把所有 Success 一律显示“进化成功”。

| ECC_PurchaseResult | 建议提示 |
|---|---|
| Success | 按本次操作显示成功，数据等待状态刷新 |
| InsufficientExperience | 经验不足 |
| MaxLevel | 已达最高档位 / 该商品已满级 |
| InvalidConfiguration | 配置不可用，请检查对应目录和资产 |
| InvalidState | 当前角色状态不可操作 |
| NotUnlocked | 该路线未解锁或未开放 |

## 8. 技能面板：真实目录、详情与四槽

### 技能目录和数值简介

读取 `Progression.SkillCatalog`，当前为39项（22旧模板+17新定义）；每个条目保存 **SkillDefinition 资产引用**，显示 `DisplayName` 与 `Description`。排序可以变化，点击时直接用该引用，不通过中文名、序号或字符串拼路径查找技能。

技能详情：先从 ASC 读取当前 Attack，再调用所选技能 `Get Numeric Description(Attack, Level)`。该文本包含用途、强度、冷却、消耗、前摇及行为相关范围等参数。需要图形排版时可用同一资产的只读字段拆开显示。

| 编号 / 资产后缀 | 模板 | 一级强度 | 冷却 / 法力 |
|---|---|---|---|
| 01 | 粒子远程 | 攻击 × 75% | 2s / 0 |
| 02 | 连击 | 3 击，每击攻击 × 60% | 3s / 10 |
| 03 | 横扫 | 攻击 × 120% | 3s / 10 |
| 04 | 突进 | 攻击 × 80% | 4s / 10 |
| 05 | 近战打击 | 攻击 × 100% | 1s / 5 |
| 06 | 散射 | 3 发，每发攻击 × 45% | 4s / 12 |
| 07 | 追踪弹 | 攻击 × 65%，无锁定目标时沿瞄准方向发射 | 3s / 10 |
| 08 | 穿透弹 | 最多 3 目标，每目标攻击 × 80% | 4s / 12 |
| 09 | 范围爆发 | 攻击 × 150% | 5s / 15 |
| 10 | 落点打击 | 攻击 × 180% | 6s / 18 |
| 11 | 持续区域 | 3s，每 0.5s 攻击 × 25% | 7s / 20 |
| 12 | 直线打击 | 攻击 × 125% | 4s / 12 |
| 13 | 自身治疗 | +30 生命，不超过最大值 | 5s / 15 |
| 14 | 范围治疗 | 每友方 +20 生命 | 7s / 20 |
| 15 | 护盾 | 40 护盾，5s | 8s / 15 |
| 16 | 防御强化 | +25 防御，5s | 8s / 15 |
| 17 | 攻击强化 | +7.5 攻击，5s | 8s / 15 |
| 18 | 移速强化 | +150 移速，4s | 8s / 10 |
| 19 | 减速 | 敌方移速 -40%，3s | 6s / 12 |
| 20 | 击退 | 攻击 × 100%，击退强度 400 | 5s / 12 |
| 21 | 交替拳击，保留原 Primary 机制 | 攻击 × 100%，左右拳交替 | 0.6s / 0 |
| 22 | 短时疾跑，保留原 Tertiary 机制 | +500 移速，1s | 5s / 5 |

01～22 只是当前模板编号，实际稳定 ID 为 `Skill_01`～`Skill_22`。原始 1～4 是槽位输入，并不等于固定装备 Skill_01～04；当前自然初始默认四槽为 **05 / 02 / 04 / 13**。原 Secondary 只有调试流程，没有作为一个完成技能计入目录。

`PowerMode=AttackMultiplier` 时，Power=0.75 表示当前攻击的 75%，不是 75 点；`Flat` 表示固定点数。使用 `Calculate Power(Attack, Level)` 计算强度，不再乘一次攻击。粒子 01 当前每级增量为 0，白板 15 攻击对应 **11.25 减伤前伤害**；实际命中还经过防御及护盾结算。不要把 11.25 写死为所有形态的粒子伤害。

目前没有 `Icon` 字段。先由 UI 自己维护 `SkillId → Soft Texture2D` 映射/展示表，找不到时用占位图，异步加载后更新；后续可以再统一增加展示资源字段。名称可以修改，稳定 SkillId 不改。

### 四槽的实际来源与等级

四槽从 `SkillsRef.CurrentSlots` 读取，循环索引 **0～3**，先 `Is Valid Index`，再 `Break CC_SkillSlot → Skill / Level`。`CurrentLoadout` 是形态默认模板引用，**不能据此显示手动装备后的四槽**。

已装备槽的实际等级：

```text
ActualLevel = CurrentSlots[Slot].Level
              + Progression.GetSkillBonus(CurrentSlots[Slot].Skill.SkillId)
```

未装备目录技能按 `1 + GetSkillBonus(SkillId)` 做“装备后预览”；手动装备会按基础 Level=1 加商品技能等级。形态默认组自身可能有更高基础等级，不要把所有槽都显示为 Lv1。

槽位角标取 `Controller.GetCombatKey(Slot) → Get Key Display Name → SetText`；冷却取 `Skills.GetCooldownRemaining(Slot)`。`GetCooldownRemaining(4)` 是**独立普攻**的内部通道，不能画成第五技能槽；`ActivateSlot` 仅接受 0～3。

### 装备与恢复默认的事件流程

```text
选择技能条目 → 保存 SelectedSkill（资产引用）
点击四槽之一 → 保存 SelectedSlot（0～3）
点击装备 → 显示“正在请求装备” → Progression.EquipSkill(SelectedSlot, SelectedSkill)
  → OnSkillConfigurationFinished(Result)：显示本次装备结果，解除提交锁
  → OnLoadoutChanged：重读 CurrentSlots，再刷新四槽和已装备标识
点击恢复默认 → Progression.ResetSkills()
  → 同样等待结果事件和当前四槽更新
```

服务器拒绝非法槽、目录外技能、无效配置、角色状态异常及任何技能/普攻仍在冷却时换装。`CanConfigureSkills()` 可用于按钮预判，但最后由服务器校验。把已装备技能放到另一槽会交换两个槽，不会重复装备同一个 SkillId。

| ECC_SkillConfigurationResult | 建议提示 |
|---|---|
| Success | 技能配置成功；槽位内容以 CurrentSlots 刷新为准 |
| InvalidState | 当前角色状态不可配置 |
| OnCooldown | 技能或普攻冷却期间不能换装 |
| InvalidSlot | 无效槽位，只支持四槽 |
| NotInCatalog | 技能未注册到可装备目录 |
| InvalidConfiguration | 技能 ID、四槽或资产配置无效 |

新增技能时：复制或创建 `CC_SkillDefinition` Data Asset → 填唯一 SkillId、显示名、简介和参数 → 加入 `BP_ProgressionPlayerState.Progression.SkillCatalog` → 配图标映射 → 面板验证。只把资产放进 Content 目录不会自动允许装备。

## 9. 属性与技能等级商品

`WBP_UpgradePage` 从 `Progression.UpgradeCatalog` 生成条目，每个条目保存对应 `UpgradeDefinition` 引用。

```text
Level = Get Upgrade Level(Upgrade)
Prices.IsValidIndex(Level) ? Prices[Level] : 已满级
当前总加成 = Level × BonusPerLevel
购买按钮 → Progression.Purchase(Upgrade)
结果提示 → OnPurchaseFinished
钱包/等级/实际属性刷新 → OnChanged 和 GAS 属性变化
```

Prices 的索引是**当前已购等级**，不是 Level+1。商品有几项 Prices 就最多买几级；空数组不能购买。用 `Type` 区分 MaxHealth / MaxMana / Attack / Defense / MoveSpeed / SkillLevel。

属性商品每次加 `BonusPerLevel`；SkillLevel 商品通过其 `SkillId` 每次加一级，不能把 BonusPerLevel 当技能等级增量。当前 01 粒子技能的每级强度增量为 0，购买等级仍能显示升级但不会自动提高其 75% 系数；要有成长需在技能资产配置 `PowerPerLevel`。

`ATTACK` 商品增加的是实际攻击点数，不是百分比。商品必须加入 UpgradeCatalog。UI 不直接调用 AddExperience、不直接设置 Experience，不向 ASC Set 属性；只提交 Purchase，由服务器统一扣费、加级和重算。

## 10. 交互提示与按钮

正式 HUD 可在本地用定时射线更新目标提示。采用与控制器一致的视点和 Visibility 通道，忽略自身 Pawn：`Get Player View Point → Line Trace By Channel`，当前射线距离 1500；判定器物组件或 `CC_ExperiencePack`，再调用目标的 `CanInteract` 作为本地提示条件。

| 目标 | 判断与文字 |
|---|---|
| 挂有 CC_ArtifactComponent 的 Actor | `Get Component By Class → Form / bEnabled / CanInteract(Character)`，再过滤 Form.bAvailable；“[交互键] 获取 ××之力并存入背包” |
| CC_ExperiencePack | `CanInteract(PlayerCharacter)`，结合 bConsumed；“[交互键] 领取 50 经验”，数量读取 ExperienceAmount |
| 无效、超距或被遮挡 | 隐藏提示 |

提示按键从 `Controller.GetCombatKey(5)` 读取。提示点击按钮统一调用 `Controller.RequestInteraction(Target)`；原 E/改键路径已经调用此接口，不在 UI 再绑定 E 触发第二次。

可挂组件的场景 Actor 不要求统一父蓝图；物体需可被 Visibility 射线击中，并配置 Form。器物默认服务器交互距离为 250，经验包同样默认 250；1500 是相机找物射线，不能据此认为角色可以远距离拾取。客户端提示只作展示，服务器仍检查真实距离、视线、存活和目录。

菜单打开时清除/隐藏场景交互提示；控制器也会拒绝菜单期间的 RequestInteraction。当前没有可供 UI 订阅的“当前交互目标变化”专用 C++ 事件，目标检测由 HUD 的本地展示逻辑实现。

## 11. 键位设置页

`InputKeySelector` 的 `Allow Gamepad Keys=false`、`Allow Modifier Keys=false`。当前设置页由 `UCC_OptionsWidget` 提供六个独立改键框；自制UI无论使用列表还是下拉框，动作索引必须遵循：

| Action 索引 | 含义 | 默认 |
|---:|---|---|
| 0 | 四技能槽 1 | 1 |
| 1 | 四技能槽 2 | 2 |
| 2 | 四技能槽 3 | 3 |
| 3 | 四技能槽 4 | 4 |
| 4 | 独立普攻 | J |
| 5 | 器物/经验包交互 | E |

初始化或刷新对应行：`GetCombatKey(Action) → Make InputChord → Set Selected Key`。用本地 `bUpdatingKeySelector` 防止程序刷新触发重新保存。

`OnKeySelected(SelectedKey)`：`Break InputChord`，有 Ctrl/Alt/Shift/Cmd 时拒绝；否则调用 `SetCombatKey(Action, Key)`。返回 true 显示“已应用”；false 显示“无效、已占用或保留按键”，并用 GetCombatKey 回填原值。不要把输入框显示的新键当作实际生效结果。

鼠标左键在战技面板收起时普攻，展开时使用选中技能或道具。WASD、空格、Tab、Shift、M、N、F1～F12、鼠标左键、鼠标右键、Enter、滚轮以及模板非战斗映射占用键被保留；重复战斗键、修饰键、手柄键和轴输入不接受。移动、跳跃、镜头仍由现有 Enhanced Input 管理，设置页只改这六项。

`Reset Combat Keys` 恢复 1/2/3/4/J/E。成功更改/重置后，控制器广播 `OnCombatKeysChanged` 并自动保存本机键位；所有槽位和交互角标都要重读。不要为改键直接修改共享 IMC 资产；当前运行时使用映射副本。

## 12. 存读档与后端边界

| 数据 | 当前接口与位置 | UI 接入规则 |
|---|---|---|
| 本机六项键位 | 控制器自动保存，`Saved/SaveGames/SpiritTalesControls.sav` | 设置成功即生效并尝试保存；不跟角色经验混存 |
| 单机角色完整进度 | `Progression.SaveProgress("SpiritTales")` / `LoadProgress("SpiritTales")`，`SpiritTales.sav` | 手动按钮调用，检查 bool 结果；读取要求角色准备好且不在技能冷却中 |
| 角色进度包含 | 经验、原生档位、各路线档位、当前路线、商品等级、手动四技能 ID | 不是关卡敌人位置/物理/背包物品全存档 |
| 后端角色经验 | `CC_ExperienceBackendSubsystem` | 目前显式快照框架，只存经验，尚未自动登录或自动同步 |

本地角色存档目前仅 **Standalone 单机且权威端**支持；联机客户端调用 SaveProgress/LoadProgress 返回 false。默认不自动读取旧进度，便于测试新配置。读取成功后不要自行重建/覆盖四槽，等状态与技能事件刷新。

稳定 ID 是存档/配置关联键：`FormId / SkillId / UpgradeId` 用于当前角色存档关联；`StatProfileId` 用于形态/怪物引用数值档案，目前不直接写入 `ProgressionSave`。各类 ID 在各自目录内唯一并长期保留。美术改名使用 DisplayName；不要把按钮标题或目录排序作为关联标识。调整前面三类 ID 时需要另做存档迁移，调整 StatProfileId 时需要更新相应配置引用。

后端联调流程由可信游戏层编排，不把共享 Token 填到 Widget、资产或发布客户端：

```text
获取 CC_ExperienceBackendSubsystem → 绑定 OnCompleted
→ LoadCharacterExperience(CharacterId)
→ 成功并且尚未开放战斗奖励时，由权威端调用 RestoreExperienceFromBackend
→ 保存点读取当前余额 → SaveCharacterExperience(CharacterId, Experience)
→ 超时/不确定结果时 RetryPendingSave（重试同一请求）
```

`OnCompleted` 提供 bSuccess、CharacterId、Experience、Version、Error；`bRequestInFlight / bHasPendingSave` 可显示加载和重试状态。第一次保存前必须先读版本；保存成功返回的旧快照不能覆盖正在游戏的余额。后端经验和本地完整进度要明确谁是权威来源，不能云端读取后再随意 LoadProgress 覆盖经验。详细服务启动见 [Backend/README](../Backend/README.md)。

## 13. 精英与数值展示的特殊情况

青铜精英使用铜械降神，陶塑精英暂用陶土僧人。两者为同路线高级生命/攻击的 150%，模型为初始配置体型的 1.2 倍：

| 档案 | HP | ATK | 粒子 75% 减伤前强度 |
|---|---:|---:|---:|
| Bronze_Elite | 750 | 112.5 | 84.375 |
| Ceramic_Elite | 1050 | 101.25 | 75.9375 |

陶土僧人同时用于普通高级和精英，不能只凭 Mesh 名称显示“精英”。读取怪物实际 `StatProfileId / CombatBalance.Profiles` 的 bElite，或者让目标展示逻辑显式提供对应档案；普通用 `Ceramic_T3`，精英用 `Ceramic_Elite`。陶罐头保留草竹，当前 `Nature_PotHead` 暂继承自然高级，不显示为陶塑精英。

怪物实际血条/属性从其 ASC 读；形态选择预览从 GetStagePreview 读。正式 UI 不复制另一套倍率表作为运算来源。

## 14. 建议接入顺序与验收

前轮文字菜单与接口版本曾完成核心构建/自动测试、菜单替换和结果回调验证；这些历史记录不代表本轮壁画菜单已经通过。当前已生成 WBP_MuralMenu、共用美术数据和逐档立绘引用，本轮编译、器物只入包/背包使用、四页与截图验证已通过。新增 `CC.MuralMenuSmokeTest` 输出 `Saved/MuralMenuSmoke.json` 和 `Saved/Screenshots/MuralMenu/<UTC时间>/`，最终以本轮报告为准。以下继续作为验收清单。

建议每完成一项就进原平地地图验证，再加下一页：

1. **菜单容器**：配置 ProgressionMenuClass；Tab/关闭按钮都能进出，光标、焦点、移动和镜头恢复，反复开关不叠界面。
2. **HUD 和背包**：创建一次 HUD，关闭文字面板；首帧显示正确 HP/经验，受伤、治疗、改最大生命都刷新。一个怪领取 10、黑包领取 50，粒子飞出但未领取不提前入账。
3. **背包与进化**：初墨 75/11.25，不可直接进化；白/绿/黄方块分别只存自然/青铜/陶塑，背包选择不变身、切换按钮才使用。当前使用路线 99 经验不能买中级，100 成功扣至 0；高级另付 1000，三档封顶，切换保留档位。非当前所选力量须先切换再进化；隐藏两条未开放路线。
4. **技能**：目录共39项，每项都有名称/用途/数值；手动把 01 装入任一四槽，白板攻击 15 时简介显示 11.25；测试 21 交替拳击、22 疾跑，保留原按键技能用途。
5. **装备状态**：已有技能换槽会交换；冷却中换装有正确失败提示；变身后手动组保留，恢复默认后跟随当前档位。连续开关面板不重复收到一次装备的多条回调。
6. **属性商品**：经验不足、满级和成功各有提示；生命/攻击与技能简介同步变化，重复点击不在 UI 自行叠属性。
7. **设置与存档**：技能 1 改为允许键，新键生效旧键失效，HUD 角标更新；重启键位恢复。单机存读档保留进化、商品、经验和手动四槽，测试后恢复所需数据。
8. **重生与网络专项**：更换 Pawn 后 HUD 仍正确；两个客户端分别拥有自己的钱包/目录操作，结果回调和属性复制先后变化都不会显示错误四槽或负经验。网络专项需实际双客户端测试，不能只凭单机正常判定通过。

### 排错表

| 现象 | 先检查 |
|---|---|
| Tab 仍打开文字菜单 | 当前关卡实际 GameMode / Controller Class、ProgressionMenuClass 是否填在正在使用的蓝图 |
| 自制菜单设计器内容被覆盖 | CC_ProgressionMenu 和当前 CC_MuralMenuWidget 都由 C++ 构建树；完全自制 Designer 菜单应另建 UserWidget 子类 |
| 刚拾取元素但人物没变 | 当前设计为交互只入包；背包选中后点“切换形态” |
| 自然找不到或回自然回了初墨 | NativeForm 已是初墨；自然资产在 ArtifactForms，须白方块解锁后传该 Form 切换 |
| 立绘随浏览路线错误变化 | 读 GetActiveForm + CurrentTier 的 Stage，不能使用 BagChoice/待购买路线作为当前人物 |
| 关闭后不能移动或鼠标仍显示 | 是否绕开 CloseProgressionMenu 仅 RemoveFromParent、是否 UI 又自行叠加输入锁 |
| 血条初始为零或只动一次 | ASC 是否就绪；主动初值读取；Health 和 MaxHealth 是否都监听；旧代理是否错误 EndTask |
| 世界血条不更新 | HealthDisplay WidgetClass、CC_AttributeWidget 父类、AttributeMap 匹配，运行时替换后 RefreshBindings |
| UI 一直只显示形态默认技能 | 是否读了 CurrentLoadout，实际装备要读 CurrentSlots 并订阅 OnLoadoutChanged |
| 装备成功提示却短暂还是旧四槽 | RPC 结果先于复制；等待 CurrentSlots 到达，不自行猜测数组 |
| 技能库缺 21 / 22 | BP_ProgressionPlayerState.Progression.SkillCatalog 是否仍为 20；是否重跑了只注册 20 的原型脚本 |
| 技能强度过大或 75% 不随形态变化 | PowerMode、系数应 0.75、是否重复乘攻击或把白板值写死 |
| 进化预览对不上实际值 | GetStagePreview 为基础值；ASC 含购买/Buff；是否直接读备用 Stage 数值 |
| 浏览某路线却升级另一条 | Evolve 作用于当前使用形态；浏览路线须先 SwitchForm 并等待状态 |
| 商品成功却显示“进化成功” | OnPurchaseFinished 是共用结果事件，按 PendingRequest 区分操作 |
| 改键显示成功但按键无效 | 检查 SetCombatKey 返回值及保留键；不要仅更换按钮文字或直接改共享 IMC |
| 多开一次菜单就多回调一次 | Construct 重复绑定、Destruct 未解绑、属性 AsyncTask 未 EndTask |
| 保存读取失败 | 是否 Standalone/权威、角色已初始化、读取时五通道无冷却、稳定 ID 和目录有效 |

接口细分见 [C++ 父类配置索引](./CPP父类配置索引.md)，玩法数值与调参见 [数值与技能面板配置](./数值与技能面板配置.md)。本文的深度定制示例不会另建 WBP_GameMenu 等资源；当前已生成并使用的美术菜单为 WBP_MuralMenu，资源维护见 [壁画菜单配置](./壁画菜单资源配置.md)。

## 15. 验证记录与当前交接

前轮文字菜单接口交接记录保留在 `Saved/UIHandoffVerification.json`、`Saved/Logs/UIContractSmoke.log`，不作为本轮美术界面通过的依据。

本轮已配置 WBP_MuralMenu、DA_MenuArt、BP_MuralHUD、初墨与白色自然方块，并新增专用 smoke。本轮编译、5 项核心测试、184 项菜单实测检查和 8 张实际截图核对均通过。优先按 [壁画菜单资源配置](./壁画菜单资源配置.md) 联调当前入口；需要完全自定义 Designer 页面时再按第 3 节替换菜单，第 5 节继续接常驻 HUD/血条。联机、长期运行与美术验收仍按第 14 节逐项完成。

## 技能页美术更新（2026-10-24）

当前技能页18项已接真实装备、流派解锁和各自执行机制；四槽读取CurrentSlots，装备成功后1～4实际施放。原22模板保留，目录39项；新增铜币与存档v3。见[真实技能与铜币配置](./真实技能与铜币配置.md)和[技能界面美术配置](./技能界面美术配置.md)。

铜币在背包Wallet显示，Progression.CopperCoins通过OnChanged刷新，存入v3本地进度。服务端AddCopperCoins仅可信奖励调用；未增加客户端增发RPC或后端铜币存储。

## 元素球交互接口更新（2026-10-24）

E/RequestInteraction入口保持不变。器物的OnInteracted与本次OnPurchaseFinished.Success只表示已释放能力球；背包力量、50经验和流派技能解锁以能力球接触后的Progression.OnChanged为准。方块和能力球共享Artifact.GetPowerColors；青铜蓝黄、陶塑黄棕、自然绿色。见[元素信物与能力球配置](元素信物与能力球配置.md)。本轮编译/测试由前端安排。

## M/N地区UI接口更新

新增[地图传送、小地图与加载界面配置](地图传送与小地图配置.md)。三地区名称为休憩之地、小镇、山河社稷，当前地区/已访问/解锁状态来自AdventureGameInstance。前端通过RequestFastTravel发请求，不自行改VisitedRegions。GetLoadingProgress返回-1时用流动条，资源读取完成进入初始化阶段仍显示等待；不自行计时伪造总进度。

BP_ProgressionController.WorldNavigationClass可注入派生Designer界面。自建根控件后自行绑定OnNavigationVisibilityChanged，读取GetMinimapCamera.RenderTarget；显隐/焦点走Controller.SetTravelMenuOpen，避免只RemoveFromParent留下输入锁。M/N现为保留键。当前新界面未启动运行验证。

## 地图与设置UI（2026-10-24）

M/N原生布局已换成古风舆图、地区选择和启程确认；Tab设置页改为六个独立改键框、小地图和存档卡片。新增UCC_OptionsWidget与GetOptionsPanel、RebindAction、OnOptionsMessage等接入点。[配置与联调](地图与选项UI配置.md)。本轮只编译，不运行游戏测试。

## 格子背包最新接口

当前背包使用三列信物格子：SelectBagForm / GetSelectedBagForm只处理浏览，使用仍请求SwitchForm，进化请求EvolveForm(所选信物)，支持升级未使用的已拥有力量。GetOwnedFormTier与GetFormEvolutionCost读取所选档位/费用。初墨默认占一个格子，InventoryIcon/InventoryName在FormDefinition配置。初墨预览与实际属性按经验余额每点+1%并四舍五入，花费会重算。详细公式、占位图状态及接口见[格子背包与初墨成长配置](格子背包与初墨成长配置.md)。本轮只编译，未执行运行测试。


## 游戏内左下角生命 HUD

ACC_ProgressionHUD 在本地玩家创建 UCC_PlayerHealthHUDWidget，加入视口第 5 层。默认收起时只显示 HP、当前／最大生命值和一条生命进度；Tab 可展开已装备技能与冷却。

生命卡左下锚定，逻辑位置为距左 40、距下 40，尺寸 360×86；透明黑底、细金边、常态金色血条，生命低于等于 25% 时改为暗红。遵循视口 DPI 缩放，不获取键盘焦点，不拦截鼠标或角色输入。

生命与上限通过 ASC 属性事件更新；角色或 ASC 改变时重新绑定，销毁时解除委托。自己的头顶 HP 标签仅在本地隐藏，其他角色标签不受影响。交互提示整体上移到血条上方，避免重叠。现有全屏菜单在更高视口层覆盖 HUD。

布局与颜色在 Source/CrashCourse/Private/UI/CC_PlayerHealthHUDWidget.cpp 的 RebuildWidget／Refresh 调整。技能展开区也在该 Widget 的 Canvas 上，保持黑色半透明与金色描边。

实际受伤截图：Saved/FactionHealthSmokeUser/Saved/Screenshots/FactionHealthHUD/CombatHealth.png。血条值随成长后的最大生命与受伤实时变化，已由实际游戏烟测检查。


## 鼠标右键战技面板与滚轮选择

- 鼠标右键：展开／收起左下角战技面板。血条位置固定，四个已装备技能在上方显示。展开不暂停、不减速、不锁定移动或镜头，也不显示鼠标。
- 滚轮上／下：仅在展开时循环选择技能1～4及道具5，金边标示选中项；不会修改装配，收起时滚轮不改变选中槽。
- 鼠标左键／Enter：使用选中的技能或道具；普通技能、药水点击一次即可，蓄力技能按住对应按键蓄力、松开出招，另一按键的松开不会误释放。原1～5及自定义技能快捷键继续生效。道具共用15秒冷却，冷却中或空槽点击不会误触普攻。收起时左键恢复普攻。
- 图标使用现有 DA_SkillGallery，与槽位实际 SkillDefinition 对应；冷却取 GetCooldownRemaining，每 0.05 秒刷新可见面板，包含倒计时、进度和图标压暗。另显示空槽、未解锁、法力不足、受控、施放中等状态。
- Tab：打开完整菜单；完整菜单内再次按 Tab 可返回游戏。展开面板时打开完整菜单，战技面板先收起。切换槽位或收起时会取消由左键／Enter发起的未完成蓄力，沿用原3秒中断冷却，避免误放或持续蓄力。Enter、Tab、鼠标左右键、滚轮保留给界面，不能重绑为战斗快捷键。
- 技能区逻辑尺寸 360×386，距左 40、距下 136，血条仍为 360×86、距左下各 40。交互提示在展开时额外上移 430 个 1080p 基准单位。

实机检查命令 CC.CombatHUDSmokeTest，仅显式执行时运行并退出独立测试进程。报告 Saved/CombatHUDSmokeUser/Saved/CombatHUDSmoke.json，展开／收起截图位于该用户目录 Saved/Screenshots/CombatHUD/。检查覆盖鼠标右键、滚轮双向循环、图标、真实冷却倒计时、Enter 蓄力释放、收起和切换时取消蓄力、Tab 完整菜单与移动／镜头输入。


### 战技 HUD 的青铜线纹

血条与展开面板保留半透明黑底，提取原技能素材的回纹角花与云纹为暗金色细线：四角短折线回纹、标题两侧云纹、菱形小印和断开的细分隔线。选中槽位的角纹与侧标更亮，其他槽位降低透明度。纹样由 UCC_PlayerHealthHUDWidget::NativePaint 绘制，随 DPI 缩放，不新增贴图、按钮或输入命中区域；装配、冷却和 右键／滚轮／Enter／Tab 的逻辑沿用原实现。


### 小地图：嵌入式青铜边框

右下小地图改为 334×334 的正方形整体，距右下各 40 个逻辑单位，与左下血条呼应。314×314 的地图图像铺在边框内，地名和舆图标识叠在上沿透明黑带，北向提示与角色方向箭头叠在地图内，M／N 操作提示放在下沿透明黑带。所有文字均位于地图图像范围内，不再使用独立的上下信息块。

外框沿用战技 HUD 的暗金回纹角花、少量云纹和断开细分隔线，由 UCC_WorldNavigationWidget::NativePaint 绘制。地图始终保持正方形，无论视口比例如何变化；M 显隐、北向地图与旋转角色箭头、N 行旅传送、加载遮罩及完整菜单隐藏地图等逻辑保留。


### 小地图最终精简：仅地图

按最终要求，右下角仅显示正方形地图、角色方向箭头和暗金细边框。移除地名、舆图标识、北向文字、M／N 提示，以及承载这些文字的上下黑条与分隔线；完整 314×314 地图图像可见。M 显隐和 N 行旅操作继续有效，行旅面板的文字与功能保留。RegionTitle 仅作为内部刷新引用保留，不加入可见布局。此项覆盖上节的文字嵌入布局。


### 细长生命条与资源读数

生命框改为 580×58，血量横条 548×6，左下位置为 (40,-78)；下方独立资源行 580×32，位置 (40,-40)，显示实际持有经验、铜钱与右键／Tab 提示。战技面板底部上移至 -146，留出间隔。暗金回纹保留。此布局替代前述 360×86 生命卡。

资源行绑定玩家 PlayerState 的 Progression.OnChanged，经验获得、花费、铜钱变更、读档及复制更新均由实际数据驱动；更换 PlayerState 时重新绑定，销毁时解除。经验是可消费资源，不是角色等级。当前形态存在下一阶进化花费时，显示“经验 当前 / 需求”及细进度条；初墨、已满阶等没有目标的形态只显示经验总数，不虚构等级或升级阈值。GetDisplayedExperience／GetDisplayedCopperCoins／GetExperienceGoal 可供蓝图与实机检查读取。


### 第一人称准星与扫描

第一人称新增中心准星与六米自动扫描提示，文案位于内容浏览器 UI/Scan 的数据资产中；普通物体通过“扫描目标 / Scan Target”组件绑定。详细配置见 Docs/第一人称准星与扫描提示配置.md。第一人称 E 与准星共用目标射线，第三人称保留近邻交互；菜单和传送时隐藏准星。


### 最终规则：第一人称即观察模式

只有第一人称观察模式显示准星、扫描信息和 E 交互提示。按 T 返回第三人称后立即隐藏这些内容，即使旁边有可交互对象，也不显示提示；旧图片提示和调试准星同样受此限制。第一人称中对准范围内对象显示信息，符合真实交互距离时显示对应按键。菜单、死亡、传送时隐藏。此次只调整提示的可见性，原有第三人称 E 交互行为保持原样。提示文案仍保存在 UI/Scan 数据资产中。


### 菜单键避让编辑器测试退出键

鼠标右键展开／收起战技面板，Tab 打开／关闭完整菜单。键位录入时按 Tab 取消当前录入，行旅面板按 Tab 转入完整菜单。T 切换观察模式，疾跑使用长按左右 Shift。右键和 Tab 保留给界面，Shift 不再触发菜单。游戏不将 Esc 绑定到菜单，保留编辑器测试的原生退出操作。


### 万象天宫 E 的最终含义

万象天宫 E 改为整个 WFC 大场景的随机重组，直接切换道路与房屋布局，不再重排九块石板。观察模式文案由 UI/Scan/DA_Scan_HeavenScene 配置；第三人称隐藏提示的规则保留。具体行为与角色安全安置见 Docs/万象天宫整场景随机重组.md。
