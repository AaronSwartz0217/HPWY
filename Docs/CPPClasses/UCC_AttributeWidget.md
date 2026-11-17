# UCC_AttributeWidget

父类：`UUserWidget`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/UI/CC_AttributeWidget.h) · [实现](../../Source/CrashCourse/Private/UI/CC_AttributeWidget.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

单个血条／蓝条等属性控件的 UserWidget 父类。

## 配置方式

现在可以直接使用原生 CC_AttributeWidget，无设计器控件树时会创建纯文字，默认 Health→MaxHealth，显示 HP 当前值／最大值。要恢复图形血条时，创建 Widget Blueprint 继承它，放 ProgressBar 并实现 On Attribute Change；已有控件树不会被默认文字覆盖。

## 参数与接口

- Attribute、MaxAttribute：要显示的两个 GAS 属性。
- On Attribute Change(NewValue,NewMaxValue,OldValue)：设置进度条百分比与文本；最大值为 0 时自行保护。
- AvatarActor：绑定的角色，只读。
- CurrentValue、CurrentMaxValue、ValuePercent：已计算的只读显示值；ValuePercent 对零最大值作保护并限制在 0～1。

## 最小验证

由 CC_WidgetComponent 绑定后，受伤时百分比 NewValue/NewMaxValue 改变；不是只在 Construct 写初值。

## 当前限制

只创建控件并 AddToViewport 不会自动触发这里的世界空间绑定；屏幕 HUD 需要自己连接 ASC 监听或成长事件。
