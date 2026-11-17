#include "World/CC_ShanheChunkWorld.h"
#include "World/CC_AmbientSpawnDirector.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"

ACC_ShanheTerrainChunk::ACC_ShanheTerrainChunk()
{
 PrimaryActorTick.bCanEverTick=false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
 Ground=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));Ground->SetupAttachment(GetRootComponent());
 RockInstances=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Rocks"));RockInstances->SetupAttachment(GetRootComponent());
 BambooInstances=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Bamboo"));BambooInstances->SetupAttachment(GetRootComponent());
 LeafInstances=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Leaves"));LeafInstances->SetupAttachment(GetRootComponent());
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Rock(TEXT("/Game/StarterContent/Props/SM_Rock"));
 Ground->SetStaticMesh(Cube.Object);RockInstances->SetStaticMesh(Rock.Succeeded()?Rock.Object:Cube.Object);
 BambooInstances->SetStaticMesh(Cylinder.Object);LeafInstances->SetStaticMesh(Cone.Object);
 Ground->SetCollisionProfileName(TEXT("BlockAll"));RockInstances->SetCollisionProfileName(TEXT("BlockAll"));
 Ground->SetCanEverAffectNavigation(true);RockInstances->SetCanEverAffectNavigation(true);
 for(auto* Instances:{BambooInstances.Get(),LeafInstances.Get()})
 {Instances->SetCollisionProfileName(TEXT("NoCollision"));Instances->SetCanEverAffectNavigation(false);Instances->SetCullDistances(12000,17000);}
 Tags.Add(TEXT("CC.ProceduralTerrainChunk"));
}
int32 ACC_ShanheChunkWorld::SeedForCoordinates(FIntPoint Key,int32 Seed)
{
 return static_cast<int32>(HashCombine(GetTypeHash(Seed),HashCombine(GetTypeHash(Key.X),GetTypeHash(Key.Y))));
}
void ACC_ShanheTerrainChunk::Generate(FIntPoint Key,int32 Seed,float Width,float GroundZ,UMaterialInterface* GroundMaterial,UMaterialInterface* BambooMaterial,UMaterialInterface* LeafMaterial,int32 Groves,int32 Rocks)
{
 Coordinates=Key;
 SetActorLocation(FVector(double(Key.X)*Width,double(Key.Y)*Width,GroundZ));
 Ground->SetRelativeLocation(FVector(0,0,-50));Ground->SetRelativeScale3D(FVector(Width/100.f,Width/100.f,1));
 if(GroundMaterial)if(auto* Dynamic=Ground->CreateDynamicMaterialInstance(0,GroundMaterial))Dynamic->SetScalarParameterValue(TEXT("ChunkWidth"),Width);
 if(BambooMaterial)BambooInstances->SetMaterial(0,BambooMaterial);
 if(LeafMaterial)LeafInstances->SetMaterial(0,LeafMaterial);
 FRandomStream Random(ACC_ShanheChunkWorld::SeedForCoordinates(Key,Seed));
 const float Half=Width*.5f-400;
 auto Allowed=[&](FVector Point)
 {
  if(FMath::Abs(Point.X)<280 || FMath::Abs(Point.Y)<280)return false;
  const FVector World=GetActorLocation()+Point;
  return FVector2D(World.X,World.Y).SizeSquared()>FMath::Square(1900.f);
 };
 const FVector RockSize=RockInstances->GetStaticMesh()->GetBounds().BoxExtent*2;
 for(int32 Index=0;Index<FMath::Clamp(Rocks,0,40);++Index)
 {
  FVector Point(Random.FRandRange(-Half,Half),Random.FRandRange(-Half,Half),-10);
  if(!Allowed(Point))continue;
  const float Height=Random.FRandRange(65,150);
  FVector Scale(Random.FRandRange(110,240)/FMath::Max(1.,RockSize.X),Random.FRandRange(110,240)/FMath::Max(1.,RockSize.Y),Height/FMath::Max(1.,RockSize.Z));
  RockInstances->AddInstance(FTransform(FRotator(0,Random.FRandRange(-180,180),0),Point,Scale));
 }
 for(int32 Grove=0;Grove<FMath::Clamp(Groves,0,30);++Grove)
 {
  FVector Center(Random.FRandRange(-Half,Half),Random.FRandRange(-Half,Half),0);
  if(!Allowed(Center))continue;
  for(int32 Stalk=0;Stalk<5;++Stalk)
  {
   const float Height=Random.FRandRange(270,490);
   const FVector Base=Center+FVector(Random.FRandRange(-160,160),Random.FRandRange(-160,160),0);
   BambooInstances->AddInstance(FTransform(FRotator::ZeroRotator,Base+FVector(0,0,Height*.5f),FVector(.10,.10,Height/100.f)));
   LeafInstances->AddInstance(FTransform(FRotator(0,Random.FRandRange(-180,180),0),Base+FVector(0,0,Height-50),FVector(Random.FRandRange(.65,1.05),Random.FRandRange(.65,1.05),1.2)));
  }
 }
}
ACC_ShanheChunkWorld::ACC_ShanheChunkWorld()
{
 PrimaryActorTick.bCanEverTick=false;bReplicates=true;bAlwaysRelevant=true;
}
void ACC_ShanheChunkWorld::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,CenterCoordinates);DOREPLIFETIME(ThisClass,WorldSeed);
}
FIntPoint ACC_ShanheChunkWorld::CoordinatesForPosition(FVector Position) const
{
 const double Width=FMath::Max(4000.f,ChunkSize);
 return FIntPoint(FMath::FloorToInt32((Position.X+Width*.5)/Width),FMath::FloorToInt32((Position.Y+Width*.5)/Width));
}
int32 ACC_ShanheChunkWorld::DifficultyRingForPosition(FVector Position) const
{
 const FIntPoint Key=CoordinatesForPosition(Position);return FMath::Max(FMath::Abs(Key.X),FMath::Abs(Key.Y));
}
bool ACC_ShanheChunkWorld::IsPositionLoaded(FVector Position) const {return Chunks.Contains(CoordinatesForPosition(Position));}
void ACC_ShanheChunkWorld::BeginPlay()
{
 Super::BeginPlay();
 for(TActorIterator<ACC_ShanheChunkWorld> It(GetWorld());It;++It)
  if(*It!=this && It->HasActorBegunPlay()){Destroy();return;}
 SynchronizeChunks();
 // The old test floor stays available for editor work, and is excluded at runtime.
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("CC.ChunkLegacyGeometry")))
 {It->SetActorHiddenInGame(true);It->SetActorEnableCollision(false);}
 if(GetNetMode()==NM_Client)return;
 for(TActorIterator<ANavMeshBoundsVolume> It(GetWorld());It;++It)
  if(It->ActorHasTag(TEXT("CC.ChunkNavigation"))){NavBounds=*It;break;}
 MoveNavigationBounds();
 GetWorldTimerManager().SetTimer(UpdateTimer,this,&ThisClass::RefreshForPlayer,.25f,true);
}
void ACC_ShanheChunkWorld::EndPlay(const EEndPlayReason::Type Reason)
{
 GetWorldTimerManager().ClearTimer(UpdateTimer);
 for(auto& Item:Chunks)if(IsValid(Item.Value))Item.Value->Destroy();Chunks.Empty();
 Super::EndPlay(Reason);
}
void ACC_ShanheChunkWorld::RefreshForPlayer()
{
 if(GetNetMode()==NM_Client)return;
 auto* PC=GetWorld()->GetFirstPlayerController();
 auto* Player=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
 if(!Player || !Player->IsAlive())return;
 const FIntPoint NewCenter=CoordinatesForPosition(Player->GetActorLocation());
 if(NewCenter==CenterCoordinates && Chunks.Num()==9)return;
 CenterCoordinates=NewCenter;SynchronizeChunks();MoveNavigationBounds();ForceNetUpdate();
}
void ACC_ShanheChunkWorld::OnRep_Center(){SynchronizeChunks();}
void ACC_ShanheChunkWorld::SynchronizeChunks()
{
 TSet<FIntPoint> Wanted;
 for(int32 X=-1;X<=1;++X)for(int32 Y=-1;Y<=1;++Y)Wanted.Add(CenterCoordinates+FIntPoint(X,Y));
 TArray<FIntPoint> Remove;
 for(auto& Item:Chunks)if(!Wanted.Contains(Item.Key))Remove.Add(Item.Key);
 for(FIntPoint Key:Remove){if(auto* Chunk=Chunks.FindRef(Key).Get())Chunk->Destroy();Chunks.Remove(Key);}
 // Build the center first on long teleports. Unload first so the actor count never exceeds nine.
 TArray<FIntPoint> Order=Wanted.Array();Order.Sort([&](FIntPoint A,FIntPoint B){return (A-CenterCoordinates).SizeSquared()<(B-CenterCoordinates).SizeSquared();});
 for(FIntPoint Key:Order)
 {
  if(Chunks.Contains(Key))continue;
  auto* Chunk=GetWorld()->SpawnActor<ACC_ShanheTerrainChunk>();if(!Chunk)continue;
  const int32 Hash=SeedForCoordinates(Key,WorldSeed);
  UMaterialInterface* Material=GroundMaterials.IsEmpty()?nullptr:GroundMaterials[static_cast<uint32>(Hash)%GroundMaterials.Num()].Get();
  Chunk->Generate(Key,WorldSeed,FMath::Max(4000.f,ChunkSize),GroundHeight,Material,BambooMaterial,LeafMaterial,GrovesPerChunk,RocksPerChunk);
  Chunks.Add(Key,Chunk);
 }
 for(TActorIterator<ACC_AmbientSpawnDirector> It(GetWorld());It;++It)It->RefreshPopulationBounds();
}
void ACC_ShanheChunkWorld::MoveNavigationBounds()
{
 if(!NavBounds.IsValid())return;
 if(auto* Root=NavBounds->GetRootComponent())Root->SetMobility(EComponentMobility::Movable);
 NavBounds->SetActorLocation(FVector(double(CenterCoordinates.X)*ChunkSize,double(CenterCoordinates.Y)*ChunkSize,GroundHeight+150));
 if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))Nav->OnNavigationBoundsUpdated(NavBounds.Get());
}
