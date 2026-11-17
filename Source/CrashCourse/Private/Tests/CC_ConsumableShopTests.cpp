#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ProgressionSave.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_CoinPickup.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Player/CC_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "GameplayTags/CCTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_ConsumableShopTest,"CrashCourse.Progression.ConsumableShop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_ConsumableShopTest::RunTest(const FString&)
{
 UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("ConsumableTests"),nullptr,true,ERHIFeatureLevel::Num,&Values);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* PS=World->SpawnActor<ACC_PlayerState>();auto* Player=World->SpawnActor<ACC_PlayerCharacter>();Player->SetPlayerState(PS);
 auto* ASC=Player->GetAbilitySystemComponent();ASC->InitAbilityActorInfo(PS,Player);ASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Player->GetAttributeSet()));
 ASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,ASC->MakeEffectContext());
 auto* P=PS->Progression.Get();P->NativeForm=LoadObject<UCC_FormDefinition>(nullptr,TEXT("/Game/HPWH/Forms/DA_Form_Native.DA_Form_Native"));P->InitializeAvatar();
 const TCHAR* Names[]={TEXT("XP_Small"),TEXT("XP_Medium"),TEXT("XP_Large"),TEXT("CooldownHaste"),TEXT("CooldownReset")};
 P->bRemoteShopUnlocked=true; // This suite covers consumables; permission boundaries have their own suite.
 for(auto* Name:Names){auto* Item=LoadObject<UCC_ConsumableDefinition>(nullptr,*FString::Printf(TEXT("/Game/HPWH/Items/DA_Item_%s.DA_Item_%s"),Name,Name));TestNotNull(Name,Item);if(Item)P->ItemCatalog.Add(Item);}
 if(P->ItemCatalog.Num()!=5){World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return false;}
 auto* Small=P->ItemCatalog[0].Get();auto* Medium=P->ItemCatalog[1].Get();auto* Haste=P->ItemCatalog[3].Get();auto* Reset=P->ItemCatalog[4].Get();
 P->AddCopperCoins(200);P->BuyItem(Small,3);TestEqual(TEXT("Purchase debits coins"),P->CopperCoins,185);TestEqual(TEXT("Three bottles stocked"),P->GetItemCount(Small),3);TestTrue(TEXT("First purchase configures slot 5"),P->EquippedItem==Small);
 P->BuyItem(Small,-1);P->BuyItem(Small,MAX_int32);TestEqual(TEXT("Invalid quantities cannot credit money"),P->CopperCoins,185);
 auto* Forged=NewObject<UCC_ConsumableDefinition>();Forged->ItemId=Small->ItemId;P->BuyItem(Forged,1);TestEqual(TEXT("Unlisted catalog entry rejected"),P->CopperCoins,185);
 P->UseEquippedItem();TestEqual(TEXT("XP potion grants 50"),P->Experience,50);TestEqual(TEXT("Exactly one bottle consumed"),P->GetItemCount(Small),2);TestTrue(TEXT("15 second shared item cooldown"),FMath::IsNearlyEqual(P->GetItemCooldownRemaining(),15.f));
 P->BuyItem(Medium,1);P->EquipItem(Medium);P->UseEquippedItem();TestEqual(TEXT("Equipping cannot bypass item cooldown"),P->GetItemCount(Medium),1);TestEqual(TEXT("Blocked item grants no XP"),P->Experience,50);
 auto* Save=P->CreateSnapshot();TestTrue(TEXT("Save snapshot restores"),P->RestoreSnapshot(Save));TestEqual(TEXT("Stable ID restores equipped item"),P->EquippedItem.Get(),Medium);TestEqual(TEXT("Stable ID restores stock"),P->GetItemCount(Small),2);TestTrue(TEXT("Reload preserves item cooldown"),P->GetItemCooldownRemaining()>14);
 Save->ItemCooldownSavedUtcTicks-=20*ETimespan::TicksPerSecond;TestTrue(TEXT("Elapsed snapshot restores"),P->RestoreSnapshot(Save));TestEqual(TEXT("Offline time elapses item cooldown"),P->GetItemCooldownRemaining(),0.f);
 P->BuyItem(Haste,1);P->BuyItem(Reset,2);
 auto Cooldown=[&](int32 Slot,float Seconds){auto Spec=ASC->MakeOutgoingSpec(UCC_CooldownEffect::StaticClass(),1,ASC->MakeEffectContext());Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("CCTags.Skill.Duration")),Seconds);Spec.Data->DynamicGrantedTags.AddTag(UCC_ConfiguredAbility::GetSlotCooldownTag(Slot));return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());};
 Cooldown(0,20);Cooldown(4,20);UCC_ConfiguredAbility::ApplyEffect(Player,Player,UCC_DefenseEffect::StaticClass(),7,20);
 P->EquipItem(Haste);P->UseEquippedItem();TestTrue(TEXT("Haste potion starts 10 second effect"),FMath::IsNearlyEqual(Player->Skills->GetCooldownHasteRemaining(),10.f));
 // UWorld caps large deltas; use actual elapsed simulation time. This fixture has not begun play, so tick its component explicitly.
 const float BeforeTime=World->GetTimeSeconds();World->Tick(LEVELTICK_All,1.f);const float Elapsed=World->GetTimeSeconds()-BeforeTime;static_cast<UActorComponent*>(Player->Skills.Get())->TickComponent(Elapsed,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Haste doubles skill cooldown progression"),FMath::IsNearlyEqual(Player->Skills->GetCooldownRemaining(0),20.f-2*Elapsed,.03f));TestTrue(TEXT("Haste affects basic attack"),FMath::IsNearlyEqual(Player->Skills->GetCooldownRemaining(4),20.f-2*Elapsed,.03f));TestTrue(TEXT("Haste never accelerates item slot"),FMath::IsNearlyEqual(P->GetItemCooldownRemaining(),15.f-Elapsed,.03f));
 TestEqual(TEXT("Haste preserves active buffs"),ASC->GetNumericAttribute(UCC_AttributeSet::GetDefenseAttribute()),7.f);
 P->ItemCooldownEndsAt=0;P->EquipItem(Reset);P->UseEquippedItem();TestEqual(TEXT("Reset clears skill cooldown"),Player->Skills->GetCooldownRemaining(0),0.f);TestEqual(TEXT("Reset clears basic attack cooldown"),Player->Skills->GetCooldownRemaining(4),0.f);TestTrue(TEXT("Reset retains 15 second item cooldown"),FMath::IsNearlyEqual(P->GetItemCooldownRemaining(),15.f));TestEqual(TEXT("Reset preserves active buffs"),ASC->GetNumericAttribute(UCC_AttributeSet::GetDefenseAttribute()),7.f);
 P->ItemCooldownEndsAt=0;P->UseEquippedItem();TestEqual(TEXT("Reset without cooldown does not waste bottle"),P->GetItemCount(Reset),1);TestEqual(TEXT("No-op does not start item cooldown"),P->GetItemCooldownRemaining(),0.f);
 P->Experience=MAX_int32-10;P->EquipItem(Small);P->UseEquippedItem();TestEqual(TEXT("XP overflow consumes nothing"),P->GetItemCount(Small),2);
 auto* Balance=LoadObject<UCC_CombatBalance>(nullptr,TEXT("/Game/HPWH/Balance/DA_CombatBalance.DA_CombatBalance"));
 const TCHAR* Families[]={TEXT("Nature"),TEXT("Ceramic"),TEXT("Bronze")};const int32 Bases[]={3,4,5};
 if(TestNotNull(TEXT("Reward balance"),Balance))for(int32 Family=0;Family<3;++Family)for(int32 Tier=0;Tier<3;++Tier)TestEqual(FString::Printf(TEXT("%s tier %d copper"),Families[Family],Tier+1),Balance->ResolveCopperReward(FName(FString::Printf(TEXT("%s_T%d"),Families[Family],Tier+1))),Bases[Family]*(1<<Tier));
 P->Experience=0;const int32 Before=P->CopperCoins;
 auto* Enemy=World->SpawnActor<ACC_EnemyCharacter>();auto* EASC=Enemy->GetAbilitySystemComponent();EASC->InitAbilityActorInfo(Enemy,Enemy);EASC->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Enemy->GetAttributeSet()));EASC->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,EASC->MakeEffectContext());Enemy->CopperReward=3;
 UCC_ConfiguredAbility::ApplyEffect(Player,Enemy,UCC_DamageEffect::StaticClass(),10000);UCC_ConfiguredAbility::ApplyEffect(Player,Enemy,UCC_DamageEffect::StaticClass(),10000);TestEqual(TEXT("Kill grants copper only once"),P->CopperCoins,Before+3);
 auto* Coin=World->SpawnActor<ACC_CoinPickup>();Coin->SetActorLocation(Player->GetActorLocation());const int32 CoinBefore=P->CopperCoins;TestTrue(TEXT("Coin collected"),Coin->TryCollect(Player));TestEqual(TEXT("Coin grants 5"),P->CopperCoins,CoinBefore+5);TestFalse(TEXT("Coin cannot collect twice"),Coin->TryCollect(Player));
 P->ClearConsumableEffects();TestEqual(TEXT("Haste cleanup"),Player->Skills->GetCooldownHasteRemaining(),0.f);
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return true;
}
#endif
