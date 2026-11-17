#include "World/CC_RoomVisibilityController.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DecalActor.h"
#include "Engine/DirectionalLight.h"

ACC_RoomVisibilityController::ACC_RoomVisibilityController()
{
 PrimaryActorTick.bCanEverTick=false;
 TriggerBox=CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
 SetRootComponent(TriggerBox);
 TriggerBox->InitBoxExtent(FVector(600,600,300));
 TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
 TriggerBox->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
 TriggerBox->SetGenerateOverlapEvents(true);
 TriggerBox->SetCanEverAffectNavigation(false);
}

void ACC_RoomVisibilityController::CollectActorsInsideBox()
{
#if WITH_EDITOR
 UWorld* World=GetWorld();
 if(!World || World->WorldType!=EWorldType::Editor || !TriggerBox) return;

 TSet<const AActor*> Claimed;
 for(TActorIterator<ACC_RoomVisibilityController> It(World);It;++It)
 {
  const ACC_RoomVisibilityController* Other=*It;
  if(Other==this) continue;
  for(const AActor* Actor : Other->bUseBoxSelection ? Other->BoxActors : Other->InteriorActors) Claimed.Add(Actor);
  for(const ALight* Light : Other->bUseBoxSelection ? Other->BoxLights : Other->RoomLights) Claimed.Add(Light);
  for(const AActor* Effect : Other->bUseBoxSelection ? Other->BoxEffects : Other->RoomEffects) Claimed.Add(Effect);
 }

 Modify();
 BoxActors.Reset();
 BoxLights.Reset();
 BoxEffects.Reset();
 const FTransform Transform=TriggerBox->GetComponentTransform();
 const FVector Extent=TriggerBox->GetUnscaledBoxExtent();
 const FBox LocalBox(-Extent,Extent);
 for(TActorIterator<AActor> It(World);It;++It)
 {
  AActor* Actor=*It;
  if(Actor==this || Actor->IsA<APawn>() || Actor->IsA<ADirectionalLight>() || Claimed.Contains(Actor)) continue;
  ALight* Light=Cast<ALight>(Actor);
  const bool bHasEffect=Actor->FindComponentByClass<UNiagaraComponent>() ||
                        Actor->FindComponentByClass<UParticleSystemComponent>();
  if(!Light && !bHasEffect && !Actor->IsA<AStaticMeshActor>() && !Actor->IsA<ADecalActor>()) continue;

  bool bInside=LocalBox.IsInsideOrOn(Transform.InverseTransformPosition(Actor->GetActorLocation()));
  if(!Light && !bHasEffect)
  {
   const FBox Bounds=Actor->GetComponentsBoundingBox(true);
   bInside=Bounds.IsValid!=0;
   for(int32 Corner=0;Corner<8 && bInside;++Corner)
   {
    const FVector Point(Corner&1 ? Bounds.Max.X : Bounds.Min.X,
                        Corner&2 ? Bounds.Max.Y : Bounds.Min.Y,
                        Corner&4 ? Bounds.Max.Z : Bounds.Min.Z);
    bInside=LocalBox.IsInsideOrOn(Transform.InverseTransformPosition(Point));
   }
  }
  if(!bInside) continue;
  if(Light) BoxLights.Add(Light);
  else if(bHasEffect) BoxEffects.Add(Actor);
  else BoxActors.Add(Actor);
 }
 MarkPackageDirty();
#endif
}

void ACC_RoomVisibilityController::BeginPlay()
{
 Super::BeginPlay();
 // Allow the pawn and particle components to finish initialization first.
 SetAutomaticPlayerDetection(bAutomaticPlayerDetection);
}

void ACC_RoomVisibilityController::SetAutomaticPlayerDetection(bool bEnabled)
{
 bAutomaticPlayerDetection=bEnabled;
 GetWorldTimerManager().ClearTimer(PollTimer);
 if(bEnabled && HasActorBegunPlay()) GetWorldTimerManager().SetTimer(PollTimer,this,&ThisClass::RefreshPlayerOverlap,0.25f,true,0.1f);
}

