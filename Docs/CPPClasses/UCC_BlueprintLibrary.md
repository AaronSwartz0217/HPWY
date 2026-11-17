# UCC_BlueprintLibrary

父类：`UBlueprintFunctionLibrary`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Utils/CC_BlueprintLibrary.h) · [实现](../../Source/CrashCourse/Private/Utils/CC_BlueprintLibrary.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

静态蓝图函数库，无需实例、组件或父蓝图。

## 配置方式

在蓝图事件图搜索函数名直接调用；与属性修改有关的函数仅从权威逻辑调用。新模板优先使用统一 IncomingDamage 执行链。

## 参数与接口

- GetHitDirection／GetHitDirectionName：计算并命名受击方向。
- FindClosestActorWithTag：在 SearchRange 内找存活 BaseCharacter。
- HitBoxOverlapTest：球形范围检测，返回去重 Actor。
- ApplyKnockback：按内外半径衰减的击退。
- SendDamageEventToPlayer(s)：旧伤害事件与 GE 应用；Damage 传正数，Payload.Instigator 为有效攻击者，DataTag 对应 GE 的 SetByCaller。

## 最小验证

检验查询超距目标被排除；旧伤害来源不能丢失，否则击杀经验无法归属。

## 当前限制

不是所有函数都会替调用方做网络权限校验；可蓝图调用不代表可由客户端随意直接结算伤害。旧 Health 伤害路径不经过新防御与护盾计算。
