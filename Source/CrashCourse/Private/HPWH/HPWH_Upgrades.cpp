#include "HPWH/HPWH_Upgrades.h"

namespace
{
	float RarityIndex(EHPWH_Rarity Rarity)
	{
		switch (Rarity)
		{
		case EHPWH_Rarity::Epic: return 1.f;
		case EHPWH_Rarity::Legendary: return 2.f;
		default: return 0.f;
		}
	}
}

bool HPWH_IsUpgradeAllowed(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity)
{
	switch (Type)
	{
	case EHPWH_UpgradeType::MoveSpeed:
		return Rarity != EHPWH_Rarity::Legendary;
	case EHPWH_UpgradeType::SoulGuard:
	case EHPWH_UpgradeType::SuccessRate:
		return Rarity == EHPWH_Rarity::Legendary;
	case EHPWH_UpgradeType::MultiShotTwo:
		return Rarity == EHPWH_Rarity::Epic;
	case EHPWH_UpgradeType::MultiShotThree:
		return Rarity == EHPWH_Rarity::Legendary;
	default:
		return true;
	}
}

EHPWH_UpgradeIcon HPWH_UpgradeIconFor(EHPWH_UpgradeType Type)
{
	switch (Type)
	{
	case EHPWH_UpgradeType::AttackDamage:
	case EHPWH_UpgradeType::BulletCooldown:
	case EHPWH_UpgradeType::MultiShotTwo:
	case EHPWH_UpgradeType::MultiShotThree:
		return EHPWH_UpgradeIcon::Attack;
	case EHPWH_UpgradeType::MaxHealth:
	case EHPWH_UpgradeType::DamageTolerance:
	case EHPWH_UpgradeType::KillHeal:
		return EHPWH_UpgradeIcon::Life;
	case EHPWH_UpgradeType::AttackRange:
	case EHPWH_UpgradeType::InteractRange:
		return EHPWH_UpgradeIcon::Range;
	case EHPWH_UpgradeType::MoveSpeed:
	case EHPWH_UpgradeType::SoulGuard:
	case EHPWH_UpgradeType::SoulSpeed:
	case EHPWH_UpgradeType::ReturnRange:
		return EHPWH_UpgradeIcon::Soul;
	default:
		return EHPWH_UpgradeIcon::Fate;
	}
}

float HPWH_UpgradeValue(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity)
{
	const int32 Tier = FMath::Clamp(FMath::RoundToInt(RarityIndex(Rarity)), 0, 2);
	switch (Type)
	{
	case EHPWH_UpgradeType::AttackDamage:
		return Tier == 0 ? 2.f : (Tier == 1 ? 5.f : 10.f);
	case EHPWH_UpgradeType::MaxHealth:
		return Tier == 0 ? 10.f : (Tier == 1 ? 20.f : 40.f);
	case EHPWH_UpgradeType::MoveSpeed:
		return Tier == 1 ? 0.30f : 0.10f;
	case EHPWH_UpgradeType::AttackRange:
		// 远击：全稀有度 ×2
		return Tier == 0 ? 0.14f : (Tier == 1 ? 0.36f : 0.60f);
	case EHPWH_UpgradeType::BulletCooldown:
		// 速射：全稀有度 ×4
		return Tier == 0 ? 0.20f : (Tier == 1 ? 0.48f : 0.80f);
	case EHPWH_UpgradeType::MultiShotTwo:
		return 2.f;
	case EHPWH_UpgradeType::MultiShotThree:
		return 3.f;
	case EHPWH_UpgradeType::SoulGuard:
		return 1.f;
	case EHPWH_UpgradeType::SoulSpeed:
		// 魂行：全稀有度 ×3
		return Tier == 0 ? 0.15f : (Tier == 1 ? 0.30f : 0.60f);
	case EHPWH_UpgradeType::ReturnRange:
		// 牵引：全稀有度 ×2
		return Tier == 0 ? 0.40f : (Tier == 1 ? 1.00f : 2.00f);
	case EHPWH_UpgradeType::DamageTolerance:
		return Tier == 0 ? 0.2f : (Tier == 1 ? 0.5f : 1.0f);
	case EHPWH_UpgradeType::KillHeal:
		return Tier == 0 ? 1.f : (Tier == 1 ? 2.f : 4.f);
	case EHPWH_UpgradeType::CrawlerDistance:
		return Tier == 0 ? 2.f : (Tier == 1 ? 5.f : 10.f);
	case EHPWH_UpgradeType::CrawlerSlow:
		// 滞影：全稀有度 ×2
		return Tier == 0 ? 0.10f : (Tier == 1 ? 0.24f : 0.60f);
	case EHPWH_UpgradeType::PuzzleTime:
		// 时限：全稀有度 ×2，并额外 +5 秒
		return Tier == 0 ? 7.f : (Tier == 1 ? 11.f : 19.f);
	case EHPWH_UpgradeType::SuccessRate:
		return 0.05f;
	case EHPWH_UpgradeType::InteractRange:
		return Tier == 0 ? 0.10f : (Tier == 1 ? 0.25f : 0.50f);
	}
	return 0.f;
}

