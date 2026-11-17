# 地图与选项 UI 配置

更新：2026-10-24。按本次确认，选项指 Tab 菜单中的设置页，只细化现有功能。画质、音量设置不在本轮范围。

## 实际入口

| 入口 | 界面与操作 |
|---|---|
| M | 显示/隐藏右下角舆图；方形画面、当前地区、北向标记与角色方向 |
| N | 右侧行旅面板，选择地区、查看状态与简介、点击“启程传送” |
| Tab → 设置 | 六行独立改键框、恢复默认、小地图显隐、本地角色存读档 |

休憩之地、山河社稷默认可达。小镇在实际抵达前显示“尚未抵达”；可以选中查看解锁条件，但不能确认传送。当前地区显示“当前所在”。角色死亡、技能未结束/冷却未完成时，确认按钮显示等待；后端旅行入口仍会重新检查条件。选中地点本身不切图。

N/Esc 或行旅面板右上角 × 关闭面板。M 隐藏的是整个舆图区块，N 仍可单独打开。Tab 主菜单打开时地图隐藏并暂停相机捕获，关闭后按原显隐状态恢复。没有增加左下角返回提示，经验及铜币余额继续只在背包展示。

## 美术与布局

直接复用已导入资源，不需要再次运行素材导入脚本，也不需要重存关卡或重拼 WBP。

- 地图父类：`UCC_WorldNavigationWidget`，默认 `ArtData` 为 `/Game/UI/MuralMenu/DA_MenuArt`。
- 设置父类：新增 `UCC_OptionsWidget`，由 `UCC_MuralMenuWidget` 在设置页自动创建，并传入同一份 `ArtData`。
- 面板金边来自 `ArtData.SkillGallery.DetailPanel`，即已导入的空白金边面板；使用九宫格边框保持角饰比例。缺少素材时回退为金边深色面板。
- 共享原生样式位于 `Source/CrashCourse/Private/UI/CC_MenuStyle.h`：金色、米色文字，暗绿色底色，按钮悬停、按下、选中与禁用状态。
- 小地图独立锚定真实窗口右下角，锚点／对齐均为(1,1)，边距16个UMG逻辑单位；行旅面板仍按1920×1080设计画布等比适配。超宽屏的小地图也靠实际右下角，内容仍为方形256×256，不拉伸场景比例。
- 设置页沿用现有全屏壁画背景、左侧导航、右侧当前形态立绘与右上称号；内部两列分别为改键和地图/存档卡片，外层保留滚动。

当前是实景摄像机舆图，未新增各地图的静态手绘地图或关卡截图。

### 2026-10-25 文字适配修复

共享按钮不再自动换行，内容槽水平铺满、垂直居中，单行行高为1；只有正文和反馈保持换行。避免缩放布局预计算后，居中的文字槽保留较窄宽度，把最后一个字拆到下一行。固定高度单行文字启用边界裁剪和省略号，供以后自定义长名称使用；当前三地区名称与状态保留完整显示。

行旅面板设计尺寸改为620×566，地区行568×58，确认按钮568×56；目的地标题与简介下移分区，关闭按钮单独去掉多余内边距。小地图地区标题、底部提示及背包信物名称/状态同样采用单行策略。设置页按钮共享本次修复，说明正文仍能换行。已有Designer布局需自行检查TextBlock的AutoWrapText、ButtonSlot对齐与内边距。

原生Slate几何回归检查为 `CrashCourse.UI.MenuButtonLayout`：主页三按钮、三地区名称/状态、等待提示、设置按钮，在0.5、2/3、1、4/3、1.5缩放下测量实际文字尺寸与槽位尺寸，并检查缩小后的再次布局；该检查不创建游戏世界、角色或读写存档。构建日志 `Saved/Logs/MenuLayoutBuild.log`，检查日志 `Saved/Logs/MenuLayoutAutomation.log`。实际游戏窗口仍由用户检查。

本次 UE5.6 Editor/Win64/Development 编译与链接成功；上述检查成功，8种按钮文字×6次缩放分配，共48组布局均完整容纳文字。首页网址资源保存成功，报告 `Saved/UI/HomeWebsiteSetup.json`。未执行PIE、游戏启动、传送或浏览器点击测试。

## 设置页数据与行为

六行顺序固定：技能一、二、三、四、普通攻击、拾取/交互，索引 0～5。直接读 `PlayerController.GetCombatKey`；点击对应键位框后录入，调用 `SetCombatKey`，使用既有重复键/保留键校验及本机键位存储。失败后还原为实际键位并显示原因。恢复默认调用 `ResetCombatKeys`。

鼠标左键始终是额外普攻。移动、跳跃、镜头沿用模板；说明中展示 WASD、空格、鼠标和 Tab/M/N，隐藏开发按键不出现在此界面。录入期间 Tab/Esc 先交给按键框处理，避免菜单提前关闭。

小地图卡片显示当前开关状态，通过控制器切换；显隐跨图在同一会话保留，未新增跨启动偏好存储。关闭地图或打开 Tab 时停止捕获，仍使用原有相机参数。

存档卡片显示本机是否存在角色档。保存/读取直接调用 `Progression.SaveProgress/LoadProgress`，按实际返回值提示成功或失败；无存档时禁用读取。读取会替换当前未保存的角色进度，界面已说明。仅支持当前单机模式；不会清零角色或创建第二套存档。启动恢复、切图前后保存沿用已有实现。

## 给后续 UI 的接口

`UCC_MuralMenuWidget.GetOptionsPanel()` 获取设置实例。`UCC_OptionsWidget` 提供：

- `GetKeySelector(Action)`：获取原生六个录入框之一。
- `IsSelectingKey()`：父菜单判断是否正在录入。
- `RefreshOptions()`：从实际角色/控制器刷新；刷新代理会在销毁时解绑。
- `RebindAction(Action, InputChord)`：按既有规则请求改键，返回是否接受。
- `ResetKeys / ToggleMap / SaveProgress / LoadProgress`：对应操作。
- `FeedbackMessage / OnOptionsMessage(Message, bError)`：显示操作反馈。

地图保留 `Controller.WorldNavigationClass` 替换入口，以及 `InitializeNavigation / ToggleMinimap / SetTravelOpen / IsTravelOpen / IsMinimapVisible / OnNavigationVisibilityChanged`。新增 `SelectRegion(Id)` 和 `GetSelectedRegion()` 用于待传送选择；正式切图仍调用 `AdventureGameInstance.RequestFastTravel`。开关 N 应通过 `Controller.SetTravelMenuOpen`，保证输入、焦点和鼠标同步。

## 验证与手动检查

已通过 UE5.6 `CrashCourseEditor / Win64 / Development` 编译及 DLL 链接，构建日志为 `Saved/Logs/MapOptionsUIBuild.log`。按并行协作约定，不启动编辑器、不执行运行测试或生成游戏截图；历史菜单截图不代表当前页面已验收。原有菜单 smoke 的控件检查已改为检查新的设置子控件，尚未运行。

建议手动检查：M 完整显隐；隐藏地图后 N 仍可用；锁定小镇能查看条件但无法启程；选择其他地区后确认才切图；Tab/N 来回打开后恢复移动与光标；设置六行显示实际键位，改键成功/冲突与取消录入均正确；存读档反馈有效，立绘与称号继续跟随当前形态。

[地图旅行机制](地图传送与小地图配置.md) · [设置父类](CPPClasses/UCC_OptionsWidget.md) · [地图父类](CPPClasses/UCC_WorldNavigationWidget.md)
