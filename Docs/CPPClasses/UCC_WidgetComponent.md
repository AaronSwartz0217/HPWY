# UCC_WidgetComponent

父类：`UWidgetComponent`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/UI/CC_WidgetComponent.h) · [实现](../../Source/CrashCourse/Private/UI/CC_WidgetComponent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

角色头顶等世界空间属性 UI 的 WidgetComponent，按 AttributeMap 自动寻找 CC_AttributeWidget 并绑定数值。

## 配置方式

CC_BaseCharacter 已自动创建 HealthDisplay，默认使用纯文字 CC_AttributeWidget，显示生命值。通常直接配置这份继承组件，不要再添加重复血条。额外的蓝条可另加组件，设置 WidgetClass 与 AttributeMap 的 Mana→MaxMana。

## 参数与接口

- AttributeMap：当前属性到最大属性的映射。
- 普通 WidgetComponent 的 Space、DrawSize、位置仍在组件详情配置。
- 默认 AttributeMap 为 Health→MaxHealth，同时监听当前值和最大值。
- RefreshBindings：更换控件或属性配置后可显式重新绑定；重复调用先解绑，不叠加监听。
- 监听 OnASCInitialized 与 OnAttributesInitialized，并在属性初始化 GE 尚未执行时先订阅变化，初值可为零，之后会自动刷新。
- 重建控件会重新推送初始值；EndPlay 移除属性、ASC 和初始化事件监听。

## 最小验证

进入游戏、受伤、恢复时，匹配的控件应更新；先用单角色与单血条排查。

## 当前限制

应挂在 CC_BaseCharacter 派生角色；普通 Actor 会安全跳过绑定。专用服务器不创建显示绑定。AttributeMap 与控件属性必须匹配；新的 HUD／AddToViewport 控件仍需另行接入，不能依靠头顶组件自动订阅。
