# UCC_WaitGameplayEvent

父类：`UAbilityAsync_WaitGameplayEvent`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/AbilitySystem/AbilityTasks/CC_WaitGameplayEvent.h) · [实现](../../Source/CrashCourse/Private/AbilitySystem/AbilityTasks/CC_WaitGameplayEvent.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

旧搜索 AI 使用的 GameplayEvent 异步代理，提供额外的 C++ StartActivation 入口。

## 配置方式

通过 Wait Gameplay Event To Actor Proxy 节点／C++ 工厂创建，传目标 Actor 与事件标签，绑定 EventReceived。C++ 创建后调用 StartActivation；蓝图异步节点使用自身激活机制。

## 参数与接口

- TargetActor：带 ASC 的目标。
- EventTag：要监听的 GameplayTag。
- OnlyTriggerOnce：默认 false。
- OnlyMatchExact：默认 true。

## 最小验证

发送匹配事件时回调，不匹配标签不应误触发；单次模式只处理一次。

## 当前限制

不是 Actor 或组件；持久监听需要在结束生命周期时清理，不能在每轮搜索重复创建未结束的代理。
