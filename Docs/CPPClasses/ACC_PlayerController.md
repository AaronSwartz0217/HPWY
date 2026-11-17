# ACC_PlayerController

## 2026-10-25 架构审计修复

本控制器在LocalPlayer的EnhancedInput子系统中安装独立RuntimeContexts。EndPlay和再次安装前调用ReleaseRuntimeInputContexts，只移除自己创建的副本，避免切图后LocalPlayer仍持有旧映射；不清空其他系统输入。旧F1–F12调试绑定现在同0/9一样，仅编译进非Shipping/Test版本；开发版本仍受bShowProgressionPanel控制。正常键位不变，反复切图的实际输入行为仍需联调。

[全部类架构审计](../Architecture/C++架构审计与优化路线.md)。

父类：`APlayerController`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Player/CC_PlayerController.h) · [实现](../../Source/CrashCourse/Private/Player/CC_PlayerController.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

管理移动、视角、跳跃、普攻、四技能、菜单和器物/经验包交互的输入。

新增左右Shift长按固有疾跑输入，不占可改六键和四槽；保留Shift防止战斗键冲突。PlayerTick检查松开、菜单／旅行输入锁、死亡和换Pawn，释放可能丢失的输入；Tab／N打开时立即结束疾跑。请求交给实际玩家角色SetSprintHeld，参数与UI读取见[疾跑配置](../固有疾跑与小镇返回配置.md)。

## 配置方式

复制 BP_ProgressionController 保留现有输入资产。控制器 Class Defaults 保留现有六个战斗 InputAction 引用与 Input Mapping Contexts，用于识别并移除运行时副本中的旧战斗映射；战斗键位由设置页管理。GameMode 的 Player Controller Class 选择此蓝图。

## 参数与接口

- Move／Look Action：二维轴；Jump、Primary、Secondary、Tertiary、Fourth、Interact：对应动作。
- BasicAttackAction 为独立普攻动作，样例绑定 J；Primary／Secondary／Tertiary／Fourth 分别绑定 1～4。运行时战斗动作统一经可改键绑定在按下时尝试一次；Enhanced Input 保留移动、镜头、跳跃。
- Input Mapping Contexts：模板移动、镜头、跳跃所用 IMC，以及供运行时识别旧战斗映射的 IMC。
- Interaction Trace Distance：相机射线长度，默认 1500，和器物/经验包服务器交互距离 250 是不同限制。
- RequestInteraction(Target)：E 射线找到物体后使用的统一请求入口，服务器按目标类型调用器物或 ExperiencePack 的验证/交互。自建交互 UI 也应使用该入口，不自行发经验。
- Show Progression Panel：控制 F1～F12 测试购买 / 变身快捷键；普攻、四技能、交互与 Tab 菜单一直可用。HUD 显示由 GameMode.HUDClass 决定。
- 战斗只走可改键路径；运行时副本移除原战斗 Action 的映射，避免一次按键重复激活。

## 最小验证

验证 J / 鼠标左键、四技能、E 与模板移动；在设置中改一个技能键，确认新键生效、旧键失效并能在下次启动恢复。正式游戏可关闭 F1～F12 测试开关。

## 当前限制

前三槽在无 CurrentLoadout 时回退旧 Primary／Secondary／Tertiary 标签；第四槽没有旧能力回退。关闭测试开关不会自动隐藏单独配置的测试 HUD。

## 2026-10-24 数值与技能配置更新

本次输入更新：运行时复制 IMC，保留模板移动 / 跳跃 / 镜头映射，移除副本里的战斗映射后使用可改键绑定。原始 IMC 资产不修改。GetCombatKey / SetCombatKey / ResetCombatKeys 管理 0–3 技能、4 普攻、5 交互，默认 1/2/3/4/J/E。鼠标左键另行保留普攻。重复键、模板移动键和 Tab/Esc 被拒绝；键位写入独立 SpiritTalesControls 存档。

完整参数表与操作见 [数值与技能面板配置](../数值与技能面板配置.md)。

## 2026-10-24 正式 UMG 接入口

`ProgressionMenuClass` 可在控制器蓝图 Class Defaults 的 Crash|Menu 配置为任意 UserWidget 子类，留空使用现有文字菜单。正式 WBP_GameMenu 直接继承 UserWidget；不要继承会生成原生文字树的 UCC_ProgressionMenu。

`OpenProgressionMenu` / `CloseProgressionMenu` 为幂等入口，`ToggleProgressionMenu` 保留 Tab。`IsProgressionMenuOpen` / `GetProgressionMenuWidget` 可在蓝图查询。`OnMenuVisibilityChanged(bool)` 通知开关，`OnCombatKeysChanged()` 通知改键；监听后主动读取初值。创建控件时控制器设置键盘焦点，关闭统一恢复输入。自定义控件在 OnPreviewKeyDown 处理 Tab/Esc 调 Close，返回 Handled，关闭按钮也调 Close，不自行 RemoveFromParent。

[按步骤接入 UMG](../UI操作与UMG接入配置.md)。

## 范围交互（2026-10-24）

E 键先优先选择相机射线命中的可交互目标；未命中有效目标时，在角色周围搜索碰撞体，选择可达且未被遮挡的附近器物/经验包，距离相近时优先视野方向。`NearbyInteractionSearchRadius` 默认 450 厘米，可在控制器 Class Defaults 的 Crash / Interaction 中调整；它是候选搜索范围，最终距离仍由物体的 `InteractionDistance` 校验。

`FindInteractionTarget()` 可供蓝图交互提示查询，返回当前可用目标或空。菜单开启、死亡、禁用器物、已消耗经验包均不能成为目标。无需增大方块的实体碰撞体。

开发者专用隐藏 0/9 快捷键见 [开发隐藏按键](../开发隐藏按键.md)，不会显示于玩家界面。

## M/N与旅行UI

新增M切地图、N切快捷传送菜单，二者保留且不可绑定战斗技能。IsGameplayInputBlocked统一检查Tab、N和加载状态；SetTravelMenuOpen处理鼠标、焦点与移动/视角输入锁。MinimapClass/WorldNavigationClass可配置派生相机和UI；GetMinimapCamera/GetWorldNavigationWidget供蓝图查询。旅行UI关闭应走控制器入口。见[地图传送与小地图](../地图传送与小地图配置.md)。
