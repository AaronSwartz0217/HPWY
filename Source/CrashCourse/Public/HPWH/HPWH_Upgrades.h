#pragma once

#include "CoreMinimal.h"

/** 卡牌稀有度：普通蓝色、中级紫色、极品金色。 */
enum class EHPWH_Rarity : uint8
{
	Common,
	Epic,
	Legendary
};

/** 肉鸽强化类型。 */
enum class EHPWH_UpgradeType : uint8
{
	AttackDamage,
	MaxHealth,
	MoveSpeed,
	AttackRange,
	BulletCooldown,
	MultiShotTwo,
	MultiShotThree,
	SoulGuard,
	SoulSpeed,
	ReturnRange,
	DamageTolerance,
	KillHeal,
	CrawlerDistance,
	CrawlerSlow,
	PuzzleTime,
	SuccessRate,
	InteractRange
};

/** 卡牌图标分类，与宣传网站的白色剪影图标一一对应。 */
enum class EHPWH_UpgradeIcon : uint8
{
	Attack,
	Life,
	Range,
	Soul,
	Fate
};

/** 一局内已经获得的强化，按类型合并并记录最高稀有度。 */
struct FHPWH_OwnedUpgrade
{
	EHPWH_UpgradeType Type = EHPWH_UpgradeType::AttackDamage;
	EHPWH_Rarity Rarity = EHPWH_Rarity::Common;
	int32 Count = 0;
};

/** 一局内累计的强化数值，由 GameMode 持有。 */
struct FHPWH_RunStats
{
	float AttackBonus = 0.f;
	float MaxHealthBonus = 0.f;
	float MoveSpeedBonus = 0.f;
	float AttackRangeBonus = 0.f;
	float BulletCooldownReduction = 0.f;
	int32 ProjectileCount = 1;
	int32 SoulHitsRequired = 1;
	float SoulSpeedBonus = 0.f;
	float ReturnRangeBonus = 0.f;
	float InvulnBonus = 0.f;
	float KillHeal = 0.f;
	int32 CrawlerDistanceCells = 0;
	float CrawlerSlow = 0.f;
	float PuzzleTimeBonus = 0.f;
	float ChanceBonus = 0.f;
	float InteractRangeBonus = 0.f;
};

struct FHPWH_UpgradeOption
{
	EHPWH_UpgradeType Type = EHPWH_UpgradeType::AttackDamage;
	EHPWH_Rarity Rarity = EHPWH_Rarity::Common;
	float Value = 0.f;
	FString Title;
	FString Description;
};

/** 三选一卡片的屏幕布局，HUD 绘制与触摸命中共用。 */
struct FHPWH_UpgradeCardLayout
{
	FVector2D CardSize = FVector2D(300.f, 420.f);
	FVector2D Centers[3];
	FVector2D RerollCenter = FVector2D::ZeroVector;
	FVector2D RerollSize = FVector2D(170.f, 42.f);
};

bool HPWH_IsUpgradeAllowed(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity);
EHPWH_UpgradeIcon HPWH_UpgradeIconFor(EHPWH_UpgradeType Type);
float HPWH_UpgradeValue(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity);
FString HPWH_UpgradeTitle(EHPWH_UpgradeType Type);
FString HPWH_UpgradeDescription(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity, float Value);
FLinearColor HPWH_RarityColor(EHPWH_Rarity Rarity);
FString HPWH_RarityName(EHPWH_Rarity Rarity);
FHPWH_UpgradeOption HPWH_MakeUpgradeOption(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity);
FHPWH_UpgradeCardLayout HPWH_MakeUpgradeCardLayout(const FVector2D& ViewportSize);