FString HPWH_UpgradeTitle(EHPWH_UpgradeType Type)
{
	switch (Type)
	{
	case EHPWH_UpgradeType::AttackDamage: return TEXT("锐意");
	case EHPWH_UpgradeType::MaxHealth: return TEXT("坚韧");
	case EHPWH_UpgradeType::MoveSpeed: return TEXT("疾行");
	case EHPWH_UpgradeType::AttackRange: return TEXT("远击");
	case EHPWH_UpgradeType::BulletCooldown: return TEXT("速射");
	case EHPWH_UpgradeType::MultiShotTwo: return TEXT("双发");
	case EHPWH_UpgradeType::MultiShotThree: return TEXT("三连");
	case EHPWH_UpgradeType::SoulGuard: return TEXT("魂壳");
	case EHPWH_UpgradeType::SoulSpeed: return TEXT("魂行");
	case EHPWH_UpgradeType::ReturnRange: return TEXT("牵引");
	case EHPWH_UpgradeType::DamageTolerance: return TEXT("定神");
	case EHPWH_UpgradeType::KillHeal: return TEXT("噬魂");
	case EHPWH_UpgradeType::CrawlerDistance: return TEXT("驱影");
	case EHPWH_UpgradeType::CrawlerSlow: return TEXT("滞影");
	case EHPWH_UpgradeType::PuzzleTime: return TEXT("时限");
	case EHPWH_UpgradeType::SuccessRate: return TEXT("窥生");
	case EHPWH_UpgradeType::InteractRange: return TEXT("探物");
	}
	return TEXT("未知");
}

