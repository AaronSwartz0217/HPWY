# UCC_OptionsWidget

父类 UUserWidget。Tab → 设置页的原生双列内容，由 UCC_MuralMenuWidget 自动创建，ArtData 继承主菜单配置；无需额外配置 WBP。

左列六个 InputKeySelector 对应控制器动作 0～5：四技能、普攻、交互。右列小地图开关、本机存档状态、保存/读取按钮。复用既有控制器、Progression 和 WorldNavigation 接口，未增加钱包或存档副本。

| 接口 | 作用 |
|---|---|
| ArtData | 共用金边面板美术；空值使用原生回退样式 |
| GetKeySelector(Action) | 获取原生录入框，越界返回空 |
| IsSelectingKey | 主菜单录入保护 |
| RefreshOptions | 刷新实际键位、小地图及存档状态 |
| RebindAction | 传动作索引和 InputChord；使用既有占用/保留键校验 |
| ResetKeys / ToggleMap | 恢复默认键位、切换舆图 |
| SaveProgress / LoadProgress | 本机角色存读档，结果显示在反馈区 |
| FeedbackMessage / OnOptionsMessage | 后续自制 UI 可读取/接收操作反馈 |

RebuildWidget 在 Slate 选择根内容之前构建原生控件树，避免首次空白；NativeConstruct 绑定 OnCombatKeysChanged，NativeDestruct 解绑。程序刷新键位时使用保护标记，避免再次写档。录入期间不覆写正在捕获的键位框。地图开关在同次会话跨图保留，键位由原控制器自动持久化。

当前类用于自动构建原生布局；完全自制 Designer 页面可直接使用控制器/Progression 接口，或复用本类操作函数及反馈事件，自行处理录入期间的菜单关闭保护。

[完整配置](../地图与选项UI配置.md) · [主菜单](UCC_MuralMenuWidget.md) · [父类索引](../CPP父类配置索引.md)
