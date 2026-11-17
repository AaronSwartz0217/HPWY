# 平地测试场与 UMG 修复说明

> 历史阶段记录：本文保留平地初建和UMG生命周期修复的诊断、操作与截图。初建时的100生命、自然默认、纯文字HUD及F键步骤不再是当前默认配置。当前版本请先读[完整功能更新与配置说明](完整功能更新与配置说明.md)。
>
> 当前：初墨75生命/11.25攻击；白/绿/黄方块存入自然/青铜/陶塑，背包显式切换/进化；Tab壁画菜单；18项新技能预览；22项战斗模板；六状态动画与死亡布娃娃；两类精英；左下范围拾取提示；经验只在背包显示。下方UMG绑定修复仍有效。

## 初建阶段测试场

关卡：`/Game/HPWH/Maps/L_FlatTestArena`。

- 120 米 × 120 米平地，无建筑与关卡障碍。
- 玩家自然形态使用竹人三档；青铜、陶塑均有独立三档模型；10 个 NPC 已换成青铜／草竹系列，并配置专用待机与跑步。
- 当前开放自然之力、青铜之力、陶塑之力，各有初级／中级／高级；中级 100 经验，高级 1000 经验。
- 绿色方块为青铜、黄色为陶塑；其余两个方块隐藏且关闭交互。
- 方块后方是 6 个不主动攻击的人物靶子，每个击杀奖励 10 经验。
- 更远处放 4 个 Patrol_Enemy 测试敌人，继承 CC_PatrolEnemyCharacter，保留四技能和经验奖励。出生点周围随机巡逻，发现玩家后追击，脱战返回中心。配置见 [巡逻父类说明](CPPClasses/ACC_PatrolEnemyCharacter.md)。
- 场景已经放置 Arena_NavMeshBounds，覆盖主要平地；Recast 使用运行时动态生成导航。编辑器按 P 检查绿色可行走区域，运行开始时需等待导航就绪。
- 地面标记使用简单英文文字；形态数据资产保存中文显示名。没有正式美术 UI。

最新配置见 [菜单与三种力量配置](菜单与三种力量配置.md)。

## 初建阶段测试操作

1. Play 后从出生点向前走，朝向任意彩色方块，近距离按 E。
2. 查看形态、四技能和属性是否切换。
3. 左键或 J 为独立普攻，数字键 1～4 对应四个技能；在人物靶子上验证伤害与经验。普攻不占技能槽，当前不消耗法力，攻击间隔 0.4 秒。
4. F1 购买当前形态的下一档；F2 返回原生；F3～F6 切换已解锁样式。
5. F7～F12 测试购买各属性／技能等级商品。
6. 接近远处巡逻敌人验证警戒与追击，再离开其出生中心 1800 厘米以上，观察清除目标、返回和继续巡逻。

HUD 为纯文字：经验、形态、进化档位、费用、生命、法力、护盾、攻击、防御、移速、技能等级和冷却。角色头顶的 UMG 同样先显示 HP 当前值／最大值，之后可换成 ProgressBar。

## 初建阶段键位

| 操作 | 按键 |
|---|---|
| 普攻 | 鼠标左键或 J |
| 菜单 | Tab，Esc 返回游戏 |
| 四个技能 | 1、2、3、4 |
| 移动 | W、A、S、D，保留模板轴映射与修饰器 |
| 视角 | 鼠标，保留模板设置 |
| 跳跃 | 空格，保留模板设置 |
| 器物交互 | E |
| 原型进化／切换／购买 | 保留 F1～F12 测试操作 |

正式输入资产在 `/Game/HPWH/Input`。IMC_Combat_J1234 管理 J、1～4、E；IMC_TemplateMovement_* 复制原模板移动映射并保留轴修饰器。普攻配置在 `/Game/HPWH/Skills/DA_BasicAttack`，由玩家 Skills.BasicAttack 指定；切换形态仍保留一份独立普攻。

原始 OverRide 和旧 L_HPWH 均保留。`build_flat_test_arena.py` 只搭建本次原型资源，避免重跑 setup_hpwh.py 覆盖已经修改的命名和颜色。

