#include "World/CC_DoorVisibilityManager.h"
#include "World/CC_InteractiveDoubleDoor.h"
#include "World/CC_RoomVisibilityController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

ACC_DoorVisibilityManager::ACC_DoorVisibilityManager(){PrimaryActorTick.bCanEverTick=false;}
void ACC_DoorVisibilityManager::BeginPlay()
{
 Super::BeginPlay();
 if(ExteriorDetails)ExteriorDetails->SetAutomaticPlayerDetection(false);
 for(auto& R:Rooms)
 {
  if(R.Visibility)R.Visibility->SetAutomaticPlayerDetection(false);
  if(R.Door){R.Door->OnDoorOpening.AddDynamic(this,&ThisClass::OnDoorStateChanged);R.Door->OnDoorClosed.AddDynamic(this,&ThisClass::OnDoorStateChanged);}
 }
 GetWorldTimerManager().SetTimer(RefreshTimer,this,&ThisClass::RefreshVisibility,.1f,true,.2f);
}
void ACC_DoorVisibilityManager::ShowAll()
{
 if(ExteriorDetails)ExteriorDetails->SetRoomActive(true);
 for(auto& R:Rooms)if(R.Visibility)R.Visibility->SetRoomActive(true);
}
void ACC_DoorVisibilityManager::SetOptimizationEnabled(bool bEnabled)
{bOptimizationEnabled=bEnabled;RefreshVisibility();}
void ACC_DoorVisibilityManager::OnDoorStateChanged(){RefreshVisibility();}
void ACC_DoorVisibilityManager::RefreshVisibility()
{
 APawn* Player=UGameplayStatics::GetPlayerPawn(this,0);
 if(!bOptimizationEnabled || !Player || Rooms.IsEmpty()){ShowAll();return;}
 OccupiedRoom=INDEX_NONE;bool AnyOpen=false;
 for(int32 I=0;I<Rooms.Num();++I)
 {
  const auto& R=Rooms[I];
  // Invalid configuration fails open rather than leaving geometry invisible.
  if(!IsValid(R.Door)||!IsValid(R.Visibility)){ShowAll();return;}
  const FVector P=(Player->GetActorLocation()-R.Center).GetAbs();
  if(P.X<=R.HalfExtent.X && P.Y<=R.HalfExtent.Y && P.Z<=R.HalfExtent.Z)OccupiedRoom=I;
  AnyOpen |= R.Door->bOpen || R.Door->bAnimating;
 }
 if(ExteriorDetails)ExteriorDetails->SetRoomActive(AnyOpen || OccupiedRoom==INDEX_NONE);
 for(int32 I=0;I<Rooms.Num();++I)
 {
  const auto& R=Rooms[I];
  R.Visibility->SetRoomActive(I==OccupiedRoom || R.Door->bOpen || R.Door->bAnimating);
 }
}
void ACC_DoorVisibilityManager::EndPlay(const EEndPlayReason::Type Reason)
{
 GetWorldTimerManager().ClearTimer(RefreshTimer);
 for(auto& R:Rooms)if(IsValid(R.Door)){R.Door->OnDoorOpening.RemoveDynamic(this,&ThisClass::OnDoorStateChanged);R.Door->OnDoorClosed.RemoveDynamic(this,&ThisClass::OnDoorStateChanged);}
 if(Reason==EEndPlayReason::Destroyed || Reason==EEndPlayReason::RemovedFromWorld)ShowAll();
 Super::EndPlay(Reason);
}
