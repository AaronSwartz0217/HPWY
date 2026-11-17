#include "Interaction/CC_PlayerScanComponent.h"
#include "Interaction/CC_ScanData.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "World/CC_LevitationPlatform.h"
#include "World/CC_InteractiveDoubleDoor.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_ExperiencePack.h"
#include "UI/CC_WidgetComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "WFCMapGenerator.h"
UCC_PlayerScanComponent::UCC_PlayerScanComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.1f;}
void UCC_PlayerScanComponent::BeginPlay(){Super::BeginPlay();if(!Catalog)Catalog=LoadObject<UCC_ScanCatalog>(nullptr,TEXT("/Game/UI/Scan/DA_ScanCatalog.DA_ScanCatalog"));}
bool UCC_PlayerScanComponent::IsScanActive()const{
 const auto* PC=Cast<ACC_PlayerController>(GetOwner());const auto* C=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
 return PC&&PC->IsLocalController()&&C&&C->IsFirstPerson()&&C->IsAlive()&&!PC->IsGameplayInputBlocked();
}
void UCC_PlayerScanComponent::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* F){Super::TickComponent(Delta,Tick,F);ScanNow();}
void UCC_PlayerScanComponent::ScanNow(){
 Target.Reset();Info=nullptr;TargetDistance=0;
 const bool bScanActive = IsScanActive() && GetWorld();
 UpdateEnemyHealthDisplays(bScanActive);
 if(!bScanActive)return;
 FHitResult Hit;if(!TraceCrosshair(Hit))return;
 AActor* Actor=Hit.GetActor();UCC_ScanDescription* Found=ResolveInfoForActor(Actor);
 if(!Found||Hit.Distance>FMath::Min(ScanDistance,Found->MaxDistance))return;
 Target=Actor;Info=Found;TargetDistance=Hit.Distance;
}
void UCC_PlayerScanComponent::UpdateEnemyHealthDisplays(bool bScanActive)
{
 TSet<TWeakObjectPtr<ACC_EnemyCharacter>> Visible;
 if (bScanActive)
 {
  const auto* PC = CastChecked<ACC_PlayerController>(GetOwner());
  const APawn* Pawn = PC->GetPawn();
  const FVector Origin = Pawn->GetActorLocation();
  const float Range = FMath::Max(1.f, ScanDistance);
  FVector Eye;
  FRotator ViewRotation;
  PC->GetPlayerViewPoint(Eye, ViewRotation);
  FCollisionQueryParams Query;
  Query.AddIgnoredActor(Pawn);
  TArray<FOverlapResult> Nearby;
  GetWorld()->OverlapMultiByObjectType(Nearby, Origin, FQuat::Identity,
   FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Range), Query);
  TSet<ACC_EnemyCharacter*> Considered;
  for (const auto& Hit : Nearby)
  {
   auto* Enemy = Cast<ACC_EnemyCharacter>(Hit.GetActor());
   if (!IsValid(Enemy) || !Enemy->IsAlive() || Enemy->IsHidden() || !Enemy->HealthDisplay || Considered.Contains(Enemy))
    continue;
   Considered.Add(Enemy);
   const auto* InfoAsset = ResolveInfoForActor(Enemy);
   const float EnemyRange = InfoAsset ? FMath::Min(Range, InfoAsset->MaxDistance) : Range;
   if (FVector::DistSquared(Origin, Enemy->GetActorLocation()) > FMath::Square(EnemyRange))
    continue;
   FCollisionQueryParams SightQuery = Query;
   SightQuery.AddIgnoredActor(Enemy);
   if (GetWorld()->LineTraceTestByChannel(Eye, Enemy->GetPawnViewLocation(), ECC_Visibility, SightQuery))
    continue;
   if (!Enemy->HealthDisplay->IsVisible())
    Enemy->HealthDisplay->SetVisibility(true);
   Visible.Add(Enemy);
  }
 }
 for (const auto& Previous : VisibleEnemyHealth)
  if (!Visible.Contains(Previous))
   if (auto* Enemy = Previous.Get())
    Enemy->HealthDisplay->SetVisibility(false);
 VisibleEnemyHealth = MoveTemp(Visible);
}