重建平地场景后再执行 `Scripts/setup_patrol_enemy.py`，它会创建／更新 BP_PatrolEnemy、替换 Active_Enemy_* 并配置导航范围。不要在尚未保存手工关卡修改时运行场景搭建脚本。

工程的 GameDefaultMap 和 EditorStartupMap 已指向 L_FlatTestArena；下次打开工程直接进入此场。

## 初建时查到的 UMG 状态

通过 UE 资源注册表及蓝图默认对象检查，本目录 `/Game` 中没有查到 WidgetBlueprint 资源，玩家／敌人也没有已配置的旧 WidgetComponent 引用。存在 UMG 的 C++ 类与模块依赖；旧敌人蓝图 Mesh／AnimClass 还为空，现已在示例敌人中补上玩家模型。

这能确认本工程快照中的资源和组件缺口，不能说明其他工程／未保存蓝图里的节点接线。Python 接口未开放这些蓝图的 ubergraph_pages，未声称逐节点审核过不可访问的事件图。

## 已修复的 C++ 逻辑

1. 原组件直接解引用角色和 WidgetTree，缺少空值保护；现在普通 Actor／尚未创建控件时安全退出。
2. 原组件只监听 Health，没有监听 MaxHealth；现在两者均监听，进化或属性购买引起上限变化也刷新显示。
3. 原回调引用捕获映射项并使用裸指针；现在由弱对象回调驱动，更新时遍历当前有效控件，不保留映射项引用。
4. 原流程在 ASC 初始化前后容易重复绑定；现在每次绑定先移除旧句柄，初始化事件使用唯一绑定。
5. 现在订阅后立即发送现有属性值；初始化 GE 较晚执行仍能更新，不依赖先后顺序碰巧正确。
6. EndPlay 解除属性、角色 ASC 初始化和属性初始化事件，避免销毁后继续回调。
7. 最大值变化时，OnAttributeChange 的 OldValue 仍按旧当前值传入，不误传旧最大值。
8. UCC_AttributeWidget 提供 CurrentValue／CurrentMaxValue／ValuePercent，零最大值不会除零；空控件树默认创建纯文字。
9. BaseCharacter 自带 HealthDisplay，所有角色继承同一绑定链，玩家和怪物不必重复拼接。
10. 测试角色改用明确设置生命、法力各 100 的 CC_InitialAttributesEffect，避免旧初始化资源没有数值而让 UI 正确地显示 0／0。

## 后续换回图形血条

1. 创建 Widget Blueprint，父类设 CC_AttributeWidget。
2. Attribute=Health，MaxAttribute=MaxHealth。
3. 放一个 ProgressBar，实现 On Attribute Change：使用 ValuePercent 或带零值保护的 NewValue/NewMaxValue 设置 Percent。
4. 把这个 Widget Blueprint 设到角色 HealthDisplay.WidgetClass。
5. 不要再在 Tick、Construct 和 On Attribute Change 三处重复 Add 监听；组件已经管理监听。
6. 如果运行时更换 WidgetClass，确认新控件创建后调用 RefreshBindings；默认 InitWidget 路径也会检测控件实例更换。

正式动画、伤害命中帧、投射物表现和多人 UI 同步仍按开发方向文档继续验收。本次不增加后端存档业务范围。

## 初建阶段验证记录

- UE 5.6 Editor Development 编译通过。
- CrashCourse.Progression.TransactionsAndDamage 通过，包含默认属性初始化、经验消费和技能激活。
- CrashCourse.UI.AttributeWidgetLifecycle 通过，覆盖初值、最大值单独变化、百分比零值保护、重新绑定及 EndPlay 后不再回调。
- CrashCourse.AI.PatrolTransitions 通过；实际关卡验证四敌人巡逻移动、发现玩家、脱战、返回后恢复巡逻和死亡停止，报告在 Saved/PatrolSmoke.json。
- 实际 D3D12 离屏渲染截图已检查，颜色、人物站立方向、HP 100／100 与 Mana 100／100 正常。截图在 Saved/Screenshots/FlatArena.png。
- 尚未完成手动操作全部 20 技能和双客户端测试；人物靶子的死亡动画、自动重生和复杂障碍场景下的 AI 调整仍是后续工作。
