#include "World/CC_CoinSpawnDirector.h"
#include "World/CC_ShanheChunkWorld.h"
#include "Progression/CC_CoinPickup.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"

void ACC_CoinSpawnDirector::BeginPlay()
{
 Super::BeginPlay();if(!HasAuthority()||GetNetMode()==NM_Client)return;
 for(TActorIterator<ACC_CoinPickup> It(GetWorld());It;++It)It->CopperAmount=5;
 GetWorldTimerManager().SetTimer(SpawnTimer,this,&ThisClass::UpdatePopulation,FMath::Max(2.f,SpawnInterval),true,5.f);
}
void ACC_CoinSpawnDirector::EndPlay(const EEndPlayReason::Type Reason)
{
 GetWorldTimerManager().ClearTimer(SpawnTimer);Super::EndPlay(Reason);
}
void ACC_CoinSpawnDirector::UpdatePopulation()
{
 TArray<ACC_PlayerCharacter*> Players;
 for(TActorIterator<ACC_PlayerCharacter> It(GetWorld());It;++It)if(It->IsAlive()&&It->IsPlayerControlled())Players.Add(*It);
 if(Players.IsEmpty())return;
 ACC_ShanheChunkWorld* Chunks=nullptr;for(TActorIterator<ACC_ShanheChunkWorld> It(GetWorld());It;++It){Chunks=*It;break;}
 for(auto& Weak:Spawned)if(auto* Coin=Weak.Get())
 {
  bool Near=false;for(auto* Player:Players)Near|=FVector::DistSquared2D(Player->GetActorLocation(),Coin->GetActorLocation())<FMath::Square(5000.f);
  if(!Near||(Chunks&&!Chunks->IsPositionLoaded(Coin->GetActorLocation())))
  {auto* Controller=Coin->GetController();Coin->Destroy();if(Controller)Controller->Destroy();}
 }
 Spawned.RemoveAll([](const auto& Weak){return !Weak.IsValid();});
 TArray<ACC_CoinPickup*> Coins;for(TActorIterator<ACC_CoinPickup> It(GetWorld());It;++It)Coins.Add(*It);
 if(Coins.Num()>=FMath::Clamp(PopulationCap,0,12))return;
 UClass* Class=CoinClass.LoadSynchronous();if(!Class)return;
 auto* Player=Players[FMath::RandHelper(Players.Num())];auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 for(int32 Attempt=0;Attempt<16;++Attempt)
 {
  const float Angle=FMath::FRandRange(0.f,2.f*PI),Radius=FMath::FRandRange(900.f,2400.f);
  FVector Position=Player->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
  if(Chunks&&!Chunks->IsPositionLoaded(Position))continue;
  FNavLocation Projected;if(Nav&&Nav->ProjectPointToNavigation(Position,Projected,FVector(180,180,400)))Position=Projected.Location;
  FHitResult Floor;FCollisionQueryParams Params(SCENE_QUERY_STAT(CoinSpawn),false);Params.AddIgnoredActor(Player);
  if(!GetWorld()->LineTraceSingleByChannel(Floor,Position+FVector(0,0,400),Position-FVector(0,0,800),ECC_Visibility,Params)||Floor.ImpactNormal.Z<.75f)continue;
  Position=Floor.ImpactPoint+FVector(0,0,Class->GetDefaultObject<ACC_CoinPickup>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3);
  bool Clear=true;for(auto* Existing:Coins)Clear&=FVector::DistSquared2D(Position,Existing->GetActorLocation())>FMath::Square(400.f);
  for(auto* Hero:Players)Clear&=FVector::DistSquared2D(Position,Hero->GetActorLocation())>FMath::Square(700.f);
  if(!Clear)continue;
  FActorSpawnParameters Spawn;Spawn.Owner=this;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
  if(auto* Coin=GetWorld()->SpawnActor<ACC_CoinPickup>(Class,Position,FRotator::ZeroRotator,Spawn)){Coin->CopperAmount=5;Spawned.Add(Coin);return;}
 }
}
