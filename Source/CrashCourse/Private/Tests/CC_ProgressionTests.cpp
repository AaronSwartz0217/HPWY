#include "Progression/CC_FormPowerOrb.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_ExperienceOrb.h"
#include "Progression/CC_SkillComponent.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "UI/CC_WidgetComponent.h"
#include "UI/CC_AttributeWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_ProgressionTransactions, "CrashCourse.Progression.TransactionsAndDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_ProgressionTransactions::RunTest(const FString& Parameters)
{
	UWorld::InitializationValues Values;
	Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("ProgressionTestWorld")), nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACC_PlayerState* PS = World->SpawnActor<ACC_PlayerState>();
	ACC_PlayerCharacter* Player = World->SpawnActor<ACC_PlayerCharacter>();
	Player->SetPlayerState(PS);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(PS, Player);
	ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));
	ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(), 1.f, ASC->MakeEffectContext());
	TestEqual(TEXT("Default initialization grants full health"), ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()), 100.f);
	TestEqual(TEXT("Default initialization grants full mana"), ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute()), 100.f);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 50.f);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxManaAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetManaAttribute(), 80.f);
	UCC_SkillLoadout* Loadout = NewObject<UCC_SkillLoadout>(PS);
	for (int32 I = 0; I < 4; ++I)
	{
		Loadout->Slots[I].Skill = NewObject<UCC_SkillDefinition>(PS);
		Loadout->Slots[I].Skill->SkillId = FName(*FString::Printf(TEXT("Test_%d"), I));
	}
	auto MakeForm = [PS, Loadout](FName Id)
	{
		UCC_FormDefinition* Form = NewObject<UCC_FormDefinition>(PS); Form->FormId = Id;
		for (int32 I = 0; I < 3; ++I) { Form->Stages[I].Loadout = Loadout; Form->Stages[I].Cost = I * 100; Form->Stages[I].MaxHealth = 100.f + I * 50.f; }
		return Form;
	};
	UCC_ProgressionComponent* P = PS->Progression;
 P->bRemoteShopUnlocked=true;
	P->NativeForm = MakeForm(TEXT("Native"));
	UCC_FormDefinition* ArtifactForm = MakeForm(TEXT("Artifact")); P->ArtifactForms.Add(ArtifactForm);
	P->InitializeAvatar();
	TestEqual(TEXT("Four abilities granted"), ASC->GetActivatableAbilities().Num(), 4);
	P->AddExperience(99); P->Evolve();
	TestEqual(TEXT("Insufficient balance preserved"), P->Experience, 99);
	TestEqual(TEXT("Insufficient evolution rejected"), P->CurrentTier, 0);
	P->AddExperience(1); P->Evolve();
	TestEqual(TEXT("Exact price spent"), P->Experience, 0);
	TestEqual(TEXT("Native evolution without artifact"), P->NativeTier, 1);
	TestEqual(TEXT("Health percentage preserved"), Cast<UCC_AttributeSet>(Player->GetAttributeSet())->GetHealth(), 75.f);
	P->SwitchForm(ArtifactForm);
	TestNull(TEXT("Locked form rejected"), P->CurrentForm.Get());
	ACC_FormArtifact* Artifact = World->SpawnActor<ACC_FormArtifact>(); Artifact->SetActorLocation(FVector(100, 0, 0)); Artifact->Artifact->Form = ArtifactForm;
	// Isolate form transactions from XP reward accounting, tested separately below.
	Artifact->Artifact->ExperienceReward=0;
	P->Interact(Artifact);
	for(TActorIterator<ACC_FormPowerOrb> It(World);It;++It)if(It->Recipient==PS&&!It->IsActorBeingDestroyed()){
		It->BeginSeeking();It->SetActorLocation(Player->GetActorLocation());It->TryCollect(Player);break;
	}
	TestEqual(TEXT("Collecting the released artifact orb unlocks the form"), P->Forms.Num(), 1);
	TestNull(TEXT("Acquiring an artifact does not automatically transform"), P->CurrentForm.Get());
	TestEqual(TEXT("Acquiring an artifact preserves current native tier"), P->CurrentTier, 1);
	P->SwitchForm(ArtifactForm);
	TestEqual(TEXT("Inventory selection uses the artifact initial tier"), P->CurrentTier, 0);
	P->AddExperience(100); P->Evolve(); P->SwitchForm(nullptr);
	TestEqual(TEXT("Native evolution preserved"), P->CurrentTier, 1);
	P->SwitchForm(ArtifactForm);
	TestEqual(TEXT("Artifact evolution preserved"), P->CurrentTier, 1);
	UCC_UpgradeDefinition* Upgrade = NewObject<UCC_UpgradeDefinition>(PS); Upgrade->UpgradeId = TEXT("Health"); Upgrade->Prices = {25}; Upgrade->BonusPerLevel = 20.f; P->UpgradeCatalog.Add(Upgrade);
	P->AddExperience(25); P->Purchase(Upgrade); P->Purchase(Upgrade);
	TestEqual(TEXT("Max-level purchase not duplicated"), P->GetUpgradeLevel(Upgrade), 1);
	TestEqual(TEXT("No negative balance"), P->Experience, 0);
	TestEqual(TEXT("Permanent health bonus"), Cast<UCC_AttributeSet>(Player->GetAttributeSet())->GetMaxHealth(), 170.f);
	for (int32 I = 0; I < 5; ++I) { P->SwitchForm(nullptr); P->SwitchForm(ArtifactForm); }
	TestEqual(TEXT("Switching does not duplicate abilities"), ASC->GetActivatableAbilities().Num(), 4);
	TestEqual(TEXT("Switching does not stack stats"), Cast<UCC_AttributeSet>(Player->GetAttributeSet())->GetMaxHealth(), 170.f);
	UCC_SkillDefinition* Basic = NewObject<UCC_SkillDefinition>(PS);
	Basic->SkillId = TEXT("Test_BasicAttack"); Basic->ManaCost = 0.f;
	Player->Skills->BasicAttack = Basic;
	Player->Skills->ApplyLoadout(Loadout);
	TestEqual(TEXT("Basic attack is separate from four skill slots"), ASC->GetActivatableAbilities().Num(), 5);
	const float ManaBeforeAttack = ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute());
	TestTrue(TEXT("J basic attack activates"), Player->Skills->ActivateBasicAttack());
	TestEqual(TEXT("Basic attack consumes no mana"), ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute()), ManaBeforeAttack);
	TestTrue(TEXT("Skill 1 remains independently usable"), Player->Skills->ActivateSlot(0));
	TestTrue(TEXT("Basic attack cooldown independent"), Player->Skills->GetCooldownRemaining(4) > 0.f);
	P->SwitchForm(nullptr); P->SwitchForm(ArtifactForm);
	TestEqual(TEXT("Form switch preserves exactly one basic attack"), ASC->GetActivatableAbilities().Num(), 5);
	UCC_ConfiguredAbility::ApplyEffect(Player, Player, UCC_ShieldEffect::StaticClass(), 30.f, 5.f);
	ACC_EnemyCharacter* Enemy = World->SpawnActor<ACC_EnemyCharacter>(FVector(600, 0, 0), FRotator::ZeroRotator);
	UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent(); EnemyASC->InitAbilityActorInfo(Enemy, Enemy);
	EnemyASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Enemy->GetAttributeSet()));
	EnemyASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(), 100.f); EnemyASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 100.f);
	UCC_ConfiguredAbility::ApplyEffect(Enemy, Player, UCC_DamageEffect::StaticClass(), 20.f);
	TestEqual(TEXT("Shield absorbs damage"), ASC->GetNumericAttribute(UCC_AttributeSet::GetShieldAttribute()), 10.f);
	UCC_ConfiguredAbility::ApplyEffect(Player, Enemy, UCC_DamageEffect::StaticClass(), 200.f);
	UCC_ConfiguredAbility::ApplyEffect(Player, Enemy, UCC_DamageEffect::StaticClass(), 200.f);
	TestEqual(TEXT("Lethal hit scatters experience without crediting the wallet early"), P->Experience, 0);
	TArray<ACC_ExperienceOrb*> RewardOrbs;
	for (TActorIterator<ACC_ExperienceOrb> It(World); It; ++It) if (It->Recipient == PS) RewardOrbs.Add(*It);
	TestEqual(TEXT("Repeated lethal hits create only one reward burst"), RewardOrbs.Num(), Enemy->ExperienceReward);
	int32 CollectedExperience = 0;
	for (ACC_ExperienceOrb* Orb : RewardOrbs)
	{
		TestEqual(TEXT("Each enemy particle represents exactly one experience"), Orb->ExperienceValue, 1);
		TestFalse(TEXT("Scattering particles cannot be collected early"), Orb->TryCollect(Player));
		Orb->BeginSeeking();
		// This test invokes collection explicitly; leave automatic overlap collection to the live-world smoke.
		Orb->Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Orb->SetActorLocation(Player->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		const int32 Before = P->Experience;
		TestTrue(TEXT("Recipient collects a seeking particle in reach"), Orb->TryCollect(Player));
		TestFalse(TEXT("The same particle cannot be collected twice"), Orb->TryCollect(Player));
		TestEqual(TEXT("Collection credits one experience"), P->Experience, Before + 1);
		++CollectedExperience;
	}
	TestEqual(TEXT("Collecting the whole burst credits the enemy reward exactly once"), P->Experience, CollectedExperience);
	// All 20 templates must be executable, including cancellation of delayed abilities.
	for (int32 I = 0; I < 20; ++I)
	{
		Loadout->Slots[0].Skill->Behavior = static_cast<ECC_SkillBehavior>(I);
		ASC->SetNumericAttributeBase(UCC_AttributeSet::GetManaAttribute(), 100.f);
		ASC->RemoveActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(UCC_ConfiguredAbility::GetSlotCooldownTag(0).GetSingleTagContainer()));
		TestTrue(FString::Printf(TEXT("Skill template %d activates"), I + 1), Player->Skills->ActivateSlot(0));
		Player->Skills->ApplyLoadout(Loadout);
	}
	P->AddExperience(MAX_int32); P->AddExperience(1);
	TestEqual(TEXT("XP addition saturates without overflow"), P->Experience, MAX_int32);
	P->AddExperience(-100);
	TestEqual(TEXT("Negative rewards rejected"), P->Experience, MAX_int32);
	P->SwitchForm(nullptr);
	ArtifactForm->bAvailable = false;
	P->SwitchForm(ArtifactForm);
	TestNull(TEXT("Disabled unlocked form cannot be selected"), P->CurrentForm.Get());
	TestFalse(TEXT("Disabled artifact cannot be interacted with"), Artifact->Artifact->CanInteract(Player));
	ArtifactForm->bAvailable = true;
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_AttributeWidgetLifecycle, "CrashCourse.UI.AttributeWidgetLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_AttributeWidgetLifecycle::RunTest(const FString& Parameters)
{
	UWorld::InitializationValues Values;
	Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("AttributeWidgetTestWorld")), nullptr, true, ERHIFeatureLevel::Num, &Values);
	ACC_PlayerState* PS = World->SpawnActor<ACC_PlayerState>();
	ACC_PlayerCharacter* Player = World->SpawnActor<ACC_PlayerCharacter>();
	Player->SetPlayerState(PS);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(PS, Player);
	ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 50.f);
	UCC_AttributeWidget* WidgetObject = NewObject<UCC_AttributeWidget>(Player);
	WidgetObject->Initialize();
	UCC_WidgetComponent* Display = Player->HealthDisplay;
	Display->SetWidget(WidgetObject);
	Player->DispatchBeginPlay();
	Display->RefreshBindings();
	TestEqual(TEXT("Initial health sent before GE-initialized flag"), WidgetObject->CurrentValue, 50.f);
	TestEqual(TEXT("Initial maximum sent"), WidgetObject->CurrentMaxValue, 100.f);
	for (int32 I = 0; I < 3; ++I) Display->RefreshBindings();
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(), 200.f);
	TestEqual(TEXT("Maximum-only changes refresh widget"), WidgetObject->CurrentMaxValue, 200.f);
	TestEqual(TEXT("Maximum-only change recomputes percent"), WidgetObject->ValuePercent, 0.25f);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 80.f);
	TestEqual(TEXT("Health changes refresh widget"), WidgetObject->CurrentValue, 80.f);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(), 0.f);
	TestEqual(TEXT("Zero maximum yields safe percentage"), WidgetObject->ValuePercent, 0.f);
	Display->EndPlay(EEndPlayReason::RemovedFromWorld);
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 10.f);
	TestEqual(TEXT("EndPlay unbinds callbacks"), WidgetObject->CurrentValue, 80.f);
	World->DestroyWorld(false);
	return true;
}
#endif
