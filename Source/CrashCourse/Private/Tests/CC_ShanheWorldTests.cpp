#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "World/CC_ShanheChunkWorld.h"
#include "World/CC_AmbientSpawnDirector.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

namespace
{
UWorld* TestWorld(FName Name)
{
 UWorld::InitializationValues Values;
 Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,Name,nullptr,true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);return W;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_ShanheChunksTest,"CrashCourse.World.ShanheNineChunks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_ShanheChunksTest::RunTest(const FString& Parameters)
{
 auto* W=TestWorld(TEXT("ShanheChunkTest"));auto* G=W->SpawnActor<ACC_ShanheChunkWorld>();
 TestEqual(TEXT("Origin chunk"),G->CoordinatesForPosition(FVector::ZeroVector),FIntPoint(0,0));
 TestEqual(TEXT("Positive boundary stays until crossed"),G->CoordinatesForPosition(FVector(3999,0,0)),FIntPoint(0,0));
 TestEqual(TEXT("Positive boundary enters next tile"),G->CoordinatesForPosition(FVector(4000,0,0)),FIntPoint(1,0));
 TestEqual(TEXT("Negative boundary is symmetric"),G->CoordinatesForPosition(FVector(-4000,0,0)),FIntPoint(0,0));
 TestEqual(TEXT("Negative crossing floors instead of truncating"),G->CoordinatesForPosition(FVector(-4001,0,0)),FIntPoint(-1,0));
 TestEqual(TEXT("Difficulty uses the larger absolute coordinate"),G->DifficultyRingForPosition(FVector(-32000,16000,0)),4);
 G->SynchronizeChunks();TestEqual(TEXT("Exactly nine initial chunks"),G->GetLoadedChunkCount(),9);
 auto* Shared=G->Chunks.FindRef(FIntPoint(0,0)).Get();
 auto* Sample=G->Chunks.FindRef(FIntPoint(1,1)).Get();
 TArray<FTransform> Original;
 for(int32 I=0;I<Sample->BambooInstances->GetInstanceCount();++I){FTransform T;Sample->BambooInstances->GetInstanceTransform(I,T);Original.Add(T);}
 TestTrue(TEXT("Procedural sample has vegetation"),Original.Num()>0);
 G->CenterCoordinates=FIntPoint(1,0);G->SynchronizeChunks();
 TestEqual(TEXT("Nine chunks after horizontal crossing"),G->GetLoadedChunkCount(),9);
 TestEqual(TEXT("Overlapping tile is reused"),G->Chunks.FindRef(FIntPoint(0,0)).Get(),Shared);
 TestFalse(TEXT("Outgoing west tile unloads"),G->IsPositionLoaded(FVector(-8000,0,0)));
 TestTrue(TEXT("Incoming east tile loads"),G->IsPositionLoaded(FVector(16000,0,0)));
 G->CenterCoordinates=FIntPoint(12,-9);G->SynchronizeChunks();
 TestEqual(TEXT("Long teleports still keep nine"),G->GetLoadedChunkCount(),9);
 TestFalse(TEXT("Old origin is unloaded"),G->IsPositionLoaded(FVector::ZeroVector));
 G->CenterCoordinates=FIntPoint(0,0);G->SynchronizeChunks();Sample=G->Chunks.FindRef(FIntPoint(1,1)).Get();
 TestEqual(TEXT("Revisited vegetation count is stable"),Sample->BambooInstances->GetInstanceCount(),Original.Num());
 bool Same=true;for(int32 I=0;I<Original.Num();++I){FTransform T;Sample->BambooInstances->GetInstanceTransform(I,T);Same&=T.Equals(Original[I]);}
 TestTrue(TEXT("Revisited tile reproduces all vegetation transforms"),Same);
 int32 Live=0;for(TActorIterator<ACC_ShanheTerrainChunk> It(W);It;++It)if(!It->IsActorBeingDestroyed())++Live;
 TestEqual(TEXT("No orphan terrain actors accumulate"),Live,9);
 W->DestroyWorld(false);GEngine->DestroyWorldContext(W);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCC_AmbientSpawnTest,"CrashCourse.World.AmbientSpawnRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCC_AmbientSpawnTest::RunTest(const FString& Parameters)
{
 auto* W=TestWorld(TEXT("AmbientSpawnRulesTest"));auto* D=W->SpawnActor<ACC_AmbientSpawnDirector>();
 D->bAvoidVisibleSpawns=false;auto* Player=W->SpawnActor<ACC_PlayerCharacter>();Player->SetActorLocation(FVector::ZeroVector);
 TArray<ACC_PlayerCharacter*> Players={Player};
 TestFalse(TEXT("No spawn beside player"),D->IsPositionAllowed(FVector(1000,0,98),Players));
 TestTrue(TEXT("Valid distance is accepted"),D->IsPositionAllowed(FVector(2300,0,98),Players));
 D->ProtectedStarts.Add(FVector(2300,0,0));
 TestFalse(TEXT("Spawn-point protection is separate from current player position"),D->IsPositionAllowed(FVector(2300,0,98),Players));
 D->ProtectedStarts.Empty();
 auto* Enemy=W->SpawnActor<ACC_PatrolEnemyCharacter>();Enemy->SetActorLocation(FVector(-2400,0,98));D->Managed.Add(Enemy);
 TestFalse(TEXT("Enemy packs maintain capsule spacing"),D->IsPositionAllowed(FVector(-2500,0,98),Players));
 FVector Position;
 TestFalse(TEXT("Missing navigation never falls back to unsafe spawning"),D->FindSpawnPosition(Player,Players,Position));
 FCC_AmbientSpawnEntry Entry;Entry.EnemyClass=ACC_PatrolEnemyCharacter::StaticClass();Entry.ClassLimit=1;D->Entries={Entry};
 TestNull(TEXT("Class population limit applies"),D->PickEntry(0));
 D->Entries[0].ClassLimit=12;D->Entries[0].MinimumDifficultyTier=2;
 TestNull(TEXT("Low coordinates cannot pick high-tier enemy"),D->PickEntry(0));
 TestNotNull(TEXT("High-tier region enables its enemy"),D->PickEntry(2));
 Enemy->SetActorLocation(FVector(7000,0,98));D->PrunePopulation(Players);
 TestTrue(TEXT("Distant managed enemies are destroyed"),Enemy->IsActorBeingDestroyed());
 TestEqual(TEXT("Despawn is recorded independently of kills"),D->TotalDespawned,1);
 auto* Corpse=W->SpawnActor<ACC_PatrolEnemyCharacter>();Corpse->SetAlive(false);D->Managed.Add(Corpse);D->PrunePopulation(Players);
 TestFalse(TEXT("Death corpses keep their existing cleanup lifecycle"),Corpse->IsActorBeingDestroyed());
 TestEqual(TEXT("Corpses do not consume living population cap"),D->GetManagedPopulation(),0);
 auto* Idle=W->SpawnActor<ACC_PatrolEnemyCharacter>();Idle->SetActorLocation(FVector(5000,0,98));D->Managed.Add(Idle);
 D->OutsideSince.Add(Idle,W->GetTimeSeconds()-D->SoftDespawnDelay-1);D->PrunePopulation(Players);
 TestTrue(TEXT("Unseen enemies outside soft distance expire"),Idle->IsActorBeingDestroyed());
 W->DestroyWorld(false);GEngine->DestroyWorldContext(W);return true;
}
#endif