void ACC_RoomVisibilityController::EndPlay(const EEndPlayReason::Type Reason)
{
 GetWorldTimerManager().ClearTimer(PollTimer);
 // Restore visibility if just this controller is removed while the level stays loaded.
 if(Reason==EEndPlayReason::Destroyed || Reason==EEndPlayReason::RemovedFromWorld) SetRoomActive(true);
 Super::EndPlay(Reason);
}

void ACC_RoomVisibilityController::RefreshPlayerOverlap()
{
 const APawn* Player=UGameplayStatics::GetPlayerPawn(this,0);
 if(!Player) return;
 // Check the player's position geometrically: also works if its overlap events are disabled.
 const FVector Local=TriggerBox->GetComponentTransform().InverseTransformPosition(Player->GetActorLocation());
 const FVector Extent=TriggerBox->GetUnscaledBoxExtent();
 SetRoomActive(FMath::Abs(Local.X)<=Extent.X && FMath::Abs(Local.Y)<=Extent.Y && FMath::Abs(Local.Z)<=Extent.Z);
}

void ACC_RoomVisibilityController::ApplyActorVisibility(AActor* Target,bool bActive,bool bManageTick)
{
 if(!IsValid(Target) || Target==this || Target->IsA<APawn>()) return;
 const TWeakObjectPtr<AActor> Key(Target);
 if(!ActorStates.Contains(Key)) ActorStates.Add(Key,{Target->IsHidden(),Target->IsActorTickEnabled()});
 const FActorState& State=ActorStates.FindChecked(Key);
 Target->SetActorHiddenInGame(bActive ? State.Hidden : true);
 if(bManageTick) Target->SetActorTickEnabled(bActive && State.Tick);
}

void ACC_RoomVisibilityController::SetRoomActive(bool bActive)
{
 if(bApplied && bRoomActive==bActive) return;
 bApplied=true; bRoomActive=bActive;
 for(AActor* Target : bUseBoxSelection ? BoxActors : InteriorActors) ApplyActorVisibility(Target,bActive,bManageInteriorActorTick);
 for(ALight* Light : bUseBoxSelection ? BoxLights : RoomLights)
 {
  if(!IsValid(Light)) continue;
  ULightComponent* Component=Light->GetLightComponent();
  if(!Component) continue;
  const TWeakObjectPtr<USceneComponent> Key(Component);
  if(!LightStates.Contains(Key)) LightStates.Add(Key,Component->IsVisible());
  Component->SetVisibility(bActive && LightStates.FindChecked(Key));
 }
 for(AActor* Target : bUseBoxSelection ? BoxEffects : RoomEffects)
 {
  if(!IsValid(Target) || Target==this || Target->IsA<APawn>()) continue;
  ApplyActorVisibility(Target,bActive,false);
  const bool bEnable=bActive && !ActorStates.FindChecked(TWeakObjectPtr<AActor>(Target)).Hidden;
  TInlineComponentArray<UNiagaraComponent*> Niagara(Target);
  for(UNiagaraComponent* Component:Niagara)
  {
   const TWeakObjectPtr<UActorComponent> Key(Component);
   if(!EffectStates.Contains(Key))EffectStates.Add(Key,Component->IsActive());
   if(bEnable && EffectStates.FindChecked(Key))Component->Activate(false);else Component->Deactivate();
  }
  TInlineComponentArray<UParticleSystemComponent*> Cascade(Target);
  for(UParticleSystemComponent* Component:Cascade)
  {
   const TWeakObjectPtr<UActorComponent> Key(Component);
   if(!EffectStates.Contains(Key))EffectStates.Add(Key,Component->IsActive());
   if(bEnable && EffectStates.FindChecked(Key))Component->Activate(false);else Component->Deactivate();
  }
 }
}
