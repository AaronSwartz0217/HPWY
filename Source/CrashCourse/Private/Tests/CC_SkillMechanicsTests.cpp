#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_SkillProjectile.h"
#include "Progression/CC_SummonedSoldier.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"

namespace {
struct FSkillWorld {
 UWorld* W;
 ACC_PlayerCharacter* C;
 ACC_PlayerCharacter* Target;
 UAbilitySystemComponent* ASC;
 UAbilitySystemComponent* TargetASC;
 UCC_SkillLoadout* Loadout;
 FSkillWorld()
 {
  UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
  W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&V);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
  auto Make=[&](FVector Location){auto* PS=W->SpawnActor<ACC_PlayerState>();auto* P=W->SpawnActor<ACC_PlayerCharacter>();P->SetPlayerState(PS);auto* A=P->GetAbilitySystemComponent();A->InitAbilityActorInfo(PS,P);A->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(P->GetAttributeSet()));A->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,A->MakeEffectContext());P->SetActorLocation(Location);return P;};
  C=Make(FVector(0,0,96));Target=Make(FVector(140,0,96));Target->Tags.Remove(CrashTags::Player);ASC=C->GetAbilitySystemComponent();TargetASC=Target->GetAbilitySystemComponent();
  ASC->SetNumericAttributeBase(UCC_AttributeSet::GetAttackAttribute(),10);TargetASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(),1000);TargetASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),1000);
  Loadout=NewObject<UCC_SkillLoadout>();Loadout->Slots.SetNum(4);
  for(int32 I=0;I<4;++I){auto* S=NewObject<UCC_SkillDefinition>();S->SkillId=FName(*FString::Printf(TEXT("SkillTest_%d"),I));S->Power=10;S->PowerPerLevel=0;S->Cooldown=.35f;S->ManaCost=10;S->Windup=0;S->Range=220;S->Angle=100;S->Count=1;Loadout->Slots[I].Skill=S;}
  C->Skills->BasicAttack=NewObject<UCC_SkillDefinition>();C->Skills->BasicAttack->SkillId=TEXT("BasicTest");C->Skills->BasicAttack->ManaCost=0;C->Skills->BasicAttack->Power=5;C->Skills->BasicAttack->PowerPerLevel=0;C->Skills->ApplyLoadout(Loadout);
  W->GetWorldSettings()->NotifyBeginPlay();
 }
 ~FSkillWorld(){GEngine->DestroyWorldContext(W);W->DestroyWorld(false);}
 UCC_SkillDefinition* Skill(int32 I){return Loadout->Slots[I].Skill;}
 // TimerManager deliberately ticks once per engine frame, not once per World::Tick call.
 void Step(float Time){for(int32 I=0;I<FMath::CeilToInt(Time*120);++I){++GFrameCounter;W->Tick(LEVELTICK_All,1.f/120);}}
 float HP()const{return TargetASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());}
 float Mana()const{return ASC->GetNumericAttribute(UCC_AttributeSet::GetManaAttribute());}
 void Charge(){auto* S=Skill(0);S->DesignMechanic=ECC_DesignMechanic::Charge;S->Windup=.2f;S->MaxChargeSeconds=2;S->Cooldown=12;S->ManaCost=15;S->ControlDuration=1.2f;}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_SkillBufferTest,"CrashCourse.Skills.InputBufferAndCancellation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_SkillBufferTest::RunTest(const FString&)
{
 FSkillWorld F;F.Skill(0)->Windup=.2f;F.Skill(1)->Power=20;
 TestTrue(TEXT("First action starts"),F.C->Skills->ActivateSlot(0));F.Step(.06f);
 TestTrue(TEXT("Next input is accepted into the short buffer"),F.C->Skills->ActivateSlot(1));
 TestEqual(TEXT("Queued skill has not spent mana early"),F.Mana(),90.f);
 TestEqual(TEXT("Queued slot is visible for state inspection"),F.C->Skills->GetQueuedSkillSlot(),1);
 F.Step(.22f);TestEqual(TEXT("Both distinct skills hit once after the windup"),F.HP(),970.f);TestEqual(TEXT("Each skill spends mana once"),F.Mana(),80.f);
 F.Step(.5f);TestEqual(TEXT("Buffered input does not repeat"),F.HP(),970.f);
 TestTrue(TEXT("Another windup starts"),F.C->Skills->ActivateSlot(0));F.C->Skills->ActivateSlot(1);F.C->Skills->CancelPendingInput();F.Step(.4f);
 TestEqual(TEXT("Menu/rebind cancellation prevents pending or windup hits"),F.HP(),970.f);TestEqual(TEXT("Pending queue is empty"),F.C->Skills->GetQueuedSkillSlot(),INDEX_NONE);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_ChargeMechanicsTest,"CrashCourse.Skills.ChargeTimingAndReleaseAim",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_ChargeMechanicsTest::RunTest(const FString&)
{
 {
  FSkillWorld F;F.Charge();F.Target->SetActorLocation(FVector(0,140,96));
  TestTrue(TEXT("Charge starts"),F.C->Skills->ActivateSlot(0));TestFalse(TEXT("Repeated press cannot create another charge"),F.C->Skills->ActivateSlot(0));TestEqual(TEXT("Repeated charge spends mana only once"),F.Mana(),85.f);
  F.Step(1.1f);F.C->SetActorRotation(FRotator(0,90,0));F.C->Skills->ReleaseSlot(0);TestFalse(TEXT("A second press before release polling cannot extend the original charge"),F.C->Skills->ActivateSlot(0));F.Step(.08f);
  TestTrue(FString::Printf(TEXT("Half charge follows the release direction and deals 300%% attack (HP %.2f)"),F.HP()),FMath::IsNearlyEqual(F.HP(),970.f,.5f));
  TestTrue(TEXT("Release starts configured cooldown"),F.C->Skills->GetCooldownRemaining(0)>11.8f);TestFalse(TEXT("Partial charge does not apply full-charge stagger"),F.Target->Skills->IsStunned());
 }
 {
  FSkillWorld F;F.Charge();F.C->Skills->ActivateSlot(0);F.Step(.05f);F.C->Skills->ReleaseSlot(0);F.Step(.25f);
  TestTrue(TEXT("A quick tap produces exactly 200% attack"),FMath::IsNearlyEqual(F.HP(),980.f,.05f));
 }
 {
  FSkillWorld F;F.Charge();F.C->Skills->ActivateSlot(0);F.Step(2.08f);
  TestTrue(TEXT("Full charge completes within two seconds including windup"),FMath::IsNearlyEqual(F.HP(),960.f,.05f));TestTrue(TEXT("Full charge applies configured stagger"),F.Target->Skills->IsStunned());
 }
 {
  FSkillWorld F;F.Charge();F.C->Skills->ActivateSlot(0);F.Step(.05f);F.C->Skills->CancelPendingInput();F.Step(.4f);
  TestEqual(TEXT("Cancelled charge never strikes later"),F.HP(),1000.f);
  float CooldownDuration=0.f;
  TestTrue(TEXT("Cancellation during windup uses the existing three-second penalty"),F.C->Skills->GetCooldownState(0,CooldownDuration)>2.5f);
  TestEqual(TEXT("Cancellation effect duration matches HUD scale"),CooldownDuration,3.f);
  TestFalse(TEXT("Charge input is rejected during cancellation cooldown"),F.C->Skills->ActivateSlot(0));
  TestFalse(TEXT("Rejected charge input does not leave the slot held"),F.C->Skills->IsSlotHeld(0));
 }
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_ProjectileMuzzleTest,"CrashCourse.Skills.ProjectileMuzzleObstruction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_ProjectileMuzzleTest::RunTest(const FString&)
{
 FSkillWorld F;F.Target->SetActorLocation(FVector(500,0,96));F.Skill(0)->Behavior=ECC_SkillBehavior::Projectile;F.Skill(0)->Range=1000;
 auto* Wall=F.W->SpawnActor<AActor>();auto* S=NewObject<UStaticMeshComponent>(Wall);Wall->AddInstanceComponent(S);Wall->SetRootComponent(S);S->SetCanEverAffectNavigation(false);S->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));S->SetCollisionProfileName(TEXT("BlockAll"));S->SetWorldScale3D(FVector(.1,4,4));S->SetWorldLocation(FVector(40,0,150));S->RegisterComponent();
 TestTrue(TEXT("Casting against a wall consumes the cast normally"),F.C->Skills->ActivateSlot(0));int32 Count=0;for(TActorIterator<ACC_SkillProjectile> It(F.W);It;++It)++Count;
 TestEqual(TEXT("Projectile cannot appear on the far side of a nearby wall"),Count,0);TestEqual(TEXT("The blocked cast cannot damage the target"),F.HP(),1000.f);
 Wall->Destroy();F.Step(.4f);F.C->Skills->ActivateSlot(0);Count=0;for(TActorIterator<ACC_SkillProjectile> It(F.W);It;++It)++Count;TestEqual(TEXT("Clear muzzle still spawns a projectile"),Count,1);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_SkillBufferWindowTest,"CrashCourse.Skills.BufferCooldownAndExpiry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_SkillBufferWindowTest::RunTest(const FString&)
{
 {
  FSkillWorld F;F.Skill(0)->Cooldown=.4f;F.C->Skills->ActivateSlot(0);F.Step(.27f);
  TestTrue(TEXT("Late cooldown tap is buffered"),F.C->Skills->ActivateSlot(0));F.C->Skills->ReleaseSlot(0);
  TestEqual(TEXT("Buffered cooldown tap does not spend mana yet"),F.Mana(),90.f);F.Step(.2f);TestEqual(TEXT("A quick released tap fires once when cooldown ends"),F.HP(),980.f);F.Step(.6f);TestEqual(TEXT("No automatic attack loop"),F.HP(),980.f);
 }
 {
  FSkillWorld F;F.Skill(0)->Windup=.4f;F.C->Skills->ActivateSlot(0);F.Step(.05f);F.C->Skills->ActivateSlot(1);F.Step(.5f);
  TestEqual(TEXT("Expired input never fires after a long action"),F.HP(),990.f);TestEqual(TEXT("Expired input never spends mana"),F.Mana(),90.f);
 }
 {
  FSkillWorld F;F.Skill(0)->Cooldown=1.f;F.C->Skills->ActivateSlot(0);TestFalse(TEXT("A long cooldown cannot queue a distant future cast"),F.C->Skills->ActivateSlot(0));F.Step(1.2f);TestEqual(TEXT("Rejected input stays rejected after cooldown ends"),F.HP(),990.f);
 }
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_SkillBackgroundTest,"CrashCourse.Skills.BackgroundEffectsAndRootedDash",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_SkillBackgroundTest::RunTest(const FString&)
{
 {
  FSkillWorld F;auto* S=F.Skill(0);S->DesignMechanic=ECC_DesignMechanic::Regrowth;S->Count=2;S->Interval=.1f;S->HealFraction=.1f;
  F.ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),20);TestTrue(TEXT("Periodic heal starts"),F.C->Skills->ActivateSlot(0));TestTrue(TEXT("A running periodic heal does not lock attacks"),F.C->Skills->ActivateSlot(1));F.Step(.3f);
  TestEqual(TEXT("Background healing keeps both ticks"),F.ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),40.f);TestEqual(TEXT("The independent attack still hits"),F.HP(),990.f);
 }
 {
  FSkillWorld F;F.Skill(1)->Behavior=ECC_SkillBehavior::Dash;F.Skill(1)->Windup=.2f;const FVector Start=F.C->GetActorLocation();F.C->Skills->ActivateSlot(1);F.Step(.05f);F.C->Skills->ApplyCrowdControl(.5f,true);F.Step(.2f);TestTrue(TEXT("A root applied during windup prevents the delayed dash"),F.C->GetActorLocation().Equals(Start,1));
  TestFalse(TEXT("Dash cannot bypass a root"),F.C->Skills->ActivateSlot(1));TestTrue(TEXT("Rooted characters retain stationary attacks"),F.C->Skills->ActivateSlot(2));
 }
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_GravityTimelineTest,"CrashCourse.Skills.GravityFinishesOnTimelineDuringHitches",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_GravityTimelineTest::RunTest(const FString&)
{
 FSkillWorld F;auto* S=F.Skill(0);S->DesignMechanic=ECC_DesignMechanic::Gravity;S->Duration=3;S->Radius=350;S->ControlDuration=1.5;
 F.C->Skills->ActivateSlot(0,F.Target);
 TestTrue(TEXT("Gravity applies only the initial 40% hit at start"),FMath::IsNearlyEqual(F.HP(),996.f,.01f));
 for(int32 I=0;I<17;++I){++GFrameCounter;F.W->Tick(LEVELTICK_All,.18f);}
 TestTrue(FString::Printf(TEXT("At 3.06 seconds, gravity has delivered exactly 40%%+80%% despite 180ms frames (HP %.2f)"),F.HP()),FMath::IsNearlyEqual(F.HP(),988.f,.01f));
 TestFalse(TEXT("Gravity no longer remains active beyond its duration"),F.C->Skills->HasBlockingSkillAction());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_BronzeFactionTest,"CrashCourse.Factions.BronzeCommandAndGuard",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_BronzeFactionTest::RunTest(const FString&)
{
 FSkillWorld F;
 UCC_ConfiguredAbility::ApplyEffect(F.C,F.Target,UCC_DamageEffect::StaticClass(),1);
 TestTrue(TEXT("Leader remembers the enemy they attack"),F.C->Skills->GetCombatFocus()==F.Target);
 auto* Near=F.W->SpawnActor<ACC_PlayerCharacter>();Near->Tags.Remove(CrashTags::Player);Near->SetActorLocation(FVector(100,-250,96));
 auto* Soldier=F.W->SpawnActor<ACC_SummonedSoldier>();Soldier->SetActorLocation(FVector(0,-250,96));F.Skill(0)->Duration=60;Soldier->InitializeSummon(F.C,F.Skill(0));Soldier->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Soldier->Tick(.2f);
 TestTrue(TEXT("Bronze soldier focuses the leader's target instead of the nearer enemy"),Soldier->CurrentTarget==F.Target);
 F.C->Skills->ClearDesignStates();Soldier->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Soldier->Tick(.2f);
 TestTrue(TEXT("Without a command, soldier falls back to the nearest enemy"),Soldier->CurrentTarget==Near);
 F.Skill(0)->DesignMechanic=ECC_DesignMechanic::Guard;F.Skill(0)->DefenseFactor=2;F.Skill(0)->Duration=5;F.Skill(0)->Windup=.12f;
 F.Skill(1)->Windup=.5f;F.C->SetActorRotation(FRotator::ZeroRotator);F.ASC->SetNumericAttributeBase(UCC_AttributeSet::GetDefenseAttribute(),10);
 F.C->Skills->ActivateSlot(1);TestTrue(TEXT("Guard interrupts an offensive windup immediately"),F.C->Skills->ActivateSlot(0));
 TestEqual(TEXT("Guard uses the configured front defense"),F.C->Skills->GuardDefense(F.Target,10),20.f);
 TestEqual(TEXT("Guard never lowers an already stronger defense"),F.C->Skills->GuardDefense(F.Target,30),30.f);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_TempleFactionTest,"CrashCourse.Factions.TempleEmergencyAndAreaHits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_TempleFactionTest::RunTest(const FString&)
{
 {
  FSkillWorld F;F.Skill(0)->DesignMechanic=ECC_DesignMechanic::Invulnerable;F.Skill(0)->Duration=2;F.Skill(0)->Windup=.12f;
  F.C->Skills->ApplyCrowdControl(4,false);TestTrue(TEXT("Golden body can be activated while stunned"),F.C->Skills->ActivateSlot(0));
  TestTrue(TEXT("Golden body grants immediate immunity without an exposed windup"),F.C->Skills->IsInvulnerable()&&!F.C->Skills->IsStunned());
  UCC_ConfiguredAbility::ApplyEffect(F.Target,F.C,UCC_DamageEffect::StaticClass(),50);TestEqual(TEXT("Golden body blocks damage"),F.ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),100.f);
  F.Step(2.1f);TestTrue(TEXT("Existing negative effects resume after golden body expires"),F.C->Skills->IsStunned());
 }
 {
  FSkillWorld F;auto* PS=F.W->SpawnActor<ACC_PlayerState>();auto* Behind=F.W->SpawnActor<ACC_PlayerCharacter>();Behind->SetPlayerState(PS);auto* A=Behind->GetAbilitySystemComponent();A->InitAbilityActorInfo(PS,Behind);A->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Behind->GetAttributeSet()));A->ApplyGameplayEffectToSelf(GetDefault<UCC_InitialAttributesEffect>(),1,A->MakeEffectContext());A->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(),1000);A->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),1000);Behind->Tags.Remove(CrashTags::Player);Behind->SetActorLocation(FVector(280,0,96));
  auto* S=F.Skill(0);S->DesignMechanic=ECC_DesignMechanic::PulseBurst;S->PowerMode=ECC_SkillPowerMode::AttackMultiplier;S->Power=.5f;S->Radius=500;S->Count=1;S->Interval=.05f;
  F.C->Skills->ActivateSlot(0);F.Step(.1f);TestEqual(TEXT("Front enemy receives the pulse"),F.HP(),995.f);TestEqual(TEXT("The front enemy does not shield the rear enemy from area magic"),A->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),995.f);
 }
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_NatureFactionTest,"CrashCourse.Factions.NatureRootLifestealAndStealth",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_NatureFactionTest::RunTest(const FString&)
{
 FSkillWorld F;auto* Leech=F.Skill(1);Leech->DesignMechanic=ECC_DesignMechanic::Lifesteal;Leech->Duration=10;Leech->HealFraction=.2f;
 auto* Root=F.Skill(0);Root->DesignMechanic=ECC_DesignMechanic::RootArea;Root->PowerMode=ECC_SkillPowerMode::AttackMultiplier;Root->Power=1.2f;Root->Radius=300;Root->ControlDuration=3;
 F.ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),20);F.C->Skills->ActivateSlot(1);F.C->Skills->ActivateSlot(0,F.Target);
 TestTrue(TEXT("Vine prison still deals 120% attack and only roots movement"),FMath::IsNearlyEqual(F.HP(),988.f,.01f)&&F.Target->Skills->IsRooted()&&!F.Target->Skills->IsStunned());
 TestTrue(TEXT("Vine prison's direct hit feeds the existing lifesteal percentage"),FMath::IsNearlyEqual(F.ASC->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()),22.4f,.01f));
 auto* Stealth=F.Skill(2);Stealth->DesignMechanic=ECC_DesignMechanic::Stealth;Stealth->Duration=8;Stealth->SpeedFactor=1.2f;auto* Heal=F.Skill(3);Heal->DesignMechanic=ECC_DesignMechanic::Regrowth;Heal->Count=2;Heal->Interval=.1f;Heal->HealFraction=.04f;
 F.C->Skills->ActivateSlot(2);F.C->Skills->ActivateSlot(3);F.Step(.25f);TestTrue(TEXT("Non-damaging regeneration preserves nature stealth"),F.C->Skills->IsStealthed());
 F.Step(.2f);F.C->Skills->ActivateSlot(0,F.Target);TestFalse(TEXT("Offensive vine prison breaks stealth"),F.C->Skills->IsStealthed());
 return true;
}
#endif
