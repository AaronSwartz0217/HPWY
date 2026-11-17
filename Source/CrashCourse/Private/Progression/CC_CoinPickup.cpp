#include "Progression/CC_CoinPickup.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
ACC_CoinPickup::ACC_CoinPickup() {
 bReplicates=true; SetReplicateMovement(true);
 GetCapsuleComponent()->InitCapsuleSize(24.f,42.f);
 GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
 GetCapsuleComponent()->SetCanEverAffectNavigation(false);
 AIControllerClass=AAIController::StaticClass(); AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
 bUseControllerRotationYaw=false;
 auto* Movement=GetCharacterMovement(); Movement->bOrientRotationToMovement=true;
 Movement->RotationRate=FRotator(0,360,0); Movement->MaxWalkSpeed=100.f;
 Movement->MaxStepHeight=25.f; Movement->bUseRVOAvoidance=true;
 // Keep the visual subobject name compatible with the existing pickup blueprint.
 CoinMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CoinMesh")); CoinMesh->SetupAttachment(GetCapsuleComponent());
 CoinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 CoinMesh->SetRelativeRotation(FRotator(0,-90,0));
 PickupSphere=CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere")); PickupSphere->SetupAttachment(GetCapsuleComponent());
 PickupSphere->InitSphereRadius(90.f); PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore); PickupSphere->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap); PickupSphere->SetGenerateOverlapEvents(true);
}
void ACC_CoinPickup::BeginPlay() {
 Super::BeginPlay();
 if(HasAuthority()) { PatrolOrigin=GetActorLocation(); if(bStartPatrolling) StartPatrol(); }
}
void ACC_CoinPickup::EndPlay(const EEndPlayReason::Type Reason) {
 GetWorldTimerManager().ClearTimer(PatrolTimer); Super::EndPlay(Reason);
}
void ACC_CoinPickup::StartPatrol() {
 if(!HasAuthority() || bCollected) return;
 StopPatrol();
 if(!GetController()) SpawnDefaultController();
 GetCharacterMovement()->MaxWalkSpeed=FMath::Max(1.f,PatrolSpeed);
 WaitForNextMove();
 GetWorldTimerManager().SetTimer(PatrolTimer,this,&ThisClass::UpdatePatrol,.2f,true);
}
void ACC_CoinPickup::StopPatrol() {
 if(!HasAuthority()) return;
 GetWorldTimerManager().ClearTimer(PatrolTimer); bMoving=false;
 if(auto* AI=GetController<AAIController>()) AI->StopMovement();
 GetCharacterMovement()->StopMovementImmediately();
}
void ACC_CoinPickup::WaitForNextMove() {
 bMoving=false;
 const float Min=FMath::Max(0.f,MinPatrolWait);
 NextMoveTime=GetWorld()->GetTimeSeconds()+FMath::FRandRange(Min,FMath::Max(Min,MaxPatrolWait));
}
void ACC_CoinPickup::UpdatePatrol() {
 auto* AI=GetController<AAIController>();
 if(!AI || bCollected) return;
 const float Now=GetWorld()->GetTimeSeconds();
 if(bMoving) {
  if(AI->GetMoveStatus()==EPathFollowingStatus::Idle || Now>=MoveDeadline) { AI->StopMovement(); WaitForNextMove(); }
  return;
 }
 if(Now<NextMoveTime) return;
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()); FNavLocation Center,Point;
 if(!Nav || PatrolRadius<=0 || !Nav->ProjectPointToNavigation(PatrolOrigin,Center,FVector(100,100,1000))
    || !Nav->GetRandomReachablePointInRadius(Center.Location,PatrolRadius,Point)) { WaitForNextMove(); return; }
 const auto Request=AI->MoveToLocation(Point.Location,5.f,false,true,true,false,nullptr,false);
 if(Request==EPathFollowingRequestResult::RequestSuccessful) { bMoving=true; MoveDeadline=Now+FMath::Max(10.f,PatrolRadius*4.f/FMath::Max(1.f,PatrolSpeed)); }
 else WaitForNextMove();
}
bool ACC_CoinPickup::TryCollect(ACC_PlayerCharacter* Player) {
 if(!HasAuthority() || bCollected || !IsValid(Player) || !Player->IsAlive()) return false;
 auto* PS=Player->GetPlayerState<ACC_PlayerState>(); if(!PS || !PS->Progression) return false;
 const auto* Capsule=Player->GetCapsuleComponent(); const FVector Delta=Player->GetActorLocation()-GetActorLocation();
 const float R=PickupSphere->GetScaledSphereRadius();
 if(Delta.SizeSquared2D()>FMath::Square(R+Capsule->GetScaledCapsuleRadius()) || FMath::Abs(Delta.Z)>R+Capsule->GetScaledCapsuleHalfHeight()) return false;
 bCollected=true; StopPatrol(); SetActorEnableCollision(false);
 PS->Progression->AddCopperCoins(CopperAmount); PS->Progression->AddExperience(ExperienceAmount);
 PlayPickupSound(GetActorLocation()); Destroy(); return true;
}
void ACC_CoinPickup::NotifyActorBeginOverlap(AActor* OtherActor) { Super::NotifyActorBeginOverlap(OtherActor); TryCollect(Cast<ACC_PlayerCharacter>(OtherActor)); }
void ACC_CoinPickup::PlayPickupSound_Implementation(FVector Location) { if(PickupSound && GetNetMode()!=NM_DedicatedServer) UGameplayStatics::PlaySoundAtLocation(this,PickupSound,Location); }
