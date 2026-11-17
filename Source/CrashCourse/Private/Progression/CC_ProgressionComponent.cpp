#include "Progression/CC_ProgressionComponent.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Player/CC_PlayerController.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Progression/CC_ExperienceOrb.h"
#include "Progression/CC_SkillProjectile.h"
#include "World/CC_AdventureGameInstance.h"

UCC_ProgressionComponent::UCC_ProgressionComponent() { SetIsReplicatedByDefault(true); }
void UCC_ProgressionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CurrentForm);
	DOREPLIFETIME(ThisClass, CurrentTier);
	DOREPLIFETIME_CONDITION(ThisClass, Experience, COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass, CopperCoins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, NativeTier, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, Forms, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, Upgrades, COND_OwnerOnly);
 DOREPLIFETIME(ThisClass, bChromaticSuppressed);
 DOREPLIFETIME(ThisClass, bChromaticPreview);
 DOREPLIFETIME(ThisClass, bChromaticUnlocked);
	DOREPLIFETIME_CONDITION(ThisClass, ManualSkills, COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass,ItemStacks,COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass,EquippedItem,COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass,ItemCooldownEndsAt,COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass,bRemoteShopUnlocked,COND_OwnerOnly);
 DOREPLIFETIME_CONDITION(ThisClass,ActiveTownShop,COND_OwnerOnly);
}
ACC_PlayerCharacter* UCC_ProgressionComponent::GetCharacter() const
{
	const ACC_PlayerState* PS = Cast<ACC_PlayerState>(GetOwner());
	return PS ? Cast<ACC_PlayerCharacter>(PS->GetPawn()) : nullptr;
}
bool UCC_ProgressionComponent::IsReady() const
{
	const ACC_PlayerCharacter* Character = GetCharacter();
	return Character && Character->IsAlive() && Character->GetAbilitySystemComponent() && CachedAvatar == Character;
}
UCC_FormDefinition* UCC_ProgressionComponent::ResolveForm() const { return CurrentForm ? CurrentForm.Get() : NativeForm.Get(); }
void UCC_ProgressionComponent::Notify() { bChromaticUnlocked=HasChromaticUnlock(); if (auto* C=GetCharacter()) C->RefreshChromaticAppearance(); GetOwner()->ForceNetUpdate(); OnChanged.Broadcast(); }
void UCC_ProgressionComponent::OnRep_State() { RefreshAppearance(); OnChanged.Broadcast(); }
void UCC_ProgressionComponent::ClientResult_Implementation(ECC_PurchaseResult Result) { OnPurchaseFinished.Broadcast(Result); }
void UCC_ProgressionComponent::AddExperience(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || Amount <= 0) return;
	Experience = static_cast<int32>(FMath::Min<int64>(MAX_int32, static_cast<int64>(Experience) + Amount));
	RefreshExperienceStats();
	Notify();
}
void UCC_ProgressionComponent::RestoreExperienceFromBackend(int32 SavedExperience)
{
	if (!GetOwner()->HasAuthority() || SavedExperience < 0) return;
	Experience = SavedExperience;
	RefreshExperienceStats();
	Notify();
}
void UCC_ProgressionComponent::InitializeAvatar()
{
	ACC_PlayerCharacter* Character = GetCharacter();
	if (!Character || !Character->GetAttributeSet()) return;
	if (CachedAvatar != Character)
	{
  ClearConsumableEffects();
		CachedAvatar = Character;
		HumanMesh = Character->GetMesh()->GetSkeletalMeshAsset();
		HumanMeshScale = Character->GetMesh()->GetRelativeScale3D();
		HumanAnimClass = Character->GetMesh()->GetAnimClass();
		const UCC_AttributeSet* AS = Cast<UCC_AttributeSet>(Character->GetAttributeSet());
		if (!AS) return;
		HumanStats.MaxHealth = AS->GetMaxHealth();
		HumanStats.MaxMana = AS->GetMaxMana();
		HumanStats.Attack = AS->GetAttack();
		HumanStats.Defense = AS->GetDefense();
		HumanStats.MoveSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
	}
	if (GetOwner()->HasAuthority())
	{
		if (NativeForm && !NativeForm->bCanEvolve) NativeTier = 0;
		if (CurrentForm && !CurrentForm->bAvailable) { CurrentForm = nullptr; CurrentTier = NativeTier; }
		if (ResolveForm() && !ResolveForm()->bCanEvolve) CurrentTier = 0;
		ApplyState(true);
		if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())
			if (Adventure->RestoreSessionProgress(this) && GetWorld()->HasBegunPlay()) Adventure->HandleRegionEntered(GetWorld());
	}
	else RefreshAppearance();
}
void UCC_ProgressionComponent::RefreshAppearance()
{
 if (GetOwner()->HasAuthority()) bChromaticUnlocked=HasChromaticUnlock();
	ACC_PlayerCharacter* Character = GetCharacter();
	if (!Character || CachedAvatar != Character) return;
	UCC_FormDefinition* Definition = ResolveForm();
	const FCC_FormStage* Stage = Definition && Definition->Stages.IsValidIndex(CurrentTier) ? &Definition->Stages[CurrentTier] : nullptr;
	USkeletalMesh* Mesh = Stage && Stage->Mesh ? Stage->Mesh.Get() : HumanMesh.Get();
	const TSubclassOf<UAnimInstance> Anim = Stage && Stage->Mesh && Stage->Mesh != HumanMesh ? Stage->AnimClass : (Stage && Stage->AnimClass ? Stage->AnimClass : HumanAnimClass);
	// An explicitly configured form mesh owns its scale; the starting pawn's
	// art scale must not be multiplied into every later transformation.
	Character->GetMesh()->SetRelativeScale3D(Stage && Stage->Mesh ? Stage->MeshScale : HumanMeshScale * (Stage ? Stage->MeshScale : FVector::OneVector));
	if (Character->GetMesh()->GetSkeletalMeshAsset() != Mesh) { Character->ClearChromaticAppearance(); Character->GetMesh()->SetSkeletalMesh(Mesh); }
	if (Character->GetMesh()->GetAnimClass() != Anim) Character->GetMesh()->SetAnimInstanceClass(Anim);
    if (Character->Locomotion) Character->Locomotion->AlignMeshToGround();
 Character->RefreshChromaticAppearance();
}
bool UCC_ProgressionComponent::ApplyState(bool bReplaceSkills)
{
	if (!IsReady()) return false;
	ACC_PlayerCharacter* Character = GetCharacter();
	UCC_FormDefinition* Definition = ResolveForm();
	if (Definition && !Definition->IsComplete()) return false;
	FCC_FormStage Stats = Definition ? Definition->Stages[FMath::Clamp(CurrentTier, 0, 2)] : HumanStats;
	if (Stats.CombatBalance && !Stats.CombatBalance->Resolve(Stats.StatProfileId, Stats.MaxHealth, Stats.Attack)) return false;
	if (bReplaceSkills && !Character->Skills->ApplyLoadout(Definition ? Stats.Loadout.Get() : Character->Skills->DefaultLoadout.Get())) return false;
	for (const FCC_UpgradeProgress& Progress : Upgrades)
	{
		if (!Progress.Upgrade) continue;
		const float Bonus = Progress.Level * Progress.Upgrade->BonusPerLevel;
		switch (Progress.Upgrade->Type)
		{
		case ECC_UpgradeType::MaxHealth: Stats.MaxHealth += Bonus; break;
		case ECC_UpgradeType::MaxMana: Stats.MaxMana += Bonus; break;
		case ECC_UpgradeType::Attack: Stats.Attack += Bonus; break;
		case ECC_UpgradeType::Defense: Stats.Defense += Bonus; break;
		case ECC_UpgradeType::MoveSpeed: Stats.MoveSpeed += Bonus; break;
		default: break;
		}
	}
	ApplyExperienceGrowth(Definition,Stats);
	UCC_AttributeSet* AS = Cast<UCC_AttributeSet>(Character->GetAttributeSet());
	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	const float HealthRatio = AS->GetMaxHealth() > 0.f ? FMath::Clamp(AS->GetHealth() / AS->GetMaxHealth(), 0.f, 1.f) : 1.f;
	const float ManaRatio = AS->GetMaxMana() > 0.f ? FMath::Clamp(AS->GetMana() / AS->GetMaxMana(), 0.f, 1.f) : 0.f;
	ASC->SetNumericAttributeBase(AS->GetMaxHealthAttribute(), FMath::Max(1.f, Stats.MaxHealth));
	ASC->SetNumericAttributeBase(AS->GetMaxManaAttribute(), FMath::Max(0.f, Stats.MaxMana));
	ASC->SetNumericAttributeBase(AS->GetAttackAttribute(), FMath::Max(0.f, Stats.Attack));
	ASC->SetNumericAttributeBase(AS->GetDefenseAttribute(), FMath::Max(0.f, Stats.Defense));
	ASC->SetNumericAttributeBase(AS->GetMoveSpeedAttribute(), FMath::Max(1.f, Stats.MoveSpeed));
	ASC->SetNumericAttributeBase(AS->GetHealthAttribute(), AS->GetMaxHealth() * HealthRatio);
	ASC->SetNumericAttributeBase(AS->GetManaAttribute(), AS->GetMaxMana() * ManaRatio);
	Character->Skills->RefreshLevels();
	RefreshAppearance();
	return true;
}
void UCC_ProgressionComponent::Interact_Implementation(AActor* ArtifactActor)
{
	if (!IsReady() || !IsValid(ArtifactActor)) { ClientResult(ECC_PurchaseResult::InvalidState); return; }
	UCC_ArtifactComponent* Artifact = ArtifactActor->FindComponentByClass<UCC_ArtifactComponent>();
	if (!Artifact || !Artifact->CanInteract(GetCharacter()) || !ArtifactForms.Contains(Artifact->Form)) { ClientResult(ECC_PurchaseResult::InvalidConfiguration); return; }
	// E releases the physical reward; inventory and experience change on contact.
	ClientResult(Artifact->ReleasePower(GetCharacter()) ? ECC_PurchaseResult::Success : ECC_PurchaseResult::InvalidState);
}
bool UCC_ProgressionComponent::CollectArtifactPower(UCC_FormDefinition* Form,int32 ExperienceReward)
{
	if (!GetOwner()->HasAuthority() || !IsReady() || !IsValid(Form) || !ArtifactForms.Contains(Form) || !Form->bAvailable || !Form->IsComplete() || ExperienceReward<0) return false;
 const bool bAlreadyChromatic=HasChromaticUnlock();
	if (!Forms.ContainsByPredicate([Form](const FCC_FormProgress& P) { return P.Form==Form; }))
	{
		FCC_FormProgress Progress; Progress.Form=Form; Forms.Add(Progress);
	}
	// Form and wallet are committed together; repeat tokens keep the existing tier.
 if (!bAlreadyChromatic && HasChromaticUnlock()) {bChromaticSuppressed=false;bChromaticPreview=false;}
	Experience=int32(FMath::Min<int64>(MAX_int32,int64(Experience)+ExperienceReward));
	RefreshExperienceStats();
	Notify();
	return true;
}
void UCC_ProgressionComponent::SwitchForm_Implementation(UCC_FormDefinition* Form)
{
	if (!IsReady()) { ClientResult(ECC_PurchaseResult::InvalidState); return; }
	int32 Tier = NativeTier;
	if (Form)
	{
		const FCC_FormProgress* P = Forms.FindByPredicate([Form](const FCC_FormProgress& Item) { return Item.Form == Form; });
		if (!Form->bAvailable || !ArtifactForms.Contains(Form) || !P) { ClientResult(ECC_PurchaseResult::NotUnlocked); return; }
		Tier = P->Tier;
	}
	UCC_FormDefinition* Definition = Form ? Form : NativeForm.Get();
	if (Definition && !Definition->IsComplete()) { ClientResult(ECC_PurchaseResult::InvalidConfiguration); return; }
	CurrentForm = Form; CurrentTier = Definition && !Definition->bCanEvolve ? 0 : Tier;
	ApplyState(true); Notify(); ClientResult(ECC_PurchaseResult::Success);
}
int32 UCC_ProgressionComponent::GetEvolutionCost() const
{
	return GetFormEvolutionCost(ResolveForm());
}
int32 UCC_ProgressionComponent::GetOwnedFormTier(UCC_FormDefinition* Form) const
{
 UCC_FormDefinition* Definition=Form ? Form : NativeForm.Get();
 if (!Definition || !Definition->bAvailable || !Definition->IsComplete()) return INDEX_NONE;
 if (Definition==NativeForm) return NativeTier>=0 && NativeTier<3 ? NativeTier : INDEX_NONE;
 if (!ArtifactForms.Contains(Definition)) return INDEX_NONE;
 const FCC_FormProgress* Owned=Forms.FindByPredicate([Definition](const FCC_FormProgress& Item){return Item.Form==Definition;});
 return Owned && Owned->Tier>=0 && Owned->Tier<3 ? Owned->Tier : INDEX_NONE;
}
int32 UCC_ProgressionComponent::GetFormEvolutionCost(UCC_FormDefinition* Form) const
{
 UCC_FormDefinition* Definition=Form ? Form : NativeForm.Get();const int32 Tier=GetOwnedFormTier(Definition);
 return Definition && Definition->bCanEvolve && Tier>=0 && Tier<2 ? Definition->Stages[Tier+1].Cost : -1;
}
bool UCC_ProgressionComponent::CanEvolveCurrentForm() const { return GetEvolutionCost() >= 0; }
void UCC_ProgressionComponent::Evolve_Implementation()
{
 EvolveForm_Implementation(CurrentForm);
}
void UCC_ProgressionComponent::EvolveForm_Implementation(UCC_FormDefinition* Form)
{
	if (!IsReady()) { ClientResult(ECC_PurchaseResult::InvalidState); return; }
 UCC_FormDefinition* Definition=Form ? Form : NativeForm.Get();
 const int32 Tier=GetOwnedFormTier(Definition);
 if (Tier<0) {ClientResult(ECC_PurchaseResult::NotUnlocked);return;}
 if (!Definition->bCanEvolve) {ClientResult(ECC_PurchaseResult::NotUnlocked);return;}
 if (Tier>=2) {ClientResult(ECC_PurchaseResult::MaxLevel);return;}
	const int32 Cost = GetFormEvolutionCost(Definition);
	if (Cost < 0) { ClientResult(ECC_PurchaseResult::InvalidConfiguration); return; }
	if (Experience < Cost) { ClientResult(ECC_PurchaseResult::InsufficientExperience); return; }
	Experience -= Cost;
 if (Definition==NativeForm) NativeTier=Tier+1;
 else for (FCC_FormProgress& P:Forms) if (P.Form==Definition) P.Tier=Tier+1;
 if (Definition==ResolveForm()) {CurrentTier=Tier+1;ApplyState(true);}
 else RefreshExperienceStats();
 Notify();ClientResult(ECC_PurchaseResult::Success);
}
int32 UCC_ProgressionComponent::GetUpgradeLevel(UCC_UpgradeDefinition* Upgrade) const
{
	const FCC_UpgradeProgress* P = Upgrades.FindByPredicate([Upgrade](const FCC_UpgradeProgress& Item) { return Item.Upgrade == Upgrade; });
	return P ? P->Level : 0;
}
int32 UCC_ProgressionComponent::GetSkillBonus(FName SkillId) const
{
	int32 Total = 0;
	for (const FCC_UpgradeProgress& P : Upgrades)
		if (P.Upgrade && P.Upgrade->Type == ECC_UpgradeType::SkillLevel && P.Upgrade->SkillId == SkillId) Total += P.Level;
	return Total;
}
void UCC_ProgressionComponent::Purchase_Implementation(UCC_UpgradeDefinition* Upgrade)
{
	if (!IsReady()) { ClientResult(ECC_PurchaseResult::InvalidState); return; }
	if (!IsValid(Upgrade) || !UpgradeCatalog.Contains(Upgrade) || Upgrade->UpgradeId.IsNone() || Upgrade->BonusPerLevel < 0.f || (Upgrade->Type == ECC_UpgradeType::SkillLevel && Upgrade->SkillId.IsNone()) || (ResolveForm() && !ResolveForm()->IsComplete())) { ClientResult(ECC_PurchaseResult::InvalidConfiguration); return; }
 if(!CanAccessTownShop(CCTownShops::ForUpgrade(Upgrade))){ClientResult(ECC_PurchaseResult::NotUnlocked);return;}
	const int32 Level = GetUpgradeLevel(Upgrade);
	if (!Upgrade->Prices.IsValidIndex(Level)) { ClientResult(ECC_PurchaseResult::MaxLevel); return; }
	const int32 Cost = Upgrade->Prices[Level];
	if (Cost < 0) { ClientResult(ECC_PurchaseResult::InvalidConfiguration); return; }
	if (Experience < Cost) { ClientResult(ECC_PurchaseResult::InsufficientExperience); return; }
	FCC_UpgradeProgress* P = Upgrades.FindByPredicate([Upgrade](const FCC_UpgradeProgress& Item) { return Item.Upgrade == Upgrade; });
	if (!P) { FCC_UpgradeProgress New; New.Upgrade = Upgrade; Upgrades.Add(New); P = &Upgrades.Last(); }
	Experience -= Cost; ++P->Level;
	ApplyState(false); Notify(); ClientResult(ECC_PurchaseResult::Success);
 PersistTownPurchase();
}
bool UCC_ProgressionComponent::SaveProgress(const FString& Slot)
{
	if (!GetOwner()->HasAuthority() || GetWorld()->GetNetMode() != NM_Standalone) return false;
	auto* Save=CreateSnapshot();
	const bool Saved=Save && UGameplayStatics::SaveGameToSlot(Save,Slot,0);
	if (Saved && Slot==TEXT("SpiritTales")) if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>()) {Adventure->AdoptProgressSnapshot(Save);Adventure->ResumeAutomaticSaves();}
	return Saved;
}
UCC_ProgressionSave* UCC_ProgressionComponent::CreateSnapshot() const
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return nullptr;
	UCC_ProgressionSave* Save = Cast<UCC_ProgressionSave>(UGameplayStatics::CreateSaveGameObject(UCC_ProgressionSave::StaticClass()));
	if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>()) Save->VisitedRegions=Adventure->VisitedRegions;
	for (const UCC_SkillDefinition* Skill : ManualSkills) if (Skill) Save->ManualSkillIds.Add(Skill->SkillId);
	Save->NativeFormId = NativeForm ? NativeForm->FormId : NAME_None;
	Save->CopperCoins = CopperCoins;
 Save->bChromaticSuppressed=bChromaticSuppressed;
 Save->bRemoteShopUnlocked=bRemoteShopUnlocked;
 Save->bDeveloperInventory=bDeveloperInventory;
 for(const auto& Stack:ItemStacks)if(Stack.Item && Stack.Count>0)Save->ConsumableCounts.Add(Stack.Item->ItemId,Stack.Count);
 Save->EquippedConsumableId=EquippedItem?EquippedItem->ItemId:NAME_None;
 Save->ItemCooldownRemaining=GetItemCooldownRemaining();Save->ItemCooldownSavedUtcTicks=FDateTime::UtcNow().GetTicks();
 Save->Experience = Experience; Save->NativeTier = NativeTier;
	Save->SelectedForm = CurrentForm ? CurrentForm->FormId : NAME_None;
	for (const FCC_FormProgress& P : Forms) if (P.Form) Save->FormTiers.Add(P.Form->FormId, P.Tier);
	for (const FCC_UpgradeProgress& P : Upgrades) if (P.Upgrade) Save->UpgradeLevels.Add(P.Upgrade->UpgradeId, P.Level);
	return Save;
}
bool UCC_ProgressionComponent::LoadProgress(const FString& Slot)
{
	if (!CanConfigureSkills() || !GetOwner()->HasAuthority() || GetWorld()->GetNetMode() != NM_Standalone) return false;
	const UCC_ProgressionSave* Save = Cast<UCC_ProgressionSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	const bool Loaded=RestoreSnapshot(Save);
	if (Loaded && Slot==TEXT("SpiritTales")) if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>()) {Adventure->AdoptProgressSnapshot(Save);Adventure->ResumeAutomaticSaves();Adventure->HandleRegionEntered(GetWorld());}
	return Loaded;
}
bool UCC_ProgressionComponent::RestoreSnapshot(const UCC_ProgressionSave* Save)
{
	if (!CanConfigureSkills() || !GetOwner()->HasAuthority() || GetWorld()->GetNetMode()!=NM_Standalone || !Save || Save->Version<1 || Save->Version>4 || (NativeForm && !NativeForm->IsComplete())) return false;
	// v1 used Native for the always-unlocked Nature route. Preserve that route
	// when InitialInk becomes the starting form instead of applying its tier to ink.
	const bool bMigrateLegacyNature = Save->Version == 1 && NativeForm && NativeForm->FormId == TEXT("InitialInk");
	UCC_FormDefinition* LegacyNature = nullptr;
	if (bMigrateLegacyNature)
	{
		for (UCC_FormDefinition* Form : ArtifactForms)
			if (Form && Form->FormId == TEXT("Native") && Form->bAvailable && Form->IsComplete()) { LegacyNature = Form; break; }
		// Refuse a partially configured migration before touching the current wallet.
		if (!LegacyNature) return false;
	}
	Forms.Empty(); Upgrades.Empty(); CurrentForm = nullptr; ManualSkills.Empty();
 bChromaticSuppressed=Save->bChromaticSuppressed;bChromaticPreview=false;
 bRemoteShopUnlocked=Save->bRemoteShopUnlocked;ActiveTownShop=nullptr;
 ClearConsumableEffects();ItemStacks.Empty();EquippedItem=nullptr;
 bDeveloperInventory=false;
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 bDeveloperInventory=Save->bDeveloperInventory;
#endif
 for(UCC_ConsumableDefinition* Item:ItemCatalog)if(IsCatalogItem(Item))
 {
  if(const int32* Count=Save->ConsumableCounts.Find(Item->ItemId);Count&&*Count>0){FCC_ConsumableStack Stack;Stack.Item=Item;Stack.Count=FMath::Clamp(*Count,0,GetItemStackLimit(Item));ItemStacks.Add(Stack);}
  if(Item->ItemId==Save->EquippedConsumableId)EquippedItem=Item;
 }
 const int64 UtcNow=FDateTime::UtcNow().GetTicks();
 const double Elapsed=Save->ItemCooldownSavedUtcTicks>0&&Save->ItemCooldownSavedUtcTicks<=UtcNow?(double(UtcNow)-double(Save->ItemCooldownSavedUtcTicks))/ETimespan::TicksPerSecond:0.;
 const double Remaining=FMath::IsFinite(Save->ItemCooldownRemaining)?FMath::Clamp(double(Save->ItemCooldownRemaining)-Elapsed,0.,double(FMath::Max(15.f,ItemCooldownSeconds))):0.;
 ItemCooldownEndsAt=GetItemClock()+Remaining;
 if (Save->ManualSkillIds.Num() == 4) {
  TSet<FName> Seen;
  for (FName Id : Save->ManualSkillIds) {
   const auto* Found = SkillCatalog.FindByPredicate([Id](const UCC_SkillDefinition* S) { return S && S->SkillId == Id; });
   if (!Found || Seen.Contains(Id)) { ManualSkills.Empty(); break; }
   ManualSkills.Add(*Found); Seen.Add(Id);
  }
 }
	Experience = FMath::Max(0, Save->Experience);
 CopperCoins = Save->Version >= 3 ? FMath::Max(0,Save->CopperCoins) : 0;
	const bool bSameNative = Save->Version == 1 ? !bMigrateLegacyNature : (NativeForm ? Save->NativeFormId == NativeForm->FormId : Save->NativeFormId.IsNone());
	NativeTier = bSameNative && (!NativeForm || NativeForm->bCanEvolve) ? FMath::Clamp(Save->NativeTier, 0, 2) : 0;
	CurrentTier = NativeTier;
	for (UCC_FormDefinition* Form : ArtifactForms)
	{
		if (!Form || !Form->bAvailable || !Form->IsComplete()) continue;
		const int32* Tier = Save->FormTiers.Find(Form->FormId);
		const bool bLegacyNative = Form == LegacyNature;
		if (!Tier && !bLegacyNative) continue;
		FCC_FormProgress P; P.Form = Form; P.Tier = Form->bCanEvolve ? FMath::Clamp(bLegacyNative ? Save->NativeTier : *Tier, 0, 2) : 0; Forms.Add(P);
		if (Save->SelectedForm == Form->FormId || (bLegacyNative && Save->SelectedForm.IsNone())) { CurrentForm = Form; CurrentTier = P.Tier; }
	}
	for (UCC_UpgradeDefinition* Upgrade : UpgradeCatalog)
	{
		if (!Upgrade) continue;
		const int32* Level = Save->UpgradeLevels.Find(Upgrade->UpgradeId);
		if (Level) { FCC_UpgradeProgress P; P.Upgrade = Upgrade; P.Level = FMath::Clamp(*Level, 0, Upgrade->Prices.Num()); Upgrades.Add(P); }
	}
	if (ManualSkills.ContainsByPredicate([this](const UCC_SkillDefinition* S){return !IsSkillUnlocked(S);})) ManualSkills.Empty();
 ApplyState(true); Notify(); return true;
}

