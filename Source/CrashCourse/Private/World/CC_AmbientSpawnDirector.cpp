#include "World/CC_AmbientSpawnDirector.h"
#include "World/CC_ShanheChunkWorld.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Controller.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"

ACC_AmbientSpawnDirector::ACC_AmbientSpawnDirector()
{
 PrimaryActorTick.bCanEverTick=false;
 bReplicates=false;
}
void ACC_AmbientSpawnDirector::BeginPlay()
{
 Super::BeginPlay();
 if(!HasAuthority() || GetNetMode()==NM_Client)return;
 // A second placed copy must never create a second population budget.
 for(TActorIterator<ACC_AmbientSpawnDirector> It(GetWorld());It;++It)
  if(*It!=this && It->HasActorBegunPlay() && It->bEnabled){bEnabled=false;return;}
 for(TActorIterator<APlayerStart> It(GetWorld());It;++It)ProtectedStarts.Add(It->GetActorLocation());
 if(bEnabled && bReplacePlacedPatrolEnemies)
 {
  TArray<ACC_PatrolEnemyCharacter*> Legacy;
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(GetWorld());It;++It)
   if(!It->ActorHasTag(TEXT("CC.KeepPlacedEnemy")) && !It->ActorHasTag(TEXT("CC.AmbientSpawned")))Legacy.Add(*It);
  for(auto* Enemy:Legacy)
  {
   auto* Controller=Enemy->GetController();Enemy->StopPatrol();Enemy->Destroy();
   if(Controller)Controller->Destroy();
  }
 }
 GetWorldTimerManager().SetTimer(SpawnTimer,this,&ThisClass::RunSpawnCycle,FMath::Max(1.f,SpawnInterval),true,FMath::Max(.1f,InitialDelay));
}
void ACC_AmbientSpawnDirector::EndPlay(const EEndPlayReason::Type Reason)
{
 GetWorldTimerManager().ClearTimer(SpawnTimer);
 // Only explicit removal needs cleanup; map teardown already owns its actors.
 if(HasAuthority() && Reason==EEndPlayReason::Destroyed)
  for(const auto& Weak:Managed)if(auto* Enemy=Weak.Get())
  {
   auto* Controller=Enemy->GetController();Enemy->Destroy();if(Controller)Controller->Destroy();
  }
 Super::EndPlay(Reason);
}
void ACC_AmbientSpawnDirector::GatherPlayers(TArray<ACC_PlayerCharacter*>& Players) const
{
 for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
  if(auto* PC=It->Get())if(auto* Player=Cast<ACC_PlayerCharacter>(PC->GetPawn());Player && Player->IsAlive())Players.Add(Player);
}
int32 ACC_AmbientSpawnDirector::GetManagedPopulation() const
{
 int32 Count=0;for(const auto& Weak:Managed)if(auto* Enemy=Weak.Get();Enemy && Enemy->IsAlive())++Count;return Count;
}
int32 ACC_AmbientSpawnDirector::CountNearby(FVector Position) const
{
 int32 Count=0;
 for(TActorIterator<ACC_PatrolEnemyCharacter> It(GetWorld());It;++It)
  if(It->IsAlive() && FVector::DistSquared2D(Position,It->GetActorLocation())<FMath::Square(LocalPopulationRadius))++Count;
 return Count;
}
bool ACC_AmbientSpawnDirector::IsVisibleToAnyPlayer(FVector Position,const TArray<ACC_PlayerCharacter*>& Players,const AActor* Subject) const
{
 for(const auto* Player:Players)if(const auto* PC=Cast<APlayerController>(Player->GetController()))
 {
  FVector Eye;FRotator Facing;PC->GetPlayerViewPoint(Eye,Facing);
  const FVector Delta=Position+FVector(0,0,70)-Eye;
  // A wide horizontal guard includes the edge of third-person view.
  if(FVector::DotProduct(Facing.Vector().GetSafeNormal2D(),Delta.GetSafeNormal2D())<.35f)continue;
  FCollisionQueryParams Params(SCENE_QUERY_STAT(AmbientSpawnVisibility),false,Player);
  if(Subject)Params.AddIgnoredActor(Subject);
  if(!GetWorld()->LineTraceTestByChannel(Eye,Position+FVector(0,0,70),ECC_Visibility,Params))return true;
 }
 return false;
}
bool ACC_AmbientSpawnDirector::IsPositionAllowed(FVector Position,const TArray<ACC_PlayerCharacter*>& Players) const
{
 for(const auto* Player:Players)
  if(FVector::DistSquared2D(Position,Player->GetActorLocation())<FMath::Square(MinimumPlayerDistance))return false;
 for(FVector Start:ProtectedStarts)
  if(FVector::DistSquared2D(Position,Start)<FMath::Square(StartProtectionRadius))return false;
 for(TActorIterator<ACC_PatrolEnemyCharacter> It(GetWorld());It;++It)
  if(It->IsAlive() && FVector::DistSquared2D(Position,It->GetActorLocation())<FMath::Square(MinimumEnemySpacing))return false;
 return !bAvoidVisibleSpawns || !IsVisibleToAnyPlayer(Position,Players);
}
bool ACC_AmbientSpawnDirector::FindSpawnPosition(ACC_PlayerCharacter* Around,const TArray<ACC_PlayerCharacter*>& Players,FVector& OutPosition,const FVector* PackAnchor) const
{
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 if(!Nav || !Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))return false;
 const float Inner=FMath::Max(100.f,MinimumPlayerDistance),Outer=FMath::Max(Inner,MaximumSpawnDistance);
 // sqrt gives uniform area density within the annulus.
 const float Radius=FMath::Sqrt(FMath::FRandRange(Inner*Inner,Outer*Outer));
 const float Angle=FMath::FRandRange(0.f,UE_TWO_PI);
 const float PackRadius=FMath::FRandRange(FMath::Max(150.f,MinimumEnemySpacing),800.f);
 const FVector Candidate=(PackAnchor?*PackAnchor:Around->GetActorLocation())+FVector(FMath::Cos(Angle)*(PackAnchor?PackRadius:Radius),FMath::Sin(Angle)*(PackAnchor?PackRadius:Radius),0);
 FNavLocation Projected;
 if(!Nav->ProjectPointToNavigation(Candidate,Projected,FVector(100,100,400)))return false;
 if(FVector::DistSquared2D(Projected.Location,Around->GetActorLocation())>FMath::Square(Outer))return false;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(AmbientSpawnGround),false);
 for(const auto* Player:Players)Params.AddIgnoredActor(Player);
 FHitResult Ground;
 if(!GetWorld()->LineTraceSingleByChannel(Ground,Projected.Location+FVector(0,0,160),Projected.Location-FVector(0,0,260),ECC_WorldStatic,Params))return false;
 if(Ground.ImpactNormal.Z<.72f || Cast<APawn>(Ground.GetActor()) || (Ground.GetActor() && Ground.GetActor()->ActorHasTag(TEXT("NoAmbientSpawn"))))return false;
 const FVector Position=Ground.ImpactPoint+FVector(0,0,98);
 if(!IsPositionAllowed(Position,Players))return false;
 if(GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43.f,97.f),Params))return false;
 auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),Around->GetActorLocation(),Ground.ImpactPoint,Around);
 if(!Path || !Path->IsValid() || Path->IsPartial())return false;
 OutPosition=Position;return true;
}
int32 ACC_AmbientSpawnDirector::HighestOwnedTier(const TArray<ACC_PlayerCharacter*>& Players) const
{
 int32 Tier=0;
 for(const auto* Player:Players)if(const auto* PS=Player->GetPlayerState<ACC_PlayerState>();PS && PS->Progression)
  for(const auto& Form:PS->Progression->Forms)Tier=FMath::Max(Tier,Form.Tier);
 return Tier;
}
const FCC_AmbientSpawnEntry* ACC_AmbientSpawnDirector::PickEntry(int32 Tier) const
{
 TArray<const FCC_AmbientSpawnEntry*> Eligible;float Total=0;
 for(const auto& Entry:Entries)
 {
  if(!Entry.EnemyClass || Entry.Weight<=0 || Entry.MinimumDifficultyTier>Tier || Entry.MinimumDifficultyTier<FMath::Max(0,Tier-1))continue;
  int32 Count=0;for(const auto& Weak:Managed)if(auto* E=Weak.Get();E && E->IsAlive() && E->IsA(Entry.EnemyClass))++Count;
  if(Count>=FMath::Max(1,Entry.ClassLimit))continue;
  Eligible.Add(&Entry);Total+=Entry.Weight*(Entry.MinimumDifficultyTier==Tier?3.f:1.f);
 }
 float Roll=FMath::FRand()*Total;
 for(const auto* Entry:Eligible){Roll-=Entry->Weight*(Entry->MinimumDifficultyTier==Tier?3.f:1.f);if(Roll<=0)return Entry;}
 return Eligible.IsEmpty()?nullptr:Eligible.Last();
}
void ACC_AmbientSpawnDirector::PrunePopulation(const TArray<ACC_PlayerCharacter*>& Players)
{
 const float Now=GetWorld()->GetTimeSeconds();
 ACC_ShanheChunkWorld* Terrain=nullptr;for(TActorIterator<ACC_ShanheChunkWorld> It(GetWorld());It;++It){Terrain=*It;break;}
 for(int32 Index=Managed.Num()-1;Index>=0;--Index)
 {
  const auto Weak=Managed[Index];auto* Enemy=Weak.Get();
  if(!Enemy || !Enemy->IsAlive()){OutsideSince.Remove(Weak);Managed.RemoveAtSwap(Index);continue;}
  float Nearest=MAX_flt;
  for(const auto* Player:Players)Nearest=FMath::Min(Nearest,FVector::DistSquared2D(Player->GetActorLocation(),Enemy->GetActorLocation()));
  bool bRemove=Players.IsEmpty() || (Terrain && !Terrain->IsPositionLoaded(Enemy->GetActorLocation())) || Nearest>FMath::Square(FMath::Max(SoftDespawnDistance,HardDespawnDistance));
  if(!bRemove && Nearest>FMath::Square(SoftDespawnDistance))
  {
   const float* Since=OutsideSince.Find(Weak);
   if(!Since)OutsideSince.Add(Weak,Now);
   else bRemove=Now-*Since>=SoftDespawnDelay && !IsVisibleToAnyPlayer(Enemy->GetActorLocation(),Players,Enemy);
  }
  else if(!bRemove)OutsideSince.Remove(Weak);
  if(!bRemove)continue;
  auto* Controller=Enemy->GetController();Enemy->StopPatrol();Enemy->Destroy();if(Controller)Controller->Destroy();
  OutsideSince.Remove(Weak);Managed.RemoveAtSwap(Index);++TotalDespawned;
 }
}
void ACC_AmbientSpawnDirector::RefreshPopulationBounds()
{
 if(!HasAuthority() || GetNetMode()==NM_Client)return;
 TArray<ACC_PlayerCharacter*> Players;GatherPlayers(Players);PrunePopulation(Players);
}
void ACC_AmbientSpawnDirector::RunSpawnCycle()
{
 if(!HasAuthority() || GetNetMode()==NM_Client)return;
 TArray<ACC_PlayerCharacter*> Players;GatherPlayers(Players);PrunePopulation(Players);
 if(!bEnabled || Players.IsEmpty() || Entries.IsEmpty())return;
 if(GetWorld()->IsPaused())return;
 ACC_ShanheChunkWorld* Terrain=nullptr;for(TActorIterator<ACC_ShanheChunkWorld> It(GetWorld());It;++It){Terrain=*It;break;}
 int32 Budget=FMath::Min(FMath::Max(0,MaxSpawnsPerCycle),FMath::Max(0,GlobalPopulationCap-GetManagedPopulation()));
 // Randomized player order avoids the first controller owning the whole budget.
 const int32 First=FMath::RandHelper(Players.Num());
 for(int32 PlayerIndex=0;PlayerIndex<Players.Num() && Budget>0;++PlayerIndex)
 {
  auto* Player=Players[(PlayerIndex+First)%Players.Num()];
  if(const auto* PC=Cast<ACC_PlayerController>(Player->GetController());PC && PC->IsGameplayInputBlocked())continue;
  int32 PackBudget=FMath::Min(Budget,FMath::Min(FMath::RandRange(1,3),FMath::Max(0,LocalPopulationCap-CountNearby(Player->GetActorLocation()))));
  FVector Anchor;bool bHasAnchor=false;
  for(int32 Attempt=0;Attempt<FMath::Clamp(AttemptsPerCycle,1,64) && PackBudget>0 && Budget>0;++Attempt)
  {
   FVector Position;if(!FindSpawnPosition(Player,Players,Position,bHasAnchor?&Anchor:nullptr))continue;
   if(Terrain && !Terrain->IsPositionLoaded(Position))continue;
   const int32 Ring=Terrain?Terrain->DifficultyRingForPosition(Position):0;
   const int32 Tier=Terrain?(Ring>=4?2:(Ring>=2?1:0)):HighestOwnedTier(Players);
   const auto* Entry=PickEntry(Tier);if(!Entry)continue;
   const FTransform Transform(FRotator(0,FMath::FRandRange(-180.f,180.f),0),Position);
   auto* Enemy=GetWorld()->SpawnActorDeferred<ACC_PatrolEnemyCharacter>(Entry->EnemyClass,Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
   if(!Enemy)continue;
   Enemy->Tags.Add(TEXT("CC.AmbientSpawned"));
   Enemy->SetReplicates(true);
   Enemy->FinishSpawning(Transform);
   if(Terrain && Ring>4)if(auto* ASC=Enemy->GetAbilitySystemComponent())
   {
    const float Health=ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute())*FMath::Min(3.f,1.f+.12f*(Ring-4));
    ASC->SetNumericAttributeBase(UCC_AttributeSet::GetMaxHealthAttribute(),Health);
    ASC->SetNumericAttributeBase(UCC_AttributeSet::GetHealthAttribute(),Health);
    ASC->SetNumericAttributeBase(UCC_AttributeSet::GetAttackAttribute(),ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute())*FMath::Min(1.75f,1.f+.04f*(Ring-4)));
    Enemy->ExperienceReward=FMath::RoundToInt(Enemy->ExperienceReward*FMath::Min(2.f,1.f+.1f*(Ring-4)));
   }
   Enemy->Tags.Add(FName(*FString::Printf(TEXT("CC.WildRing.%d"),Ring)));
   if(!bHasAnchor){Anchor=Position;bHasAnchor=true;}
   Managed.Add(Enemy);++TotalSpawned;--Budget;--PackBudget;
  }
 }
}
