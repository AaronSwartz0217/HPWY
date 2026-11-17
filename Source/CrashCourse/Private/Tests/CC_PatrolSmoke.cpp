#if WITH_DEV_AUTOMATION_TESTS
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Characters/CC_PlayerCharacter.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Characters/CC_CharacterAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
struct FPatrolProbe
{
	TArray<TWeakObjectPtr<ACC_PatrolEnemyCharacter>> Enemies;
	TArray<FVector> Starts;
	TArray<bool> Moved;
	TArray<bool> Animated;
	TWeakObjectPtr<ACC_PlayerCharacter> Player;
	FVector PlayerStart;
	bool bAcquired = false;
	bool bDisengaged = false;
	FTimerHandle Monitor;
};
void RunPatrolSmoke(const TArray<FString>& Args, UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client) return;
	const TSharedRef<FPatrolProbe> Probe = MakeShared<FPatrolProbe>();
	for (TActorIterator<ACC_PatrolEnemyCharacter> It(World); It; ++It) { Probe->Enemies.Add(*It); Probe->Starts.Add(It->GetActorLocation()); Probe->Moved.Add(false); Probe->Animated.Add(false); }
	Probe->Player = Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (Probe->Player.IsValid()) { Probe->PlayerStart = Probe->Player->GetActorLocation(); Probe->Player->GetCharacterMovement()->DisableMovement(); Probe->Player->SetActorEnableCollision(false); }
	World->GetTimerManager().SetTimer(Probe->Monitor, [Probe]()
	{
		for (int32 I = 0; I < Probe->Enemies.Num(); ++I)
		{
			if (Probe->Enemies[I].IsValid() && FVector::DistSquared2D(Probe->Starts[I], Probe->Enemies[I]->GetActorLocation()) > FMath::Square(40.f)) Probe->Moved[I] = true;
			if (ACC_PatrolEnemyCharacter* E = Probe->Enemies[I].Get(); E && E->GetVelocity().Size2D() > 5.f && E->Locomotion->Library)
			{
				const FCC_MeshLocomotion* Entry = E->Locomotion->Library->Entries.FindByPredicate([E](const FCC_MeshLocomotion& Item) { return Item.Mesh == E->GetMesh()->GetSkeletalMeshAsset(); });
				const UCC_CharacterAnimInstance* Anim = Cast<UCC_CharacterAnimInstance>(E->GetMesh()->GetAnimInstance());
				const bool bBlueprintRunning = Entry && Entry->AnimClass && Anim && Anim->IsA(Entry->AnimClass) && Anim->bRun;
				const bool bSingleNodeRunning = Entry && !Entry->AnimClass && E->GetMesh()->GetSingleNodeInstance() && E->GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() == Entry->Run;
				if (bBlueprintRunning || bSingleNodeRunning) Probe->Animated[I] = true;
			}
		}
	}, 0.2f, true);
	FTimerHandle Approach;
	World->GetTimerManager().SetTimer(Approach, [Probe]()
	{
		if (Probe->Player.IsValid() && !Probe->Enemies.IsEmpty() && Probe->Enemies[0].IsValid())
			Probe->Player->SetActorLocation(Probe->Enemies[0]->GetActorLocation() + FVector(350.f, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	}, 8.f, false);
	FTimerHandle Retreat;
	World->GetTimerManager().SetTimer(Retreat, [Probe]()
	{
		if (!Probe->Enemies.IsEmpty() && Probe->Enemies[0].IsValid()) Probe->bAcquired = Probe->Enemies[0]->CurrentTarget == Probe->Player.Get();
		if (Probe->Player.IsValid()) Probe->Player->SetActorLocation(Probe->PlayerStart, false, nullptr, ETeleportType::TeleportPhysics);
	}, 11.f, false);
	FTimerHandle Observe;
	World->GetTimerManager().SetTimer(Observe, [Probe]()
	{
		if (!Probe->Enemies.IsEmpty() && Probe->Enemies[0].IsValid()) Probe->bDisengaged = Probe->Enemies[0]->CurrentTarget == nullptr;
	}, 13.f, false);
	FTimerHandle Finish;
	World->GetTimerManager().SetTimer(Finish, [Probe, World]()
	{
		World->GetTimerManager().ClearTimer(Probe->Monitor);
		bool bAllMoved = !Probe->Enemies.IsEmpty();
		for (bool bMoved : Probe->Moved) bAllMoved &= bMoved;
		bool bAllAnimated = !Probe->Animated.IsEmpty();
		for (bool bAnimated : Probe->Animated) bAllAnimated &= bAnimated;
		bool bReturned = false, bDeathStopped = false;
		FString FinalState, FinalLocation;
		float HomeDistance = -1.f;
		if (!Probe->Enemies.IsEmpty() && Probe->Enemies[0].IsValid())
		{
			ACC_PatrolEnemyCharacter* Enemy = Probe->Enemies[0].Get();
			bReturned = Enemy->IsInsidePatrolArea(Enemy->GetActorLocation()) && (Enemy->PatrolState == ECC_PatrolState::Waiting || Enemy->PatrolState == ECC_PatrolState::Patrolling);
			FinalState = UEnum::GetValueAsString(Enemy->PatrolState);
			FinalLocation = Enemy->GetActorLocation().ToString();
			HomeDistance = FVector::Dist2D(Enemy->GetActorLocation(), Enemy->PatrolOrigin);
			Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(), 0.f);
			bDeathStopped = Enemy->PatrolState == ECC_PatrolState::Dead;
		}
		const bool bPass = bAllMoved && bAllAnimated && Probe->bAcquired && Probe->bDisengaged && bReturned && bDeathStopped;
		TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetBoolField(TEXT("passed"), bPass); Json->SetNumberField(TEXT("enemyCount"), Probe->Enemies.Num());
		Json->SetBoolField(TEXT("allMoved"), bAllMoved); Json->SetBoolField(TEXT("acquiredPlayer"), Probe->bAcquired);
		Json->SetBoolField(TEXT("allPlayedRunAnimation"), bAllAnimated);
		Json->SetBoolField(TEXT("disengaged"), Probe->bDisengaged); Json->SetBoolField(TEXT("returnedToPatrol"), bReturned); Json->SetBoolField(TEXT("deathStopped"), bDeathStopped);
		Json->SetStringField(TEXT("finalState"), FinalState); Json->SetStringField(TEXT("finalLocation"), FinalLocation); Json->SetNumberField(TEXT("homeDistance"), HomeDistance);
		FString Text; FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text));
		FFileHelper::SaveStringToFile(Text, *(FPaths::ProjectSavedDir() / TEXT("PatrolSmoke.json")));
		UE_LOG(LogTemp, Display, TEXT("PATROL_SMOKE_%s: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Text);
		if (GEngine) GEngine->Exec(World, TEXT("QUIT"));
	}, 28.f, false);
}
FAutoConsoleCommandWithWorldAndArgs PatrolSmokeCommand(TEXT("CC.PatrolSmokeTest"), TEXT("Run the flat-arena patrol smoke test, write Saved/PatrolSmoke.json, then exit."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RunPatrolSmoke));
}
#endif
