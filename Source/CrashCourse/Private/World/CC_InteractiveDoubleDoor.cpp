#include "World/CC_InteractiveDoubleDoor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

ACC_InteractiveDoubleDoor::ACC_InteractiveDoubleDoor()
{
 PrimaryActorTick.bCanEverTick=true;
 PrimaryActorTick.bStartWithTickEnabled=false;
}
void ACC_InteractiveDoubleDoor::BeginPlay()
{
 Super::BeginPlay();
 TInlineComponentArray<USceneComponent*> Components(this);
 for(USceneComponent* C:Components)
 {
  if(C->GetFName()==LeftDoorComponentName) LeftDoor=C;
  if(C->GetFName()==RightDoorComponentName) RightDoor=C;
 }
 if(LeftDoor && RightDoor)
 {
  LeftClosed=LeftDoor->GetRelativeRotation();RightClosed=RightDoor->GetRelativeRotation();
  LeftDoor->SetMobility(EComponentMobility::Movable);RightDoor->SetMobility(EComponentMobility::Movable);
 }
 else UE_LOG(LogTemp,Warning,TEXT("Door %s: configured door components were not found."),*GetName());
}
bool ACC_InteractiveDoubleDoor::CanInteract(const APawn* Player) const
{
 if(!IsValid(Player) || !Player->IsPlayerControlled() || bAnimating || !LeftDoor || !RightDoor || IsHidden()) return false;
 const FBox Box=GetComponentsBoundingBox(true);
 const FVector Point=Box.IsValid?Box.GetClosestPointTo(Player->GetActorLocation()):GetActorLocation();
 if(FVector::DistSquared(Point,Player->GetActorLocation())>FMath::Square(InteractionDistance)) return false;
 FCollisionQueryParams Params;Params.AddIgnoredActor(Player);
 TArray<AActor*> Attached;Player->GetAttachedActors(Attached,true,true);Params.AddIgnoredActors(Attached);
 for(const USceneComponent* Leaf:{LeftDoor.Get(),RightDoor.Get()})
 {
  const auto* Primitive=Cast<UPrimitiveComponent>(Leaf);
  const FVector Target=Primitive?Primitive->Bounds.Origin:Leaf->GetComponentLocation();
  FHitResult Hit;
  if(!GetWorld()->LineTraceSingleByChannel(Hit,Player->GetPawnViewLocation(),Target,ECC_Visibility,Params) || Hit.GetActor()==this)return true;
 }
 return false;
}
bool ACC_InteractiveDoubleDoor::Interact(APawn* Player)
{
 if(!CanInteract(Player)) return false;
 bOpen=!bOpen;bAnimating=true;
 // Consumers can restore a room before the first opening frame, and hide only after closing.
 if(bOpen) OnDoorOpening.Broadcast();
 SetActorTickEnabled(true);return true;
}
void ACC_InteractiveDoubleDoor::ApplyDoorRotation()
{
 const float Smooth=Alpha*Alpha*(3.f-2.f*Alpha);
 FRotator L=LeftClosed,R=RightClosed;L.Yaw+=LeftOpenAngle*Smooth;R.Yaw+=RightOpenAngle*Smooth;
 LeftDoor->SetRelativeRotation(L);RightDoor->SetRelativeRotation(R);
}
void ACC_InteractiveDoubleDoor::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if(!bAnimating || !LeftDoor || !RightDoor){SetActorTickEnabled(false);return;}
 Alpha=FMath::Clamp(Alpha+(bOpen?1.f:-1.f)*DeltaSeconds/FMath::Max(OpenDuration,.05f),0.f,1.f);
 ApplyDoorRotation();
 if((bOpen && Alpha>=1.f)||(!bOpen && Alpha<=0.f))
 {bAnimating=false;SetActorTickEnabled(false);if(!bOpen) OnDoorClosed.Broadcast();}
}
