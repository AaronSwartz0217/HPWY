#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_ConsumableDefinition.h"
#include "World/CC_TownShop.h"
#include "CC_ProgressionComponent.generated.h"

class ACC_PlayerCharacter;
class UCC_ProgressionSave;

USTRUCT(BlueprintType)
struct FCC_FormProgress
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UCC_FormDefinition> Form;
	UPROPERTY(BlueprintReadOnly) int32 Tier = 0;
};
USTRUCT(BlueprintType)
struct FCC_UpgradeProgress
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UCC_UpgradeDefinition> Upgrade;
	UPROPERTY(BlueprintReadOnly) int32 Level = 0;
};
UENUM(BlueprintType)
enum class ECC_PurchaseResult : uint8 { Success, InsufficientExperience, MaxLevel, InvalidConfiguration, InvalidState, NotUnlocked };
UENUM(BlueprintType)
enum class ECC_SkillConfigurationResult : uint8 { Success, InvalidState, OnCooldown, InvalidSlot, NotInCatalog, InvalidConfiguration, NotUnlocked };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_SkillConfigurationFinished, ECC_SkillConfigurationResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCC_ProgressionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCC_PurchaseFinished, ECC_PurchaseResult, Result);

// Attach to PlayerState. Null CurrentForm means the always-available native form.
UCLASS(ClassGroup=(Crash), meta=(BlueprintSpawnableComponent))
class CRASHCOURSE_API UCC_ProgressionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCC_ProgressionComponent();
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_State,Category="Town Shops") bool bRemoteShopUnlocked=false;
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_State,Category="Town Shops") TObjectPtr<ACC_TownShop> ActiveTownShop;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Town Shops",meta=(ClampMin="1")) int32 RemoteShopPrice=100;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Town Shops",meta=(ClampMin="1")) int32 InnRestPrice=5;
 UFUNCTION(BlueprintPure,Category="Town Shops") ECC_TownShopId GetLocalTownShop() const;
 UFUNCTION(BlueprintPure,Category="Town Shops") bool CanAccessTownShop(ECC_TownShopId Shop) const;
 UFUNCTION(BlueprintCallable,Server,Reliable,Category="Town Shops") void EnterTownShop(ACC_TownShop* Shop);
 UFUNCTION(BlueprintCallable,Server,Reliable,Category="Town Shops") void PurchaseRemoteShop();
 UFUNCTION(BlueprintCallable,Server,Reliable,Category="Town Shops") void PurchaseInnRest();
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Items") TArray<TObjectPtr<UCC_ConsumableDefinition>> ItemCatalog;
 UPROPERTY(Transient,BlueprintReadOnly,Category="Developer") bool bDeveloperInventory=false;
 UFUNCTION(BlueprintPure,Category="Items") int32 GetItemStackLimit(const UCC_ConsumableDefinition* Item) const;
 bool ApplyDeveloperSandbox();
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_State,Category="Items") TArray<FCC_ConsumableStack> ItemStacks;
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_State,Category="Items") TObjectPtr<UCC_ConsumableDefinition> EquippedItem;
 UPROPERTY(BlueprintReadOnly,ReplicatedUsing=OnRep_State,Category="Items") double ItemCooldownEndsAt=0;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Items",meta=(ClampMin="15")) float ItemCooldownSeconds=15.f;
 UPROPERTY(BlueprintAssignable,Category="Items") FCC_ItemActionFinished OnItemActionFinished;
 UFUNCTION(BlueprintPure,Category="Items") int32 GetItemCount(const UCC_ConsumableDefinition* Item) const;
 UFUNCTION(BlueprintPure,Category="Items") float GetItemCooldownRemaining() const;
 UFUNCTION(BlueprintPure,Category="Items") float GetEquippedItemCooldownRemaining() const;
 UFUNCTION(BlueprintPure,Category="Items") UCC_ConsumableDefinition* FindTravelScroll(FName Region) const;
 bool ConsumeTravelScroll(FName Region);
 UFUNCTION(BlueprintCallable,Server,Reliable,Category="Items") void BuyItem(UCC_ConsumableDefinition* Item,int32 Quantity=1);
 UFUNCTION(BlueprintCallable,Server,Reliable,Category="Items") void EquipItem(UCC_ConsumableDefinition* Item);
 UFUNCTION(BlueprintCallable,Server,Reliable,Category="Items") void UseEquippedItem();
 UFUNCTION(Server,Reliable,Category="Items") void UseItem(UCC_ConsumableDefinition* Item);
 void ClearConsumableEffects();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Progression") TObjectPtr<UCC_FormDefinition> NativeForm;
	// Artifact-unlocked routes, including Nature. Server rejects assets outside this catalog.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Progression") TArray<TObjectPtr<UCC_FormDefinition>> ArtifactForms;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Progression") TArray<TObjectPtr<UCC_UpgradeDefinition>> UpgradeCatalog;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Skills") TArray<TObjectPtr<UCC_SkillDefinition>> SkillCatalog;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) TArray<TObjectPtr<UCC_SkillDefinition>> ManualSkills;
	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Skills") void EquipSkill(int32 Slot, UCC_SkillDefinition* Skill);
	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Skills") void ResetSkills();
	UFUNCTION(BlueprintPure, Category="Skills") bool CanConfigureSkills() const;
 UFUNCTION(BlueprintPure, Category="Skills") bool IsSkillUnlocked(const UCC_SkillDefinition* Skill) const;
 UFUNCTION(BlueprintPure, Category="Progression") bool HasUnlockedForm(FName FormId) const;
 UFUNCTION(BlueprintPure, Category="Progression|Chromatic") bool HasChromaticUnlock() const;
 UFUNCTION(BlueprintPure, Category="Progression|Chromatic") bool IsChromaticAppearanceEnabled() const;
 UFUNCTION(BlueprintPure, Category="Progression|Chromatic") bool HasRainbowHealth() const { return HasChromaticUnlock() || bChromaticPreview; }
 UFUNCTION(BlueprintCallable, Server, Reliable, Category="Progression|Chromatic") void ToggleChromaticAppearance();
 UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State, Category="Progression|Chromatic") bool bChromaticSuppressed=false;
 UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State, Category="Progression|Chromatic") bool bChromaticPreview=false;
 UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State, Category="Progression|Chromatic") bool bChromaticUnlocked=false;
 UPROPERTY(BlueprintAssignable, Category="Skills") FCC_SkillConfigurationFinished OnSkillConfigurationFinished;
 UFUNCTION(BlueprintPure, Category="Progression|UI") UCC_FormDefinition* GetActiveForm() const;
 // Base stage values resolved from the shared balance table, before purchased bonuses.
 UFUNCTION(BlueprintPure, Category="Progression|UI") bool GetStagePreview(UCC_FormDefinition* Form, int32 Tier, FCC_FormStage& OutStage) const;
 // InitialInk only: held currency, not lifetime experience. Other forms keep their balance profiles.
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Progression|InitialInk",meta=(ClampMin="0")) double InitialInkBonusPerExperience=0.01;
 UFUNCTION(BlueprintPure,Category="Progression|UI") double GetExperienceGrowthMultiplier(const UCC_FormDefinition* Form) const;
 UFUNCTION(BlueprintPure,Category="Progression|UI") int32 GetOwnedFormTier(UCC_FormDefinition* Form) const;
 UFUNCTION(BlueprintPure,Category="Progression|UI") int32 GetFormEvolutionCost(UCC_FormDefinition* Form) const;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) int32 Experience = 0;
 UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) int32 CopperCoins = 0;
 UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Progression") void AddCopperCoins(int32 Amount);
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) TObjectPtr<UCC_FormDefinition> CurrentForm;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) int32 CurrentTier = 0;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) int32 NativeTier = 0;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) TArray<FCC_FormProgress> Forms;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_State) TArray<FCC_UpgradeProgress> Upgrades;
	UPROPERTY(BlueprintAssignable) FCC_ProgressionChanged OnChanged;
	UPROPERTY(BlueprintAssignable) FCC_PurchaseFinished OnPurchaseFinished;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Progression") void AddExperience(int32 Amount);
	// Call only after a validated backend load, before admitting gameplay/rewards.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Progression") void RestoreExperienceFromBackend(int32 SavedExperience);
	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Progression") void Interact(AActor* Artifact);
	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Progression") void SwitchForm(UCC_FormDefinition* Form);
	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Progression") void Evolve();
 UFUNCTION(BlueprintCallable, Server, Reliable, Category="Progression") void EvolveForm(UCC_FormDefinition* Form);
	UFUNCTION(BlueprintCallable, Server, Reliable, Category="Progression") void Purchase(UCC_UpgradeDefinition* Upgrade);
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetEvolutionCost() const;
	UFUNCTION(BlueprintPure, Category="Progression") bool CanEvolveCurrentForm() const;
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetUpgradeLevel(UCC_UpgradeDefinition* Upgrade) const;
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetSkillBonus(FName SkillId) const;
	UFUNCTION(BlueprintCallable, Category="Progression") bool SaveProgress(const FString& Slot = TEXT("SpiritTales"));
	UFUNCTION(BlueprintCallable, Category="Progression") bool LoadProgress(const FString& Slot = TEXT("SpiritTales"));
	// Native travel handoff; does not itself write a file or trust a client RPC.
	UCC_ProgressionSave* CreateSnapshot() const;
	bool RestoreSnapshot(const UCC_ProgressionSave* Save);
	// Local development only; never exposed as a client RPC or UI action.
	bool ApplyDeveloperPreset(bool bUnlockAll);
	void InitializeAvatar();
	void RefreshAppearance();
