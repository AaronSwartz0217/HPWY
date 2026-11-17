#include "Progression/CC_GameplayData.h"

int32 UCC_CombatBalance::ResolveCopperReward(FName Id,const USkeletalMesh* Mesh) const
{
 const FCC_CombatProfile* Profile=FindProfile(Id,Mesh);TSet<FName> Seen;
 while(Profile&&!Seen.Contains(Profile->ProfileId))
 {
  Seen.Add(Profile->ProfileId);
  if(Profile->CopperReward>=0)return Profile->CopperReward;
  Profile=Profile->ParentProfileId.IsNone()?nullptr:FindProfile(Profile->ParentProfileId);
 }
 return 0;
}

const FCC_CombatProfile* UCC_CombatBalance::FindProfile(FName Id, const USkeletalMesh* Mesh) const
{
	return Profiles.FindByPredicate([Id, Mesh](const FCC_CombatProfile& P) { return !Id.IsNone() ? P.ProfileId == Id : Mesh && P.Mesh == Mesh; });
}
bool UCC_CombatBalance::Resolve(FName Id, float& Health, float& Attack, const USkeletalMesh* Mesh) const
{
	const FCC_CombatProfile* P = FindProfile(Id, Mesh);
	if (!P || BaseHealth < 1.f || BaseAttack < 0.f || P->HealthMultiplier <= 0.f || P->AttackMultiplier < 0.f) return false;
	Health = BaseHealth; Attack = BaseAttack;
	TSet<FName> Visited;
	while (P)
	{
		if (Visited.Contains(P->ProfileId) || P->HealthMultiplier <= 0.f || P->AttackMultiplier < 0.f) return false;
		Visited.Add(P->ProfileId);
		const float Elite = P->bElite ? FMath::Max(1.f, P->EliteMultiplier) : 1.f;
		Health *= P->HealthMultiplier * Elite; Attack *= P->AttackMultiplier * Elite;
		if (P->ParentProfileId.IsNone()) break;
		P = FindProfile(P->ParentProfileId);
		if (!P) return false;
	}
	return FMath::IsFinite(Health) && FMath::IsFinite(Attack);
}
float UCC_SkillDefinition::CalculatePower(float Attack, int32 Level) const
{
	const float Value = FMath::Max(0.f, Power + (FMath::Max(1, Level) - 1) * PowerPerLevel);
	return PowerMode == ECC_SkillPowerMode::AttackMultiplier ? FMath::Max(0.f, Attack) * Value : Value;
}
FText UCC_SkillDefinition::GetNumericDescription(float Attack, int32 Level) const
{
	if(DesignMechanic!=ECC_DesignMechanic::None) {
  FString Timing=FString::Printf(TEXT("冷却 %.2fs | 法力 %.0f"),Cooldown,ManaCost);
  if(DesignMechanic==ECC_DesignMechanic::Charge)Timing+=FString::Printf(TEXT(" | 最长蓄力 %.1fs"),MaxChargeSeconds);
  else if(Duration>0.f)Timing+=FString::Printf(TEXT(" | 持续 %.1fs"),Duration);
  return FText::FromString(FString::Printf(TEXT("%s · Lv%d\n%s\n%s\n流派解锁：%s"),*DisplayName.ToString(),FMath::Max(1,Level),*Description.ToString(),*Timing,RequiredFormId.IsNone() ? TEXT("通用，默认可用") : *RequiredFormId.ToString()));
 }
 const float Value = CalculatePower(Attack, Level);
	const TCHAR* Names[] = { TEXT("近战"), TEXT("连击（每击）"), TEXT("横扫"), TEXT("突进"), TEXT("粒子远程"), TEXT("散射（每发）"), TEXT("追踪弹"), TEXT("穿透弹"), TEXT("范围爆发"), TEXT("落点打击"), TEXT("持续区域（每跳）"), TEXT("直线打击"), TEXT("自身治疗"), TEXT("范围治疗"), TEXT("护盾"), TEXT("防御加成"), TEXT("攻击加成"), TEXT("移速加成"), TEXT("减速"), TEXT("击退") };
	FString Formula = PowerMode == ECC_SkillPowerMode::AttackMultiplier ? FString::Printf(TEXT("攻击力 × %.1f%% = %.2f"), (Power + (FMath::Max(1, Level)-1)*PowerPerLevel)*100.f, Value) : FString::Printf(TEXT("固定数值 %.2f"), Value);
	if (Behavior == ECC_SkillBehavior::Slow) Formula = FString::Printf(TEXT("移速降低 %.0f%%"), SlowFraction*100.f);
	FString Extra;
	if (Behavior == ECC_SkillBehavior::Combo && !DamageSteps.IsEmpty())
	{
		TArray<FString> Steps;
		float Total = 0.f;
		for (int32 Index = 0; Index < FMath::Clamp(Count, 1, 32); ++Index)
		{
			const float Hit = DamageSteps.IsValidIndex(Index) ? Attack * DamageSteps[Index] : Value;
			Steps.Add(FString::Printf(TEXT("%.2f"), Hit));
			Total += Hit;
		}
		Formula = FString::Printf(TEXT("各击 %s；全中合计 %.2f"), *FString::Join(Steps, TEXT(" / ")), Total);
	}
	else if (Behavior == ECC_SkillBehavior::Combo || Behavior == ECC_SkillBehavior::Spread) Extra += FString::Printf(TEXT("；数量 %d"), Count);
	if (Behavior == ECC_SkillBehavior::Piercing) Extra += FString::Printf(TEXT("；最多命中 %d 个不同目标"), Count);
	if (Behavior == ECC_SkillBehavior::DamageZone) Extra += FString::Printf(TEXT("；每 %.2f 秒一跳"), Interval);
	if (Behavior == ECC_SkillBehavior::Knockback) Extra += FString::Printf(TEXT("；击退 %.0f"), KnockbackForce);
	FString Parameters = FString::Printf(TEXT("冷却 %.2fs | 消耗 %.0f 法力 | 前摇 %.2fs"), Cooldown, ManaCost, Windup);
	const bool bProjectile = Behavior == ECC_SkillBehavior::Projectile || Behavior == ECC_SkillBehavior::Spread || Behavior == ECC_SkillBehavior::Homing || Behavior == ECC_SkillBehavior::Piercing;
	const bool bMelee = Behavior == ECC_SkillBehavior::Melee || Behavior == ECC_SkillBehavior::Combo || Behavior == ECC_SkillBehavior::Sweep || Behavior == ECC_SkillBehavior::Dash || Behavior == ECC_SkillBehavior::LineStrike;
	const bool bGround = Behavior == ECC_SkillBehavior::GroundStrike || Behavior == ECC_SkillBehavior::DamageZone;
	if (bProjectile) Parameters += FString::Printf(TEXT("\n射程 %.0f | 弹速 %.0f"), Range, ProjectileSpeed);
	if (bMelee || bGround) Parameters += FString::Printf(TEXT("\n距离 %.0f"), Range);
	if (bMelee && Behavior != ECC_SkillBehavior::LineStrike) Parameters += FString::Printf(TEXT(" | 扇形 %.0f°"), Angle);
	if (Behavior == ECC_SkillBehavior::LineStrike) Parameters += FString::Printf(TEXT(" | 通道宽度 %.0f"), Radius * 2.f);
	if (bGround || Behavior == ECC_SkillBehavior::Burst || Behavior == ECC_SkillBehavior::AreaHeal || Behavior == ECC_SkillBehavior::Slow || Behavior == ECC_SkillBehavior::Knockback) Parameters += FString::Printf(TEXT(" | 范围半径 %.0f"), Radius);
	if (Behavior == ECC_SkillBehavior::DamageZone || Behavior == ECC_SkillBehavior::Shield || Behavior == ECC_SkillBehavior::DefenseBuff || Behavior == ECC_SkillBehavior::AttackBuff || Behavior == ECC_SkillBehavior::SpeedBuff || Behavior == ECC_SkillBehavior::Slow) Parameters += FString::Printf(TEXT(" | 持续 %.1fs"), Duration);
	if (Behavior == ECC_SkillBehavior::GroundStrike) Parameters += FString::Printf(TEXT(" | 落点额外延迟 %.2fs"), Interval);
	return FText::FromString(FString::Printf(TEXT("%s · Lv%d\n%s\n%s：%s%s\n%s\n伤害为减伤前数值；防御及护盾在命中时结算。"), *DisplayName.ToString(), FMath::Max(1, Level), *Description.ToString(), Names[FMath::Clamp(int32(Behavior),0,19)], *Formula, *Extra, *Parameters));
}

