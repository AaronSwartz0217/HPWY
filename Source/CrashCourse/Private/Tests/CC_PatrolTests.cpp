#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystem/Abilities/Enemy/CC_SearchForTarget.h"
#include "Progression/CC_ConfiguredAbility.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_PatrolBehaviorTest, "CrashCourse.AI.PatrolTransitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCC_PatrolBehaviorTest::RunTest(const FString& Parameters)
{
	UWorld::InitializationValues Values;
	Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PatrolUnitTestWorld"), nullptr, true, ERHIFeatureLevel::Num, &Values);
	ACC_PatrolEnemyCharacter* Enemy = World->SpawnActorDeferred<ACC_PatrolEnemyCharacter>(ACC_PatrolEnemyCharacter::StaticClass(), FTransform::Identity);
	Enemy->bStartPatrolling = false; Enemy->bDetectPlayers = false;
	Enemy->FinishSpawning(FTransform::Identity);
	Enemy->GetAbilitySystemComponent()->AddAttributeSetSubobject(Cast<UCC_AttributeSet>(Enemy->GetAttributeSet()));
	Enemy->DispatchBeginPlay();
	World->SpawnActor<AAIController>()->Possess(Enemy);
	Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Enemy->StartPatrol(true);
	const FVector Origin = Enemy->PatrolOrigin;
	Enemy->UpdateBrain();
	TestEqual(TEXT("Missing navigation waits safely"), Enemy->PatrolState, ECC_PatrolState::Waiting);
	TestTrue(TEXT("Point inside patrol circle accepted"), Enemy->IsInsidePatrolArea(Origin + FVector(Enemy->PatrolRadius * 0.5f, 0, 0)));
	TestFalse(TEXT("Point outside patrol circle rejected"), Enemy->IsInsidePatrolArea(Origin + FVector(Enemy->PatrolRadius + 1.f, 0, 0)));
	ACC_PlayerState* PS = World->SpawnActor<ACC_PlayerState>();
	ACC_PlayerCharacter* Player = World->SpawnActor<ACC_PlayerCharacter>(FVector(400, 0, 0), FRotator::ZeroRotator);
	Player->SetPlayerState(PS);
	Enemy->bDetectPlayers = true; Enemy->bRequireLineOfSight = false;
	Enemy->UpdateBrain();
	TestEqual(TEXT("Detects alive player"), Enemy->CurrentTarget.Get(), static_cast<ACC_BaseCharacter*>(Player));
	TestEqual(TEXT("Detection enters chase"), Enemy->PatrolState, ECC_PatrolState::Chasing);
	Player->SetActorLocation(Origin + FVector(Enemy->MaxChaseDistance + 500.f, 0, 0));
	Enemy->UpdateBrain();
	TestNull(TEXT("Leash clears target"), Enemy->CurrentTarget.Get());
	TestEqual(TEXT("Leash returns home"), Enemy->PatrolState, ECC_PatrolState::Returning);
	Enemy->SetActorLocation(Origin + FVector(Enemy->PatrolAcceptanceRadius + 1.f, 0, 0)); Enemy->UpdateBrain();
	TestEqual(TEXT("Outside explicit acceptance radius keeps returning"), Enemy->PatrolState, ECC_PatrolState::Returning);
	Enemy->SetActorLocation(Origin); Enemy->UpdateBrain();
	TestEqual(TEXT("At home resumes waiting"), Enemy->PatrolState, ECC_PatrolState::Waiting);
	Enemy->StopPatrol();
	TestFalse(TEXT("Stopped brain timer is removed"), World->GetTimerManager().IsTimerActive(Enemy->BrainTimer));
	TestEqual(TEXT("Stop disables state"), Enemy->PatrolState, ECC_PatrolState::Disabled);
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	const FGameplayAbilitySpecHandle Legacy = ASC->GiveAbility(FGameplayAbilitySpec(UCC_SearchForTarget::StaticClass()));
	TestFalse(TEXT("Legacy search cannot compete for movement"), ASC->FindAbilitySpecFromHandle(Legacy)->IsActive());
	Enemy->StartPatrol();
	ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 0.f);
	TestEqual(TEXT("Death stops patrol"), Enemy->PatrolState, ECC_PatrolState::Dead);
	TestFalse(TEXT("Death clears brain timer"), World->GetTimerManager().IsTimerActive(Enemy->BrainTimer));
	Enemy->StartPatrol(); TestEqual(TEXT("Dead enemy cannot restart"), Enemy->PatrolState, ECC_PatrolState::Dead);
	Enemy->bStartPatrolling = true;
	Enemy->ResetAttributes(); Enemy->HandleRespawn();
	TestEqual(TEXT("Respawn resumes patrol"), Enemy->PatrolState, ECC_PatrolState::Waiting);
	TestEqual(TEXT("Respawn preserves patrol origin"), Enemy->PatrolOrigin, Origin);
	World->DestroyWorld(false);
	return true;
}
#endif