bool UCC_ProgressionComponent::CanConfigureSkills() const
{
 if (!IsReady()) return false;
 for (const auto& Spec:GetCharacter()->GetAbilitySystemComponent()->GetActivatableAbilities()) if(Spec.IsActive() && Cast<UCC_SkillDefinition>(Spec.SourceObject.Get())) return false;
 for (int32 I=0; I<5; ++I) if (GetCharacter()->Skills->GetCooldownRemaining(I) > 0.f) return false;
 return true;
}
void UCC_ProgressionComponent::EquipSkill_Implementation(int32 Slot, UCC_SkillDefinition* Skill)
{
 if (!IsReady()) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidState); return; }
 if (!CanConfigureSkills()) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::OnCooldown); return; }
 if (Slot < 0 || Slot >= 4) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidSlot); return; }
 if (!IsValid(Skill) || !SkillCatalog.Contains(Skill)) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::NotInCatalog); return; }
 if (!IsSkillUnlocked(Skill)) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::NotUnlocked); return; }
 UCC_SkillComponent* Skills = GetCharacter()->Skills;
 if (Skill->SkillId.IsNone() || Skills->CurrentSlots.Num() != 4) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidConfiguration); return; }
 TArray<TObjectPtr<UCC_SkillDefinition>> Selection;
 for (const FCC_SkillSlot& S : Skills->CurrentSlots) Selection.Add(S.Skill);
 for (int32 I=0; I<4; ++I) if (I != Slot && Selection[I] == Skill) Selection[I] = Selection[Slot];
 Selection[Slot] = Skill;
 UCC_SkillLoadout* Check = NewObject<UCC_SkillLoadout>();
 for (int32 I=0; I<4; ++I) Check->Slots[I].Skill = Selection[I];
 if (!Check->IsComplete()) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidConfiguration); return; }
 const auto Previous = ManualSkills;
 ManualSkills = Selection;
 if (!Skills->ApplyLoadout(Skills->CurrentLoadout)) {
  ManualSkills = Previous; ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidConfiguration); return;
 }
 Notify(); ClientSkillConfigurationResult(ECC_SkillConfigurationResult::Success);
}
void UCC_ProgressionComponent::ResetSkills_Implementation()
{
 if (!IsReady()) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidState); return; }
 if (!CanConfigureSkills()) { ClientSkillConfigurationResult(ECC_SkillConfigurationResult::OnCooldown); return; }
 const auto Previous = ManualSkills;
 ManualSkills.Empty();
 if (!ApplyState(true)) { ManualSkills = Previous; ClientSkillConfigurationResult(ECC_SkillConfigurationResult::InvalidConfiguration); return; }
 Notify(); ClientSkillConfigurationResult(ECC_SkillConfigurationResult::Success);
}
void UCC_ProgressionComponent::ClientSkillConfigurationResult_Implementation(ECC_SkillConfigurationResult Result)
{
 OnSkillConfigurationFinished.Broadcast(Result);
}
UCC_FormDefinition* UCC_ProgressionComponent::GetActiveForm() const { return ResolveForm(); }
bool UCC_ProgressionComponent::GetStagePreview(UCC_FormDefinition* Form, int32 Tier, FCC_FormStage& OutStage) const
{
 OutStage = FCC_FormStage();
 const UCC_FormDefinition* Definition = Form ? Form : ResolveForm();
 if (!Definition || !Definition->bAvailable || !Definition->IsComplete() || !Definition->Stages.IsValidIndex(Tier)) return false;
 OutStage = Definition->Stages[Tier];
 if (OutStage.CombatBalance && !OutStage.CombatBalance->Resolve(OutStage.StatProfileId,OutStage.MaxHealth,OutStage.Attack)) return false;
 ApplyExperienceGrowth(Definition,OutStage);
 return true;
}
double UCC_ProgressionComponent::GetExperienceGrowthMultiplier(const UCC_FormDefinition* Form) const
{
 if (!Form || Form->FormId!=TEXT("InitialInk")) return 1.0;
 const double Rate=FMath::IsFinite(InitialInkBonusPerExperience) ? FMath::Max(0.0,InitialInkBonusPerExperience) : 0.0;
 return 1.0+double(FMath::Max(0,Experience))*Rate;
}
void UCC_ProgressionComponent::ApplyExperienceGrowth(const UCC_FormDefinition* Form,FCC_FormStage& Stats) const
{
 if (!Form || Form->FormId!=TEXT("InitialInk")) return;
 const double Multiplier=GetExperienceGrowthMultiplier(Form);
 Stats.MaxHealth=float(FMath::RoundToDouble(double(Stats.MaxHealth)*Multiplier));
 Stats.Attack=float(FMath::RoundToDouble(double(Stats.Attack)*Multiplier));
}
void UCC_ProgressionComponent::RefreshExperienceStats()
{
 if (GetOwner()->HasAuthority() && ResolveForm() && ResolveForm()->FormId==TEXT("InitialInk")) ApplyState(false);
}

