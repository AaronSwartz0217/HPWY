#if WITH_DEV_AUTOMATION_TESTS
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_GameplayData.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_SkillProjectile.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
namespace {
struct FCameraMeleeProbe {
 TWeakObjectPtr<ACC_PlayerCharacter> Player;
 TWeakObjectPtr<ACC_PatrolEnemyCharacter> Enemy;
 TWeakObjectPtr<AStaticMeshActor> Wall;
 bool Config=false, Retracted=false, Outside=false, Hidden=false, Restored=false, Attack=false;
 float Health=0;
};
void RunMeleeCamera(UWorld* World) {
 if (!World || World->GetNetMode()==NM_Client) return;
 auto P=MakeShared<FCameraMeleeProbe>();
 FTimerHandle Start;
 World->GetTimerManager().SetTimer(Start,[P,World] {
  P->Player=Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerPawn(World,0));
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(World);It;++It) { It->StopPatrol(); if(!P->Enemy.IsValid()) P->Enemy=*It; }
  if(!P->Player.IsValid() || !P->Enemy.IsValid()) { GEngine->Exec(World,TEXT("QUIT")); return; }
  auto* Player=P->Player.Get(); auto* Enemy=P->Enemy.Get();
  Player->SetActorLocation(FVector(-3000,-3000,100));
  Player->GetCharacterMovement()->DisableMovement();
  Player->GetController()->SetControlRotation(FRotator::ZeroRotator);
  Enemy->SetActorLocation(Player->GetActorLocation()+FVector(150,0,0)); Enemy->SetActorRotation(FRotator(0,180,0));
  P->Config=Enemy->Skills->CurrentLoadout && Enemy->Skills->CurrentLoadout->IsComplete();
  if(P->Config) for(const auto& Slot:Enemy->Skills->CurrentLoadout->Slots) {
   auto* S=Slot.Skill.Get();
   P->Config &= (S->Behavior==ECC_SkillBehavior::Melee || S->Behavior==ECC_SkillBehavior::Combo || S->Behavior==ECC_SkillBehavior::Sweep || S->Behavior==ECC_SkillBehavior::Knockback) && S->Range<=200 && !S->ProjectileNiagara && !S->CastNiagara && !S->CastParticles;
  }
  P->Health=Player->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());
  P->Attack=Enemy->Skills->ActivateSlot(0,Player);
  auto* Wall=World->SpawnActor<AStaticMeshActor>(); P->Wall=Wall;
  Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
  Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
  Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
  Wall->SetActorScale3D(FVector(.2,12,8)); Wall->SetActorLocation(Player->GetActorLocation()+FVector(-250,0,60));
  FTimerHandle Blocked; World->GetTimerManager().SetTimer(Blocked,[P] {
   auto* C=P->Player.Get(); auto* Arm=C->GetCameraBoom(); auto* Camera=C->GetFollowCamera();
   P->Retracted=Arm->bDoCollisionTest && Arm->ProbeChannel==ECC_Camera && Camera->GetAttachParent()==Arm && FVector::Dist(Camera->GetComponentLocation(),Arm->GetComponentLocation()+Arm->TargetOffset)<300;
   P->Outside=FMath::Abs(Camera->GetComponentLocation().X-P->Wall->GetActorLocation().X)>=24;
   P->Wall->SetActorLocation(C->GetActorLocation()+FVector(-50,0,60));
  },.8f,false);
  FTimerHandle Close; World->GetTimerManager().SetTimer(Close,[P] {
   P->Hidden=P->Player->GetMesh()->bOwnerNoSee;
   P->Wall->Destroy();
  },1.5f,false);
  FTimerHandle Finish; World->GetTimerManager().SetTimer(Finish,[P,World] {
   auto* C=P->Player.Get(); auto* Arm=C->GetCameraBoom();
   P->Restored=!C->GetMesh()->bOwnerNoSee && FVector::Dist(C->GetFollowCamera()->GetComponentLocation(),Arm->GetComponentLocation()+Arm->TargetOffset)>550;
   const bool Damage=C->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute())<P->Health;
   int32 Projectiles=0; for(TActorIterator<ACC_SkillProjectile> It(World);It;++It) ++Projectiles;
   auto J=MakeShared<FJsonObject>();
   J->SetBoolField(TEXT("passed"),P->Config&&P->Retracted&&P->Outside&&P->Hidden&&P->Restored&&P->Attack&&Damage&&Projectiles==0);
   J->SetBoolField(TEXT("allEnemySlotsMelee"),P->Config); J->SetBoolField(TEXT("cameraRetractsAtWall"),P->Retracted);
   J->SetBoolField(TEXT("cameraOutsideWall"),P->Outside); J->SetBoolField(TEXT("closeCameraHidesOwnerMesh"),P->Hidden);
   J->SetBoolField(TEXT("cameraAndMeshRecover"),P->Restored); J->SetBoolField(TEXT("meleeDealsDamage"),P->Attack&&Damage);
   J->SetNumberField(TEXT("liveSkillProjectiles"),Projectiles);
   FString Text; FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));
   FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("MeleeCameraSmoke.json")));
   UE_LOG(LogTemp,Display,TEXT("MELEE_CAMERA_SMOKE: %s"),*Text); GEngine->Exec(World,TEXT("QUIT"));
  },2.3f,false);
 },3.f,false);
}
FAutoConsoleCommandWithWorld MeleeCameraSmokeCommand(TEXT("CC.MeleeCameraSmokeTest"),TEXT("Test melee enemies and spring-arm wall response in a separate game process."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunMeleeCamera));
}
#endif