FString HPWH_UpgradeDescription(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity, float Value)
{
	switch (Type)
	{
	case EHPWH_UpgradeType::AttackDamage:
		return FString::Printf(TEXT("攻击力 +%.0f"), Value);
	case EHPWH_UpgradeType::MaxHealth:
		return FString::Printf(TEXT("生命上限 +%.0f\n并回复等量生命"), Value);
	case EHPWH_UpgradeType::MoveSpeed:
		return FString::Printf(TEXT("移动速度 +%.0f%%"), Value * 100.f);
	case EHPWH_UpgradeType::AttackRange:
		return FString::Printf(TEXT("攻击距离 +%.0f%%"), Value * 100.f);
	case EHPWH_UpgradeType::BulletCooldown:
		return FString::Printf(TEXT("子弹冷却 -%.2f 秒"), Value);
	case EHPWH_UpgradeType::MultiShotTwo:
		return TEXT("一次发射 2 颗子弹");
	case EHPWH_UpgradeType::MultiShotThree:
		return TEXT("一次发射 3 颗子弹");
	case EHPWH_UpgradeType::SoulGuard:
		return TEXT("灵魂离体需要\n被攻击 2 次");
	case EHPWH_UpgradeType::SoulSpeed:
		return FString::Printf(TEXT("灵魂速度 +%.0f%%"), Value * 100.f);
	case EHPWH_UpgradeType::ReturnRange:
		return FString::Printf(TEXT("回体判定范围 +%.0f%%"), Value * 100.f);
	case EHPWH_UpgradeType::DamageTolerance:
		return FString::Printf(TEXT("受击后无敌时间\n+%.1f 秒"), Value);
	case EHPWH_UpgradeType::KillHeal:
		return FString::Printf(TEXT("击杀怪物回复\n%.0f 点生命"), Value);
	case EHPWH_UpgradeType::CrawlerDistance:
		return FString::Printf(TEXT("徘徊者重新出现时\n远离你 %.0f 格"), Value);
	case EHPWH_UpgradeType::CrawlerSlow:
		return FString::Printf(TEXT("徘徊者速度 -%.0f%%"), Value * 100.f);
	case EHPWH_UpgradeType::PuzzleTime:
		return FString::Printf(TEXT("解密时限 +%.0f 秒"), Value);
	case EHPWH_UpgradeType::SuccessRate:
		return TEXT("通关率 +5%");
	case EHPWH_UpgradeType::InteractRange:
		return FString::Printf(TEXT("交互范围 +%.0f%%"), Value * 100.f);
	}
	return FString();
}

FLinearColor HPWH_RarityColor(EHPWH_Rarity Rarity)
{
	switch (Rarity)
	{
	case EHPWH_Rarity::Epic:
		return FLinearColor(0.57f, 0.33f, 1.f, 1.f);
	case EHPWH_Rarity::Legendary:
		return FLinearColor(0.760f, 0.630f, 0.390f, 1.f);
	default:
		return FLinearColor(0.205f, 0.448f, 1.f, 1.f);
	}
}

FString HPWH_RarityName(EHPWH_Rarity Rarity)
{
	switch (Rarity)
	{
	case EHPWH_Rarity::Epic: return TEXT("中级");
	case EHPWH_Rarity::Legendary: return TEXT("极品");
	default: return TEXT("普通");
	}
}

FHPWH_UpgradeOption HPWH_MakeUpgradeOption(EHPWH_UpgradeType Type, EHPWH_Rarity Rarity)
{
	FHPWH_UpgradeOption Option;
	Option.Type = Type;
	Option.Rarity = Rarity;
	Option.Value = HPWH_UpgradeValue(Type, Rarity);
	Option.Title = HPWH_UpgradeTitle(Type);
	Option.Description = HPWH_UpgradeDescription(Type, Rarity, Option.Value);
	return Option;
}

FHPWH_UpgradeCardLayout HPWH_MakeUpgradeCardLayout(const FVector2D& ViewportSize)
{
	FHPWH_UpgradeCardLayout Layout;
	const float MinSide = FMath::Min(ViewportSize.X, ViewportSize.Y);
	const float CardWidth = FMath::Clamp(MinSide * 0.34f, 280.f, 420.f);
	const float CardHeight = FMath::Clamp(CardWidth * 1.42f, 400.f, 600.f);
	Layout.CardSize = FVector2D(CardWidth, CardHeight);

	const float Gap = CardWidth * 0.16f;
	const float TotalWidth = CardWidth * 3.f + Gap * 2.f;
	const float StartX = ViewportSize.X * 0.5f - TotalWidth * 0.5f + CardWidth * 0.5f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Layout.Centers[Index] = FVector2D(StartX + Index * (CardWidth + Gap), ViewportSize.Y * 0.53f);
	}
	Layout.RerollCenter = FVector2D(ViewportSize.X * 0.5f,
		ViewportSize.Y * 0.53f + CardHeight * 0.5f + 74.f);
	return Layout;
}