private:
 UFUNCTION(Client,Reliable) void ClientOpenTownShop(ECC_TownShopId Shop);
 void PersistTownPurchase();
 friend class FCC_ConsumableShopTest;
 double GetItemClock() const;
 void ApplyConsumable(UCC_ConsumableDefinition* Item);
 bool IsCatalogItem(const UCC_ConsumableDefinition* Item) const;
 UFUNCTION(Client,Reliable) void ClientItemResult(ECC_ItemResult Result);
	friend class ACC_FormPowerOrb;
	bool CollectArtifactPower(UCC_FormDefinition* Form,int32 ExperienceReward);
	ACC_PlayerCharacter* GetCharacter() const;
	UCC_FormDefinition* ResolveForm() const;
	bool IsReady() const;
	bool ApplyState(bool bReplaceSkills);
 void ApplyExperienceGrowth(const UCC_FormDefinition* Form,FCC_FormStage& Stats) const;
 void RefreshExperienceStats();
	void Notify();
	UFUNCTION() void OnRep_State();
	UFUNCTION(Client, Reliable) void ClientResult(ECC_PurchaseResult Result);
 UFUNCTION(Client, Reliable) void ClientSkillConfigurationResult(ECC_SkillConfigurationResult Result);
	UPROPERTY() TWeakObjectPtr<ACC_PlayerCharacter> CachedAvatar;
	UPROPERTY() TObjectPtr<USkeletalMesh> HumanMesh;
	UPROPERTY() TSubclassOf<UAnimInstance> HumanAnimClass;
	FVector HumanMeshScale = FVector::OneVector;
	FCC_FormStage HumanStats;
};