bool UCC_SkillLoadout::IsComplete() const
{
	if (Slots.Num() != 4) return false;
	TSet<FName> Ids;
	for (const FCC_SkillSlot& Slot : Slots)
	{
		if (!IsValid(Slot.Skill) || Slot.Skill->SkillId.IsNone() || Slot.Level < 1 || Ids.Contains(Slot.Skill->SkillId)) return false;
		Ids.Add(Slot.Skill->SkillId);
	}
	return true;
}

FText UCC_FormDefinition::GetInventoryName() const
{
 if (!InventoryName.IsEmpty()) return InventoryName;
 return FormId==TEXT("InitialInk") ? FText::FromString(TEXT("初墨之力")) : DisplayName;
}
bool UCC_FormDefinition::IsComplete() const
{
	if (FormId.IsNone() || Stages.Num() != 3) return false;
	for (const FCC_FormStage& Stage : Stages)
	{
		if (!IsValid(Stage.Loadout) || !Stage.Loadout->IsComplete() || Stage.Cost < 0 || Stage.MaxHealth < 1.f || Stage.MaxMana < 0.f || Stage.MoveSpeed < 1.f) return false;
		float Health = Stage.MaxHealth, Attack = Stage.Attack;
		if (Stage.CombatBalance && !Stage.CombatBalance->Resolve(Stage.StatProfileId, Health, Attack)) return false;
		if (!FMath::IsFinite(Health) || !FMath::IsFinite(Attack) || Health < 1.f || Attack < 0.f) return false;
	}
	return true;
}

int32 UCC_CombatBalance::ResolveExperienceReward(FName Id, const USkeletalMesh* Mesh) const
{
 const FCC_CombatProfile* P=FindProfile(Id,Mesh); TSet<FName> Seen;
 while(P && !Seen.Contains(P->ProfileId)) {
  Seen.Add(P->ProfileId);
  if(P->ExperienceReward>=0) return P->ExperienceReward;
  if(P->bElite) return 500;
  P=P->ParentProfileId.IsNone() ? nullptr : FindProfile(P->ParentProfileId);
 }
 return 10;
}
