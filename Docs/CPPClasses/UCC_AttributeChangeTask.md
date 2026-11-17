# UCC_AttributeChangeTask

架构审计：正式血条已由CC_WidgetComponent直接绑定属性委托，当前未检出该旧异步节点的外部原生或资源使用证据，列为清理候选，尚未删除。若重新启用，应先补齐异步代理的持有/生命周期和EndTask释放约定。 [完整审计](../Architecture/C++架构审计与优化路线.md)。

父类：`UBlueprintAsyncActionBase`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Tasks/CC_AttributeChangeTask.h) · [实现](../../Source/CrashCourse/Private/Tasks/CC_AttributeChangeTask.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

蓝图异步监听某个 GAS 属性变化。

## 配置方式

等 ASC 初始化后调用 Listen For Attribute Change，传 ASC 与 Attribute，保存返回的 AsyncTask 引用；绑定 OnAttributeChanged。控件销毁／角色解绑时调用 EndTask。

## 参数与接口

- OnAttributeChanged：Attribute、NewValue、OldValue。
- EndTask：移除监听并释放代理。

## 最小验证

重复打开／关闭 UI 后，每次变化应只回调一次。

## 当前限制

这是异步节点代理，不是可挂载组件，不需创建父蓝图。它不自动发送初始值，也不自动在所有 UI 生命周期结束时解绑。
