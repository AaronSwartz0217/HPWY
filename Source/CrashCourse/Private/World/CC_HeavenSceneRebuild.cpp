#include "World/CC_HeavenSceneRebuild.h"
#include "WFCMapGenerator.h"
#include "WFCTileSet.h"
#include "World/CC_AdventureGameInstance.h"
#include "World/CC_LevitationPlatform.h"
#include "World/CC_HeavenMazeExit.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
namespace CCHeavenScene {
bool IsHeaven(UWorld* W){auto* GI=W?Cast<UCC_AdventureGameInstance>(W->GetGameInstance()):nullptr;return GI&&GI->RegionForMap(UGameplayStatics::GetCurrentLevelName(W,true))==TEXT("Heaven");}
AWFCMapGenerator* FindGenerator(UWorld* W){if(W)for(TActorIterator<AWFCMapGenerator> I(W);I;++I)if(I->TileSet)return *I;return nullptr;}
namespace
{
const FName MazeEnemyTag(TEXT("CC.HeavenMazeEnemy"));

void ClearMazeEnemies(UWorld* World)
{
 for (TActorIterator<ACC_PatrolEnemyCharacter> It(World); It; ++It)
 {
  if (!It->ActorHasTag(MazeEnemyTag))
   continue;
  AController* Controller = It->GetController();
  It->StopPatrol();
  It->Destroy();
  if (IsValid(Controller))
   Controller->Destroy();
 }
}

void PopulateMaze(AWFCMapGenerator& Generator, FIntVector Entrance)
{
 UWorld* World = Generator.GetWorld();
 const TCHAR* Root = TEXT("/Game/HPWH/Spawning/HeavenMaze/");
 TSubclassOf<ACC_PatrolEnemyCharacter> Classes[3];
 for (int32 Tier = 0; Tier < UE_ARRAY_COUNT(Classes); ++Tier)
 {
  const FString Name = FString::Printf(TEXT("BP_MazeBronze_T%d"), Tier + 1);
  Classes[Tier] = LoadClass<ACC_PatrolEnemyCharacter>(nullptr, *(FString(Root) + Name + TEXT(".") + Name + TEXT("_C")));
  if (!Classes[Tier])
  {
   UE_LOG(LogTemp, Error, TEXT("Heaven maze enemy class is missing: %s"), *Name);
   return;
  }
 }

 FRandomStream Random(Generator.RuntimeSeed ^ 0x156C0273);
 for (const auto& Cell : Generator.DisplayedResult.Cells)
 {
  const FIntVector Offset = Cell.Coordinate - Entrance;
  const bool bSafeEntrance = FMath::Abs(Offset.X) <= 1 && FMath::Abs(Offset.Y) <= 1;
  if (Cell.Coordinate.Z != 0 || Cell.TileId != TEXT("MazePath") || bSafeEntrance || Random.FRand() >= .30f)
   continue;

  const float TierRoll = Random.FRand();
  const auto EnemyClass = Classes[TierRoll < .60f ? 0 : TierRoll < .90f ? 1 : 2];
  const auto* Defaults = EnemyClass->GetDefaultObject<ACC_PatrolEnemyCharacter>();
  FVector Position = Generator.GetActorTransform().TransformPosition(FVector(Cell.Coordinate) * Generator.TileSet->CellSize);
  FHitResult Floor;
  if (World->LineTraceSingleByChannel(Floor, Position + FVector(0, 0, 80), Position - FVector(0, 0, 100), ECC_Visibility)
      && Floor.GetActor() == &Generator)
   Position.Z = Floor.ImpactPoint.Z;
  Position.Z += Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 4.f;
  const FTransform Transform(FRotator(0, Random.FRandRange(-180.f, 180.f), 0), Position);
  if (auto* Enemy = World->SpawnActorDeferred<ACC_PatrolEnemyCharacter>(EnemyClass, Transform, &Generator,
         nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding))
  {
   Enemy->Tags.Add(MazeEnemyTag);
   Enemy->bUseNavMeshForPatrol = false;
   Enemy->FinishSpawning(Transform);
  }
 }
}

bool Generate(UWorld* W, bool bToggleMaze, bool bEntering){
 auto* Generator=FindGenerator(W);if(!Generator||!Generator->HasAuthority()||!Generator->CanRegenerateRandomScene())return false;
 struct FPassenger{ACC_PlayerCharacter* Character;ACC_LevitationPlatform* Platform;FVector Relative;};TArray<FPassenger> Players;TArray<ACC_LevitationPlatform*> Platforms;
 for(TActorIterator<ACC_LevitationPlatform> I(W);I;++I)Platforms.Add(*I);
 for(TActorIterator<ACC_PlayerCharacter> I(W);I;++I)if(I->IsAlive()){
  ACC_LevitationPlatform* Base=nullptr;for(auto* P:Platforms)if(P->IsCarrying(*I)){Base=P;break;}
  Players.Add({*I,Base,Base?Base->GetActorTransform().InverseTransformPosition(I->GetActorLocation()):FVector::ZeroVector});
 }

 const bool bWasMaze = Generator->DisplayedSettings.LayoutPreset == EWFCLayoutPreset::Maze;
 const bool bMaze = bEntering || (bToggleMaze ? !bWasMaze : bWasMaze);
 const auto Preset = bMaze ? EWFCLayoutPreset::Maze : EWFCLayoutPreset::Settlement;
 if (!Generator->RegenerateScenePreset(Preset))
  return false;

 for (TActorIterator<ACC_HeavenMazeExit> It(W); It; ++It)
  It->Destroy();
 ClearMazeEnemies(W);
 if (bMaze)
 {
  FVector Entry = Generator->GetActorLocation(), Exit = Entry;
  FIntVector EntranceCell = FIntVector::ZeroValue;
  for (const auto& Cell : Generator->DisplayedResult.Cells)
  {
   const FVector Position = Generator->GetActorTransform().TransformPosition(FVector(Cell.Coordinate) * Generator->TileSet->CellSize);
   if (Cell.TileId == TEXT("MazeEntry"))
   {
    Entry = Position;
    EntranceCell = Cell.Coordinate;
   }
   if (Cell.TileId == TEXT("MazeExit")) Exit = Position;
  }

  // Park the lift below the floor so it cannot give access to wall tops.
  for (auto* Platform : Platforms)
  {
   Platform->SetActorLocation(Entry - FVector(0, 0, 5000), false, nullptr, ETeleportType::TeleportPhysics);
   Platform->ForceNetUpdate();
  }
  for (int32 Index = 0; Index < Players.Num(); ++Index)
  {
   auto* Character = Players[Index].Character;
   Character->SetBase(nullptr);
   Character->GetCharacterMovement()->StopMovementImmediately();
   // Start inside the 3x3 courtyard. Offset nearby players so capsules do not stack.
   const FIntPoint Offset(Index % 9 % 3 - 1, Index % 9 / 3 - 1);
   const FVector LocalOffset(Offset.X * 210, Offset.Y * 210, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 8);
   const FVector Position = Entry + Generator->GetActorTransform().TransformVectorNoScale(LocalOffset);
   const FRotator Rotation(0, Generator->GetActorRotation().Yaw, 0);
   Character->TeleportTo(Position, Rotation, false, true);
   Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
   Character->ForceNetUpdate();
  }

  // Enable the exit only once the previous maze occupants have returned to the court.
  const FTransform ExitTransform(Generator->GetActorRotation(), Exit + FVector(0,0,105));
  if (auto* Goal = W->SpawnActorDeferred<ACC_HeavenMazeExit>(ACC_HeavenMazeExit::StaticClass(), ExitTransform))
  {
   Goal->Revision = Generator->RuntimeRevision;
   Goal->FinishSpawning(ExitTransform);
  }
  PopulateMaze(*Generator, EntranceCell);
  return true;
 }

 FVector Center=FVector::ZeroVector;int32 Count=0;
 for(const auto& Cell:Generator->DisplayedResult.Cells)if(Cell.Coordinate.Z==0&&Cell.TileId.ToString().StartsWith(TEXT("Plaza_"))){Center+=FVector(Cell.Coordinate)*Generator->TileSet->CellSize;++Count;}
 if(Count>0)Center/=Count;Center=Generator->GetActorTransform().TransformPosition(Center);
 FCollisionQueryParams Params;for(auto* P:Platforms)Params.AddIgnoredActor(P);for(const auto& P:Players)Params.AddIgnoredActor(P.Character);
 FHitResult Floor;FCollisionObjectQueryParams Ground;Ground.AddObjectTypesToQuery(ECC_WorldStatic);Ground.AddObjectTypesToQuery(ECC_WorldDynamic);
 if(W->LineTraceSingleByObjectType(Floor,Center+FVector(0,0,10000),Center-FVector(0,0,2000),Ground,Params)&&Floor.ImpactNormal.Z>.5f)Center.Z=Floor.ImpactPoint.Z;
 for(auto* P:Platforms){P->SetReplicateMovement(true);P->SetActorLocation(Center+FVector(0,0,25),false,nullptr,ETeleportType::TeleportPhysics);P->LayoutSeed=Generator->RuntimeSeed;P->ForceNetUpdate();}
 for(const auto& Record:Players){
  auto* C=Record.Character;auto* Movement=C->GetCharacterMovement();C->SetBase(nullptr);Movement->StopMovementImmediately();
  ACC_LevitationPlatform* Destination=Record.Platform?Record.Platform:(Platforms.IsEmpty()?nullptr:Platforms[0]);
  FVector Position;
  if(Record.Platform)Position=Record.Platform->GetActorTransform().TransformPosition(Record.Relative);
  else if(Destination)Position=Destination->Deck->GetComponentLocation()+FVector(0,0,Destination->Deck->Bounds.BoxExtent.Z+C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3);
  else Position=Center+FVector(0,0,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+5);
  C->TeleportTo(Position,C->GetActorRotation(),false,true);
  if(Record.Platform){Movement->SetMovementMode(MOVE_Walking);C->SetBase(Record.Platform->Deck);}else Movement->SetMovementMode(MOVE_Falling);
  C->ForceNetUpdate();
 }
 return true;
}
}

bool Enter(UWorld* World) { return Generate(World, false, true); }
bool Rebuild(UWorld* World, bool bToggleMaze) { return Generate(World, bToggleMaze, false); }
}
