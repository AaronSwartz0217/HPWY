# UCC_UpgradeDefinition

父类：`UDataAsset`。核对日期：2026-10-23。

[头文件](../../Source/CrashCourse/Public/Progression/CC_GameplayData.h) · [实现](../../Source/CrashCourse/Private/Progression/CC_GameplayData.cpp) · [返回索引](../CPP父类配置索引.md)

## 用途

经验商品定义，支持全局属性购买和按 SkillId 购买技能等级。

## 配置方式

创建 CC_UpgradeDefinition Data Asset，设置稳定 UpgradeId、Type、Prices，然后加入 PlayerState.Progression.UpgradeCatalog。正式 UI 的按钮保存资产引用，点击调用 Purchase。

## 参数与接口

- UpgradeId：存档键，目录内保持唯一。
- Type：MaxHealth、MaxMana、Attack、Defense、MoveSpeed、SkillLevel。
- Prices：每级价格列表，数组长度就是可购买上限。
- BonusPerLevel：每级属性增量；SkillLevel 类型忽略此字段，每次固定增加一级。
- SkillId：仅 SkillLevel 使用，必须与技能资产一致。
- DisplayName：商品显示名。

## 最小验证

例如生命商品 Prices=[25,50]、BonusPerLevel=20：两次购买共花 75，最大生命累计加 40，第三次应提示满级。

## 当前限制

属性加成全形态共用；没有配置前置商品、形态专属范围或退款。不要给多个不同商品重复使用同一 UpgradeId。
