# UCC_MuralMenuWidget

父类：`UUserWidget`。编辑器名称：`CC_MuralMenuWidget`。核对日期：2026-10-24。

[头文件](../../Source/CrashCourse/Public/UI/CC_MuralMenuWidget.h) · [实现](../../Source/CrashCourse/Private/UI/CC_MuralMenuWidget.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途与已有入口

当前壁画菜单的原生布局与玩法接入父类。已生成 `/Game/UI/MuralMenu/WBP_MuralMenu`，在 `BP_ProgressionController.ProgressionMenuClass` 使用；Class Defaults 的 `ArtData` 指向 `DA_MenuArt`。

Tab 打开全 viewport 菜单，默认 `Overview` 主页。点击左侧导航进入背包、技能、交易、设置四个独立功能页，返回主菜单按钮回 Overview。Tab / Esc 统一走控制器 `CloseProgressionMenu()`。录入新按键时由 InputKeySelector 先处理，避免把本次录入当成关闭菜单。

## 配置字段与公开接口

| 字段 / 蓝图接口 | 用途 |
|---|---|
| `ArtData` | 共用背景、Logo、四导航普通/激活图和初墨回退图 |
| `ShowPage(ECC_MuralMenuPage)` | 切换 Overview、Inventory、Skills、Trade、Settings |
| `SelectSkillSlot(int32 Index)` | 选择待装备槽位，0～3；不是立即施放技能 |
| `GetMenuPage()` | 查询当前页面 |
| `GetOptionsPanel()` | 获取新设置子控件，包含六个独立改键框、小地图和存读档 |
| `SelectBagForm(Form)` / `GetSelectedBagForm()` | 格子背包浏览选择，不立即变身 |
| `GetDisplayedPortraitTexture()` | 读取当前实际显示的立绘纹理 |
| `GetDisplayedTitleTexture()` | 读取当前实际显示的称号纹理 |
| `GetDisplayedFormId()` / `GetDisplayedTier()` | 当前展示的真实形态 ID / 档位，下标 0～2 |

本类在 `NativeOnInitialized` 构建控件树。仅更换美术时修改 `ArtData` 或各 Stage 字段即可，不要在 Designer 再堆一套布局。完全自定义 Designer 页面请新建 UserWidget 子类，按 [UI 接入文档](../UI操作与UMG接入配置.md) 使用已有控制器、Progression、Skills 接口。

## 背包与变身

`Progression.NativeForm` 是 `DA_Form_InitialInk`（FormId=InitialInk）。原始基准生命75、攻击11.25；按当前经验每点+1%并四舍五入，零经验实际75/11。`bCanEvolve=false`，不购买三档进化，依靠经验余额强化。

元素方块交互释放能力球，实际领取后力量进入 `Progression.Forms`，并获得50经验。格子背包默认自带初墨，展示已获得信物；点击格子只浏览，“使用”才请求 `SwitchForm`。进化通过 `EvolveForm(Selected)`作用于所选信物，不必先变身；余额仍仅在背包显示。切换保留各路线档位，重复获得不重复添加。

自然仍使用原 `DA_Form_Native` 和稳定 FormId=Native，但已进入 `ArtifactForms` 解锁目录。`SwitchForm(nullptr)` 回初墨；切自然应传自然 Form 资产。

## 页面与数据来源

| 页面 | 数据与行为 |
|---|---|
| 背包 | 三列信物格子、默认初墨、使用/所选信物进化、成长说明与费用、共用钱包及当前真实属性 |
| 技能 | `SkillCatalog` 共39项、数值简介、四槽选择/装备及恢复当前形态默认技能 |
| 交易 | 永久属性等已配置商品购买 |
| 设置 | UCC_OptionsWidget双列面板：六个独立改键框、恢复默认、小地图开关、单机存读档 |

属性从当前角色 ASC 读取；预览通过 `GetStagePreview` 解析共享数值；技能说明调用 `SkillDefinition.GetNumericDescription`。进化中级 100 经验、高级 1000 经验，具体仍读配置。购买与装备由服务器再次校验，界面不自行扣款或加属性。

## 立绘和右上称号

菜单根据 `GetActiveForm()` 与 `CurrentTier` 读取 `FCC_FormStage.MenuPortrait / MenuTitle / MenuDisplayName`。`CurrentForm=nullptr` 表示使用 `NativeForm` 初墨。不能根据中文名称、网格体名称或下拉框待选路线猜当前图。

立绘在右侧；竖版称号保持右上方。进化/变身通知到达时刷新图片；浏览背包或交易路线只更改预览内容，不替换当前实际形态的立绘。每档缺图时使用 `DA_MenuArt` 的初墨回退。

## 生命周期与限制

`NativeConstruct` 获取所属玩家 Progression、角色 Skills 与控制器，订阅成长/购买/装备/技能组/键位变化；`NativeDestruct` 解除绑定。短间隔刷新可在 Pawn 尚未就绪或更换时重新解析引用。控制器负责创建、销毁、光标与输入锁，避免 Widget 再叠一层输入模式。

菜单打开时世界与怪物继续运行。当前没有普通物品格或技能解锁商店；背包“已存元素”使用实际 `Forms`，目录39项，流派项需已获取对应力量。

## 本轮验证状态

新增专用运行入口 `CC.MuralMenuSmokeTest`，用于检查初墨、三种真实器物获取、显式使用、九档配图、右上称号、22 技能装备、四页及分帧截图；输出 `Saved/MuralMenuSmoke.json`。须在单独 standalone 游戏进程使用渲染，测试不保存玩家进度、不改键位。

本轮构建、5 项核心自动测试、184 项菜单实测检查与 8 张分帧截图核对通过；菜单接口兼容和实际改键/粒子命中检查也通过。详细配图表见 [壁画菜单资源配置](../壁画菜单资源配置.md)。

## 新技能美术页

ArtData.SkillGallery 配置后创建 CC_SkillGalleryWidget，技能页隐藏通用面板、立绘和称号，展示四分类/图标/详情/真实装备槽；其他页面恢复原显示。GetSkillGallery() 可读当前子页。清空 ArtData.SkillGallery 时回退原22技能配置页。详见 [技能界面美术配置](../技能界面美术配置.md)。

## 2026-10-24真实技能与铜币更新

默认技能页已真实装备18项，不再临时预览；完整执行目录39项。背包Wallet同时显示经验和铜币；技能结果回调通知Gallery。

详见[真实技能与铜币配置](../真实技能与铜币配置.md)。