bool UCC_ProgressionComponent::ApplyDeveloperPreset(bool bUnlockAll)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 // Do not allow remote clients or network sessions to invoke development presets.
 if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || GetWorld()->GetNetMode()!=NM_Standalone) return false;
 auto* Avatar=GetCharacter();
 if (!Avatar || CachedAvatar!=Avatar || !NativeForm || !NativeForm->IsComplete()) return false;
 auto* ASC=Avatar->GetAbilitySystemComponent();
 auto* AS=Cast<UCC_AttributeSet>(Avatar->GetAttributeSet());
 if (!ASC || !AS) return false;
 if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>()) {
  if (Adventure->bTravelling) return false;
  Adventure->MarkDeveloperSession();
 }
 Avatar->Skills->CancelActiveSkills();
 Avatar->ResetSprintState();
 Avatar->Skills->ClearDesignStates();
 ASC->CancelAllAbilities();
 ++Avatar->Skills->ShieldGeneration;
 ASC->RemoveActiveEffects(FGameplayEffectQuery());
 for (TActorIterator<ACC_SkillProjectile> It(GetWorld());It;++It) if (It->GetOwner()==Avatar) It->Destroy();
 if (!bUnlockAll) for (TActorIterator<ACC_ExperienceOrb> It(GetWorld());It;++It) if (It->Recipient==GetOwner()) It->Destroy();
 // Heal before HandleRespawn: the player override immediately reinitializes
 // progression, so a zero health ratio would otherwise trigger death again.
 ASC->SetNumericAttributeBase(AS->GetHealthAttribute(),AS->GetMaxHealth());
 ASC->SetNumericAttributeBase(AS->GetManaAttribute(),AS->GetMaxMana());
 if (!Avatar->IsAlive() || Avatar->IsRagdollActive()) Avatar->HandleRespawn();
 Forms.Empty(); Upgrades.Empty();
 bChromaticPreview=false;bChromaticSuppressed=false;
 ClearConsumableEffects();ItemStacks.Empty();EquippedItem=nullptr;ItemCooldownEndsAt=0;
 bDeveloperInventory=false;
 bRemoteShopUnlocked=false;ActiveTownShop=nullptr;
 Experience=bUnlockAll ? 1000000 : 0;
 CopperCoins=bUnlockAll ? 1000000 : 0;
 NativeTier=bUnlockAll && NativeForm->bCanEvolve ? 2 : 0;
 if (bUnlockAll) {
  for (UCC_FormDefinition* F:ArtifactForms) if (F && F->bAvailable && F->IsComplete()) {
   FCC_FormProgress P; P.Form=F; P.Tier=F->bCanEvolve ? 2 : 0; Forms.Add(P);
  }
  for (UCC_UpgradeDefinition* U:UpgradeCatalog) if (U && !U->Prices.IsEmpty()) {
   FCC_UpgradeProgress P; P.Upgrade=U; P.Level=U->Prices.Num(); Upgrades.Add(P);
  }
  const auto* Selected=Forms.FindByPredicate([this](const FCC_FormProgress& P) { return P.Form==CurrentForm; });
  if (CurrentForm && !Selected) CurrentForm=nullptr;
  CurrentTier=CurrentForm && Selected ? Selected->Tier : NativeTier;
 } else {
  CurrentForm=nullptr; CurrentTier=0; ManualSkills.Empty();
 }
 if (!ApplyState(true)) return false;
 ASC->SetNumericAttributeBase(AS->GetHealthAttribute(),AS->GetMaxHealth());
 ASC->SetNumericAttributeBase(AS->GetManaAttribute(),AS->GetMaxMana());
 ASC->SetNumericAttributeBase(AS->GetShieldAttribute(),0.f);
 Avatar->GetCharacterMovement()->StopMovementImmediately();
 if(!bUnlockAll)if(auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>()) {
  Adventure->ResetDeveloperTravel();Adventure->AdoptProgressSnapshot(CreateSnapshot());
 }
 Notify(); return true;