void UCC_PlayerScanComponent::EndPlay(const EEndPlayReason::Type Reason)
{
 UpdateEnemyHealthDisplays(false);
 Super::EndPlay(Reason);
}
AActor* UCC_PlayerScanComponent::GetTarget()const{return IsScanActive()&&Target.IsValid()&&!Target->IsHidden()?Target.Get():nullptr;}
UCC_ScanDescription* UCC_PlayerScanComponent::GetInfo()const{return GetTarget()?Info.Get():nullptr;}
bool UCC_PlayerScanComponent::HasInteraction()const{
 auto* A=GetTarget();return A&&(Cast<ACC_InteractiveDoubleDoor>(A)||Cast<AWFCMapGenerator>(A)||Cast<ACC_LevitationPlatform>(A)||Cast<ACC_ExperiencePack>(A)||A->FindComponentByClass<UCC_ArtifactComponent>());
}
bool UCC_PlayerScanComponent::CanInteractWithTarget()const{
 auto* A=GetTarget();const auto* PC=Cast<ACC_PlayerController>(GetOwner());auto* C=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;if(!A||!C)return false;
 if(auto* Generator=Cast<AWFCMapGenerator>(A))return PC->GetHeavenSceneTarget()==Generator&&Generator->CanRegenerateRandomScene();
 if(auto* Platform=Cast<ACC_LevitationPlatform>(A))return Platform->CanInteract(C);
 if(auto* Door=Cast<ACC_InteractiveDoubleDoor>(A))return Door->CanInteract(C);
 if(auto* Pack=Cast<ACC_ExperiencePack>(A))return Pack->CanInteract(C);
 if(auto* Artifact=A->FindComponentByClass<UCC_ArtifactComponent>())return Artifact->CanInteract(C);
 return false;
}

bool UCC_PlayerScanComponent::TraceCrosshair(FHitResult& Hit)const{
 const auto* PC=Cast<ACC_PlayerController>(GetOwner());APawn* Pawn=PC?PC->GetPawn():nullptr;if(!Pawn||!GetWorld())return false;
 FVector Eye;FRotator Rotation;PC->GetPlayerViewPoint(Eye,Rotation);const FVector End=Eye+Rotation.Vector()*FMath::Max(1.f,ScanDistance);
 FCollisionQueryParams Params;Params.AddIgnoredActor(Pawn);TArray<AActor*> Attached;Pawn->GetAttachedActors(Attached,true,true);Params.AddIgnoredActors(Attached);
 const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,Eye,End,ECC_Visibility,Params);
 // Pawn/CharacterMesh profiles intentionally ignore Visibility. Include the
 // nearest pawn separately, but stop at the first opaque-world obstruction.
 FHitResult PawnHit;FCollisionObjectQueryParams Pawns;Pawns.AddObjectTypesToQuery(ECC_Pawn);
 if(GetWorld()->LineTraceSingleByObjectType(PawnHit,Eye,Blocked?Hit.ImpactPoint:End,Pawns,Params)){
  if(!Blocked||PawnHit.Distance<=Hit.Distance+.1f){Hit=PawnHit;return true;}
 }
 return Blocked;
}

UCC_ScanDescription* UCC_PlayerScanComponent::ResolveInfoForActor(AActor* Actor)const{
if(!IsValid(Actor)||Actor->IsHidden())return nullptr;
 if(const auto* Enemy=Cast<ACC_EnemyCharacter>(Actor);Enemy&&!Enemy->IsAlive())return nullptr;
 if(const auto* Pack=Cast<ACC_ExperiencePack>(Actor);Pack&&Pack->bConsumed)return nullptr;
 if(const auto* Artifact=Actor->FindComponentByClass<UCC_ArtifactComponent>();Artifact&&(!Artifact->bEnabled||Artifact->bConsumed))return nullptr;
 UCC_ScanDescription* Found=nullptr;
 if(const auto* Component=Actor->FindComponentByClass<UCC_ScanTargetComponent>()){
  if(!Component->bEnabled)return nullptr;Found=Component->Info;
 }else if(Catalog)for(const auto& Rule:Catalog->Rules){
  if(!Rule.ActorClass&&!Rule.ComponentClass)continue;
  if(Rule.ActorClass&&!Actor->IsA(Rule.ActorClass))continue;
  if(Rule.ComponentClass&&!Actor->GetComponentByClass(Rule.ComponentClass))continue;
  Found=Rule.Info;break;
 }
 return Found;
}
