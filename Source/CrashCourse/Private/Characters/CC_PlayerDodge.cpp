#include "Characters/CC_PlayerCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "CC_DodgeAfterimageComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Player/CC_PlayerController.h"
#include "Progression/CC_SkillComponent.h"
#include "TimerManager.h"

namespace
{
bool SpawnDodgeAfterimages(UWorld& World, USkeletalMeshComponent& Source, UMaterialInterface* Material, FVector AvatarLocation, FVector From, FVector To)
{
 auto* Mesh=Source.GetSkeletalMeshAsset();
 if(!Mesh || !Material)return false;

 auto* Trail=World.SpawnActor<AActor>(AActor::StaticClass(),From,FRotator::ZeroRotator);
 if(!Trail)return false;
 Trail->Tags.Add(TEXT("CC.DodgeTrail"));
 Trail->SetActorEnableCollision(false);
 Trail->SetLifeSpan(.36f);

 const int32 Count=FVector::DistSquared2D(From,To)>FMath::Square(85.f)?2:1;
 for(int32 Index=0;Index<Count;++Index)
 {
  auto* Pose=NewObject<UCC_DodgeAfterimageComponent>(Trail);
  Trail->AddInstanceComponent(Pose);
  if(Index==0)Trail->SetRootComponent(Pose);
  Pose->SetSkinnedAssetAndUpdate(Mesh);
  Pose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Pose->SetCastShadow(false);
  Pose->SetCanEverAffectNavigation(false);

  FTransform PoseTransform=Source.GetComponentTransform();
  PoseTransform.AddToTranslation(FMath::Lerp(From,To,.48f*Index)-AvatarLocation);
  Pose->SetWorldTransform(PoseTransform);
  auto* Tint=UMaterialInstanceDynamic::Create(Material,Pose);
  const float Opacity=Index==0?.55f:.36f;
  Tint->SetScalarParameterValue(TEXT("GhostOpacity"),Opacity);
  for(int32 Slot=0;Slot<Pose->GetNumMaterials();++Slot)Pose->SetMaterial(Slot,Tint);
  Pose->RegisterComponent();
  Pose->CopyPoseFromSkeletalComponent(&Source);
  Pose->RefreshBoneTransforms();
  for(int32 Step=1;Step<=4;++Step)
  {
   FTimerHandle Fade;
   const float Alpha=1.f-Step/5.f;
   World.GetTimerManager().SetTimer(Fade,FTimerDelegate::CreateWeakLambda(Tint,[Tint,Opacity,Alpha]()
   {
    Tint->SetScalarParameterValue(TEXT("GhostOpacity"),Opacity*Alpha);
   }),.065f*Step,false);
  }
 }
 return true;
}
}

void ACC_PlayerCharacter::RequestDodge(FVector Direction)
{
 Direction=Direction.GetSafeNormal2D();
 if(HasAuthority())ExecuteDodge(Direction);
 else if(IsLocallyControlled())ServerRequestDodge(Direction);
}

void ACC_PlayerCharacter::ServerRequestDodge_Implementation(FVector_NetQuantizeNormal Direction)
{
 ExecuteDodge(Direction);
}

void ACC_PlayerCharacter::ExecuteDodge(FVector Direction)
{
 auto* Movement=GetCharacterMovement();
 const auto* PC=Cast<ACC_PlayerController>(GetController());
 const float Now=GetSprintClock();
 if(!HasAuthority() || !PC || PC->IsGameplayInputBlocked() || !IsAlive() || IsSprinting()
    || !Movement->IsMovingOnGround() || !Skills || Skills->IsRooted() || Now<DodgeReadyAt || Direction.ContainsNaN())return;

 Direction=Direction.GetSafeNormal2D();
 if(Direction.IsNearlyZero())Direction=GetActorForwardVector().GetSafeNormal2D();
 const FVector From=GetActorLocation();
 FHitResult Block;
 SetActorLocation(From+Direction*DodgeDistance,true,&Block,ETeleportType::TeleportPhysics);
 const FVector To=GetActorLocation();
 if(FVector::DistSquared2D(From,To)<FMath::Square(5.f))return;

 DodgeReadyAt=Now+DodgeCooldown;
 Movement->bJustTeleported=true;
 Movement->StopMovementImmediately();
 ForceNetUpdate();
 MulticastDodgeTrail(From,To);
}

void ACC_PlayerCharacter::MulticastDodgeTrail_Implementation(FVector_NetQuantize From,FVector_NetQuantize To)
{
 UWorld* World=GetWorld();
 if(!World || GetNetMode()==NM_DedicatedServer)return;
 const FVector Delta=To-From;
 const float Length=Delta.Size();
 if(Length<5.f)return;
 if(auto* SourceMesh=GetMesh(); SourceMesh && SpawnDodgeAfterimages(*World,*SourceMesh,DodgeAfterimageMaterial.LoadSynchronous(),GetActorLocation(),From,To))return;

 auto* Shape=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StarterContent/Particles/Materials/M_Heat_Distortion.M_Heat_Distortion"));
 if(!Shape || !Material)return;

 const FVector Center=(From+To)*.5f+FVector(0.f,0.f,-22.f);
 auto* Trail=World->SpawnActor<AActor>(AActor::StaticClass(),Center,Delta.Rotation());
 if(!Trail)return;
 Trail->Tags.Add(TEXT("CC.DodgeTrail"));
 Trail->SetActorEnableCollision(false);
 Trail->SetLifeSpan(.24f);

 auto* Streak=NewObject<UStaticMeshComponent>(Trail);
 Trail->AddInstanceComponent(Streak);
 Trail->SetRootComponent(Streak);
 Streak->SetStaticMesh(Shape);
 Streak->SetMaterial(0,Material);
 Streak->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Streak->SetCastShadow(false);
 Streak->SetCanEverAffectNavigation(false);
 Streak->RegisterComponent();
 Streak->SetWorldLocationAndRotation(Center,Delta.Rotation());
 Streak->SetWorldScale3D(FVector(Length*.01f,.42f,.25f));

 FTimerHandle Shrink;
 World->GetTimerManager().SetTimer(Shrink,FTimerDelegate::CreateWeakLambda(Streak,[Streak,Length]()
 {
  Streak->SetWorldScale3D(FVector(Length*.009f,.19f,.11f));
 }),.085f,false);
}