#else
 return false;
#endif
}

bool UCC_ProgressionComponent::ApplyDeveloperSandbox()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 if(!ApplyDeveloperPreset(true))return false;
 bDeveloperInventory=true;bRemoteShopUnlocked=true;
 for(UCC_ConsumableDefinition* Item:ItemCatalog)if(IsCatalogItem(Item))
 {
  FCC_ConsumableStack Stack;Stack.Item=Item;Stack.Count=999;ItemStacks.Add(Stack);
  if(!EquippedItem)EquippedItem=Item;
 }
 if(auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>())Adventure->UnlockDeveloperTravel();
 Notify();return true;
#else
 return false;
#endif
}
void UCC_ProgressionComponent::AddCopperCoins(int32 Amount) {
 if(!GetOwner() || !GetOwner()->HasAuthority() || Amount<=0) return;
 CopperCoins=int32(FMath::Min<int64>(MAX_int32,int64(CopperCoins)+Amount)); Notify();
}
bool UCC_ProgressionComponent::HasUnlockedForm(FName Id) const {
 if(Id.IsNone()) return true;
 if(NativeForm && NativeForm->bAvailable && NativeForm->FormId==Id) return true;
 return Forms.ContainsByPredicate([this,Id](const FCC_FormProgress& P){return P.Form && P.Form->bAvailable && P.Form->FormId==Id && ArtifactForms.Contains(P.Form);});
}
bool UCC_ProgressionComponent::IsSkillUnlocked(const UCC_SkillDefinition* Skill) const {
 return Skill && HasUnlockedForm(Skill->RequiredFormId);
}
bool UCC_ProgressionComponent::HasChromaticUnlock() const
{
 if (GetOwner() && !GetOwner()->HasAuthority()) return bChromaticUnlocked;
 return HasUnlockedForm(TEXT("Native")) && HasUnlockedForm(TEXT("Artifact_01")) && HasUnlockedForm(TEXT("Artifact_02"));
}
bool UCC_ProgressionComponent::IsChromaticAppearanceEnabled() const
{
 return HasChromaticUnlock() ? !bChromaticSuppressed : bChromaticPreview;
}
void UCC_ProgressionComponent::ToggleChromaticAppearance_Implementation()
{
 if (!IsReady()) return;
 if (const auto* PC=Cast<ACC_PlayerController>(GetCharacter()->GetController()); PC && PC->IsGameplayInputBlocked()) return;
 if (HasChromaticUnlock()) bChromaticSuppressed=!bChromaticSuppressed;
 else bChromaticPreview=!bChromaticPreview;
 Notify();
}
