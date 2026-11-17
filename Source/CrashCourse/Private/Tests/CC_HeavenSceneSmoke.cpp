#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Components/InputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "World/CC_LevitationPlatform.h"
#include "World/CC_HeavenSceneRebuild.h"
#include "Progression/CC_ProgressionHUD.h"
#include "UI/CC_FirstPersonScanWidget.h"
#include "WFCMapGenerator.h"
#include "WFCTileSet.h"
#include "World/CC_HeavenMazeExit.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
namespace {
uint32 HeavenHash(AWFCMapGenerator* G){uint32 H=0;for(const auto& C:G->DisplayedResult.Cells){H=HashCombineFast(H,GetTypeHash(C.TileId));H=HashCombineFast(H,GetTypeHash(C.Coordinate));H=HashCombineFast(H,C.QuarterTurns);}return H;}
struct FHeavenProbe {
 bool Pass=true;TWeakObjectPtr<ACC_PlayerController> PC;TWeakObjectPtr<ACC_PlayerCharacter> C;TWeakObjectPtr<ACC_LevitationPlatform> P;TWeakObjectPtr<AWFCMapGenerator> G;uint32 Hash=0;int32 Revision=0;TArray<FTransform> Stones;TMap<FIntVector,FName> Before;
 TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
 void Check(bool B,const TCHAR* Name){Pass&=B;Checks->SetBoolField(Name,B);UE_LOG(LogTemp,Display,TEXT("HEAVEN_SCENE_CHECK %s: %d"),Name,B);}
 void E(){Press(PC->GetCombatKey(5));}
 void Press(const FKey& Key){for(auto& Binding:PC->InputComponent->KeyBindings)if(Binding.Chord.Key==Key&&Binding.KeyEvent==IE_Pressed){Binding.KeyDelegate.Execute(Key);break;}}
};
void RunHeavenSceneSmoke(UWorld* W){
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;
 auto Q=MakeShared<FHeavenProbe>();auto Later=[W](float Delay,TFunction<void()> F){FTimerHandle H;W->GetTimerManager().SetTimer(H,MoveTemp(F),Delay,false);};
 Later(3.f,[W,Q,Later](){
  Q->PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());Q->C=Q->PC.IsValid()?Cast<ACC_PlayerCharacter>(Q->PC->GetPawn()):nullptr;Q->G=CCHeavenScene::FindGenerator(W);for(TActorIterator<ACC_LevitationPlatform> I(W);I;++I){Q->P=*I;break;}
  Q->Check(CCHeavenScene::IsHeaven(W)&&Q->PC.IsValid()&&Q->C.IsValid()&&Q->G.IsValid()&&Q->P.IsValid(),TEXT("real_heaven_scene_player_and_generator_exist"));
  if(!Q->C.IsValid()||!Q->G.IsValid()||!Q->P.IsValid()){GEngine->Exec(W,TEXT("QUIT"));return;}
  for(TActorIterator<ACC_HeavenMazeExit> It(W);It;++It)
  {Q->C->SetActorLocation(It->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);break;}
  Q->PC->ServerToggleHeavenMaze();
  Q->Check(Q->P.IsValid()&&Q->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Settlement,TEXT("exit_unlocks_city_for_rebuild_test"));
  if(!Q->P.IsValid()){GEngine->Exec(W,TEXT("QUIT"));return;}
  Q->C->TeleportTo(Q->P->GetActorLocation()+FVector(0,0,116),FRotator::ZeroRotator);Q->C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
  Later(4.5f,[Q](){Q->Check(Q->P->IsCarrying(Q->C.Get())&&Q->P->CurrentLift>550,TEXT("platform_still_lifts_grounded_player"));Q->C->SetFirstPerson(true);Q->PC->SetControlRotation(FRotator(-20,0,0));});
  Later(4.8f,[Q](){
   Q->Hash=HeavenHash(Q->G.Get());Q->Revision=Q->G->RuntimeRevision;for(const auto& C:Q->G->DisplayedResult.Cells)Q->Before.Add(C.Coordinate,C.TileId);
   for(int32 I=0;I<Q->P->Tiles->GetInstanceCount();++I){FTransform T;Q->P->Tiles->GetInstanceTransform(I,T,false);Q->Stones.Add(T);}
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/HeavenScene/Before.png"),true,false);
   Q->Check(Q->PC->FindInteractionTarget()==Q->G.Get(),TEXT("E_targets_the_world_generator"));
  });
  Later(5.1f,[Q](){
   Q->E();Q->Check(Q->G->RuntimeRevision==Q->Revision+1,TEXT("E_regenerates_whole_scene"));Q->Check(HeavenHash(Q->G.Get())!=Q->Hash,TEXT("scene_layout_changes"));
   int32 Changed=0;for(const auto& C:Q->G->DisplayedResult.Cells){const FName* Old=Q->Before.Find(C.Coordinate);if(!Old||*Old!=C.TileId)++Changed;}
   Q->Check(Changed>9,TEXT("more_than_nine_world_cells_are_rebuilt"));Q->Check(Q->G->GetGeneratedInstanceCount()>100,TEXT("full_scene_instances_exist_after_rebuild"));Q->Check(Q->G->Settings.LayoutPreset==EWFCLayoutPreset::Settlement,TEXT("roads_and_buildings_use_random_layout"));
   bool Same=Q->Stones.Num()==Q->P->Tiles->GetInstanceCount();for(int32 I=0;I<Q->Stones.Num()&&Same;++I){FTransform T;Q->P->Tiles->GetInstanceTransform(I,T,false);Same&=T.Equals(Q->Stones[I],.001f);}
   bool Ready=true;TArray<UHierarchicalInstancedStaticMeshComponent*> Groups;Q->G->GetComponents(Groups);for(auto* Group:Groups)Ready&=Group->IsTreeFullyBuilt()&&Group->NumBuiltInstances==Group->GetInstanceCount();Q->Check(Ready,TEXT("all_render_clusters_are_ready_immediately"));Q->Check(Same,TEXT("platform_stones_do_not_reassemble_or_animate"));Q->Check(Q->P->IsCarrying(Q->C.Get()),TEXT("passenger_is_preserved_immediately"));
   Q->Revision=Q->G->RuntimeRevision;Q->E();Q->Check(Q->G->RuntimeRevision==Q->Revision,TEXT("immediate_repeat_is_rate_limited"));
  });
  Later(5.5f,[Q](){Q->Check(Q->P->IsCarrying(Q->C.Get())&&Q->C->GetCharacterMovement()->MovementMode==MOVE_Walking,TEXT("player_remains_grounded_after_scene_swap"));auto* H=Cast<ACC_ProgressionHUD>(Q->PC->GetHUD());Q->Check(H&&H->FirstPersonScanHUD->GetDisplayedTarget()==Q->G.Get(),TEXT("observation_hint_describes_the_scene"));FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/HeavenScene/After.png"),true,false);});
  Later(5.9f,[Q](){Q->C->SetFirstPerson(false);Q->Hash=HeavenHash(Q->G.Get());Q->E();Q->Check(HeavenHash(Q->G.Get())!=Q->Hash,TEXT("third_person_E_also_randomizes_scene"));auto* H=Cast<ACC_ProgressionHUD>(Q->PC->GetHUD());Q->Check(H&&!H->IsPickupPromptVisible()&&!H->FirstPersonScanHUD->GetDisplayedTarget(),TEXT("third_person_still_has_no_observation_hint"));});
  Later(6.7f,[Q](){Q->C->SetBase(nullptr);Q->C->SetActorLocation(FVector(30000,30000,5000),false,nullptr,ETeleportType::TeleportPhysics);Q->C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);Q->Hash=HeavenHash(Q->G.Get());Q->E();Q->Check(HeavenHash(Q->G.Get())!=Q->Hash,TEXT("E_does_not_require_a_small_platform_console"));Q->Check(FVector::Dist(Q->C->GetActorLocation(),Q->P->Deck->GetComponentLocation())<300,TEXT("unbased_player_is_moved_to_safe_support"));});
  Later(7.3f,[Q](){Q->Revision=Q->G->RuntimeRevision;Q->PC->OpenProgressionMenu();Q->E();Q->Check(Q->G->RuntimeRevision==Q->Revision,TEXT("menus_block_scene_rebuild"));Q->PC->CloseProgressionMenu();});
  Later(7.7f,[W,Q](){auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Q->Pass);Root->SetObjectField(TEXT("checks"),Q->Checks);Root->SetNumberField(TEXT("finalInstances"),Q->G->GetGeneratedInstanceCount());FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("HeavenSceneSmoke.json")));UE_LOG(LogTemp,Display,TEXT("HEAVEN_WHOLE_SCENE_SMOKE_%s"),Q->Pass?TEXT("PASS"):TEXT("FAIL"));GEngine->Exec(W,TEXT("QUIT"));});
 });
}
void RunHeavenMazeSmoke(UWorld* World)
{
 if(!World||!World->IsGameWorld()||World->GetNetMode()!=NM_Standalone)return;
 auto Probe=MakeShared<FHeavenProbe>();
 auto Later=[World](float Delay,TFunction<void()> Action)
 {
  FTimerHandle Handle;
  World->GetTimerManager().SetTimer(Handle,MoveTemp(Action),Delay,false);
 };
 Later(2.5f,[World,Probe]()
 {
  Probe->PC=Cast<ACC_PlayerController>(World->GetFirstPlayerController());
  Probe->C=Probe->PC.IsValid()?Cast<ACC_PlayerCharacter>(Probe->PC->GetPawn()):nullptr;
  Probe->G=CCHeavenScene::FindGenerator(World);
  if(!Probe->PC.IsValid()||!Probe->C.IsValid()||!Probe->G.IsValid()){GEngine->Exec(World,TEXT("QUIT"));return;}
  Probe->Revision=Probe->G->RuntimeRevision;
  Probe->Hash=HeavenHash(Probe->G.Get());
  Probe->Check(Probe->Revision>0&&Probe->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Maze,TEXT("map_starts_in_random_maze"));
  Probe->Press(EKeys::L);
  Probe->Check(Probe->G->RuntimeRevision==Probe->Revision&&!Probe->PC->IsHeavenCityUnlocked(),TEXT("L_cannot_build_locked_city"));
  Probe->Check(Probe->G->DisplayedResult.WalkableComponents==1,TEXT("maze_routes_are_connected"));
  const auto& Cells=Probe->G->DisplayedResult.Cells;
  const auto* Entry=Cells.FindByPredicate([](const auto& Cell){return Cell.TileId==TEXT("MazeEntry");});
  const auto* Wall=Cells.FindByPredicate([](const auto& Cell){return Cell.TileId==TEXT("MazeWall");});
  const auto Point=[&](const FWFCCellPlacement& Cell){return Probe->G->GetActorTransform().TransformPosition(FVector(Cell.Coordinate)*Probe->G->TileSet->CellSize);};
  Probe->Check(Entry&&FVector::Dist2D(Point(*Entry),Probe->C->GetActorLocation())<330,TEXT("player_starts_in_clear_entrance"));
  FHitResult Hit;
  const FVector WallCenter=Wall?Point(*Wall):FVector::ZeroVector;
  const bool bWallBlocks=Wall&&World->LineTraceSingleByChannel(Hit,WallCenter+FVector(0,0,520),WallCenter+FVector(0,0,100),ECC_Visibility)&&Hit.GetActor()==Probe->G.Get();
  Probe->Check(bWallBlocks,TEXT("wall_cube_has_collision"));
  Probe->Check(!Probe->PC->HasCompletedHeavenMaze(Probe->Revision),TEXT("entry_does_not_complete_maze"));
  TMap<FIntVector,FName> GroundTiles;
  for(const auto& Cell:Cells)
   if(Cell.Coordinate.Z==0)GroundTiles.Add(Cell.Coordinate,Cell.TileId);
  bool bOnlyBronzeOnPath=true;
  TSet<FIntVector> Occupied;
  const FVector Size=Probe->G->TileSet->CellSize;
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(World);It;++It)
  {
   const FVector Local=Probe->G->GetActorTransform().InverseTransformPosition(It->PatrolOrigin);
   const FIntVector Coordinate(FMath::RoundToInt(Local.X/Size.X),FMath::RoundToInt(Local.Y/Size.Y),0);
   const FIntVector Offset=Entry?Coordinate-Entry->Coordinate:FIntVector::ZeroValue;
   const FName Profile=It->StatProfileId;
   bOnlyBronzeOnPath &= It->ActorHasTag(TEXT("CC.HeavenMazeEnemy"))
    && (Profile==TEXT("Bronze_T1")||Profile==TEXT("Bronze_T2")||Profile==TEXT("Bronze_T3"))
    && GroundTiles.FindRef(Coordinate)==TEXT("MazePath") && !Occupied.Contains(Coordinate)
    && (FMath::Abs(Offset.X)>1||FMath::Abs(Offset.Y)>1);
   Occupied.Add(Coordinate);
  }
  Probe->Check(!Occupied.IsEmpty()&&bOnlyBronzeOnPath,TEXT("only_bronze_on_unique_path_tiles_outside_entrance"));
 });
 Later(3.1f,[Probe]()
 {
  if(!Probe->C.IsValid())return;
  Probe->Check(Probe->C->GetCharacterMovement()->IsMovingOnGround(),TEXT("entrance_floor_supports_player"));
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/HeavenMaze/Entrance.png"),false,false);
 });
 Later(3.5f,[World,Probe]()
 {
  if(!Probe->C.IsValid())return;
  for(TActorIterator<ACC_HeavenMazeExit> It(World);It;++It)
  {
   Probe->C->SetActorLocation(It->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);
   break;
  }
 });
 Later(3.85f,[Probe]()
 {
  if(!Probe->C.IsValid())return;
  Probe->Check(Probe->PC->HasCompletedHeavenMaze(Probe->Revision)&&Probe->PC->IsHeavenCityUnlocked(),TEXT("gold_exit_overlap_unlocks_city"));
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/HeavenMaze/Exit.png"),false,false);
 });
 Later(4.3f,[Probe]()
 {
  if(!Probe->C.IsValid())return;
  Probe->E();
  Probe->Check(Probe->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Maze&&
   Probe->G->RuntimeRevision==Probe->Revision+1&&HeavenHash(Probe->G.Get())!=Probe->Hash,TEXT("E_regenerates_active_maze"));
  Probe->Check(!Probe->PC->HasCompletedHeavenMaze(Probe->G->RuntimeRevision),TEXT("new_maze_clears_completion"));
  Probe->Check(Probe->PC->IsHeavenCityUnlocked(),TEXT("rebuild_preserves_visit_city_unlock"));
 });
 Later(5.2f,[World,Probe]()
 {
  if(!Probe->C.IsValid())return;
  Probe->PC->OpenProgressionMenu();
  Probe->PC->ServerToggleHeavenMaze();
  Probe->Check(Probe->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Maze,TEXT("menu_blocks_maze_switch"));
  Probe->PC->CloseProgressionMenu();
  Probe->Press(EKeys::L);
  Probe->Check(Probe->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Settlement,TEXT("L_restores_city"));
  Probe->Check(!TActorIterator<ACC_HeavenMazeExit>(World),TEXT("city_removes_maze_exit"));
  Probe->Check(!TActorIterator<ACC_PatrolEnemyCharacter>(World),TEXT("city_removes_maze_enemies"));
 });
 Later(5.8f,[Probe]()
 {
  if(!Probe->C.IsValid())return;
  const auto Previous=Probe->G->RuntimeRevision;
  Probe->E();
  Probe->Check(Probe->G->RuntimeRevision==Previous+1&&
   Probe->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Settlement,TEXT("E_rebuilds_unlocked_city"));
 });
 Later(6.4f,[Probe]()
 {
  if(!Probe->C.IsValid())return;
  Probe->Press(EKeys::L);
  Probe->Check(Probe->G->DisplayedSettings.LayoutPreset==EWFCLayoutPreset::Maze,TEXT("L_returns_to_maze"));
 });
 Later(7.0f,[World,Probe]()
 {
  if(!Probe->C.IsValid())return;
  auto Report=MakeShared<FJsonObject>();
  Report->SetBoolField(TEXT("passed"),Probe->Pass);
  Report->SetObjectField(TEXT("checks"),Probe->Checks);
  FString Json;
  FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
  FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("HeavenMazeSmoke.json")));
  UE_LOG(LogTemp,Display,TEXT("HEAVEN_MAZE_SMOKE_%s"),Probe->Pass?TEXT("PASS"):TEXT("FAIL"));
  GEngine->Exec(World,TEXT("QUIT"));
 });
}
FAutoConsoleCommandWithWorld HeavenMazeProbe(TEXT("CC.HeavenMazeSmoke"),TEXT("Verify maze input, wall collision and exit in the actual Heaven map."),
 FConsoleCommandWithWorldDelegate::CreateStatic(&RunHeavenMazeSmoke));

FAutoConsoleCommandWithWorld HeavenSceneProbe(TEXT("CC.HeavenWholeSceneSmokeTest"),TEXT("Actual E rebuilds the entire Heaven WFC world, preserves player support, no stone animation; isolated test then exit."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunHeavenSceneSmoke));
}
#endif
