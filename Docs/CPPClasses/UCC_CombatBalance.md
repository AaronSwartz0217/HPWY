# UCC_CombatBalance

父类 `UDataAsset`。创建 Data Asset → CC_CombatBalance；示例 `/Game/HPWH/Balance/DA_CombatBalance`，不拖场景。

`BaseHealth=100`、`BaseAttack=15` 为统一白板。`Profiles` 中 FCC_CombatProfile 用唯一 ProfileId、模型 Mesh、生命 / 攻击倍率定义档位；ParentProfileId 可继承另一个档位。精英行继承同类高级，EliteMultiplier=1.5、EliteScale=1.2，并引用 DA_EliteLoadout。Resolve 拒绝循环、缺失父级、无效倍率。

形态 Stage 通过 CombatBalance + StatProfileId 读取生命 / 攻击；敌人通过同名字段读取，ID 留空时按 Mesh 匹配。显式 ID 优先，适合共用模型的精英。普通陶土僧人 Ceramic_T3，精英 Ceramic_Elite；陶罐头 Nature_PotHead 仍归草竹。

FindProfile 用于查找配置行；Resolve 输出实际生命 / 攻击。倍率 140% 填 1.4。修改高级倍率会自动影响继承它的精英，不必复制数值。

[完整配置表](../数值与技能面板配置.md) · [头文件](../../Source/CrashCourse/Public/Progression/CC_GameplayData.h) · [返回索引](../CPP父类配置索引.md)

## 2026-10-24真实技能与铜币更新

Profiles.ExperienceReward=-1继承；T1=10/T2=40/T3=200/Elite=500，ResolveExperienceReward供敌人ApplyCombatProfile应用。陶罐头继承自然高级200。

详见[真实技能与铜币配置](../真实技能与铜币配置.md)。
