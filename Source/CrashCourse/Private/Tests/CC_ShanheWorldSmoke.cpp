#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "World/CC_ShanheChunkWorld.h"
#include "World/CC_AmbientSpawnDirector.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_GameplayData.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

namespace
{
struct FShanheProbe
{
 bool Pass=true;TSharedPtr<FJsonObject> Results=MakeShared<FJsonObject>();
 TWeakObjectPtr<ACC_ShanheChunkWorld> Terrain;
 TWeakObjectPtr<ACC_AmbientSpawnDirector> Director;
 TWeakObjectPtr<ACC_PlayerCharacter> Player;
 TWeakObjectPtr<ACC_PlayerController> Controller;
 void Check(bool Good,const TCHAR* Name){Pass&=Good;Results->SetBoolField(Name,Good);UE_LOG(LogTemp,Display,TEXT("SHANHE_CHECK %s: %d"),Name,Good);}
 void Save(){Results->SetBoolField(TEXT("passed"),Pass);FString Text;FJsonSerializer::Serialize(Results.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectSavedDir()/TEXT("ShanheWorldSmoke.json")));}
};
}
static void ShanheWorldSmoke(UWorld* W)
{
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;
 auto Q=MakeShared<FShanheProbe>();
 for(TActorIterator<ACC_AmbientSpawnDirector> It(W);It;++It)It->bEnabled=false;
 auto Later=[W](float Delay,TFunction<void()> Fn){FTimerHandle Timer;W->GetTimerManager().SetTimer(Timer,MoveTemp(Fn),Delay,false);};
 Later(5.f,[W,Q,Later]()
 {
  Q->Controller=Cast<ACC_PlayerController>(W->GetFirstPlayerController());
  Q->Player=Q->Controller.IsValid()?Cast<ACC_PlayerCharacter>(Q->Controller->GetPawn()):nullptr;
  for(TActorIterator<ACC_ShanheChunkWorld> It(W);It;++It){Q->Terrain=*It;break;}
  for(TActorIterator<ACC_AmbientSpawnDirector> It(W);It;++It){Q->Director=*It;break;}
  Q->Check(Q->Terrain.IsValid()&&Q->Director.IsValid()&&Q->Player.IsValid(),TEXT("real_map_has_player_generator_and_director"));
  if(!Q->Terrain.IsValid()||!Q->Director.IsValid()||!Q->Player.IsValid()){Q->Save();GEngine->Exec(W,TEXT("QUIT"));return;}
  auto* PS=Q->Player->GetPlayerState<ACC_PlayerState>();PS->Progression->ApplyDeveloperPreset(false);
  Q->Check(Q->Terrain->GetLoadedChunkCount()==9,TEXT("nine_chunks_on_actual_map"));
  auto Before=Q->Player->GetMesh()->GetMaterials();
  bool Black=true;for(auto* Mat:Before)Black&=Mat && Mat->GetMaterial()->GetName()==TEXT("M_InkBlack");
  Q->Check(Black,TEXT("initial_ink_all_slots_are_black"));
  bool Key=false;for(auto& Binding:Q->Controller->InputComponent->KeyBindings)if(Binding.Chord.Key==EKeys::Q&&Binding.KeyEvent==IE_Pressed){Binding.KeyDelegate.Execute(EKeys::Q);Key=true;break;}
  Q->Check(Key&&Q->Player->GetMesh()->GetOverlayMaterial()&&Before==Q->Player->GetMesh()->GetMaterials(),TEXT("Q_adds_outline_without_recoloring_body"));
  FCollisionQueryParams Params;Params.AddIgnoredActor(Q->Player.Get());FHitResult Hit;
  Q->Check(W->LineTraceSingleByChannel(Hit,FVector(3000,2000,1000),FVector(3000,2000,-1000),ECC_WorldStatic,Params)&&Cast<ACC_ShanheTerrainChunk>(Hit.GetActor()),TEXT("generated_ground_has_real_collision"));
  Q->Director->bEnabled=true;for(int32 I=0;I<10;++I)Q->Director->RunSpawnCycle();Q->Director->bEnabled=false;
  Q->Check(Q->Director->GetManagedPopulation()>0,TEXT("bamboo_spawns_on_runtime_navigation"));
  Q->Check(Q->Director->GetManagedPopulation()<=12,TEXT("nearby_population_respects_twelve_cap"));
  bool Valid=true;int32 Count=0;
  for(TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It)if(It->ActorHasTag(TEXT("CC.AmbientSpawned"))&&It->IsAlive())
  {++Count;Valid&=It->StatProfileId==TEXT("Nature_T1")&&FVector::Dist2D(It->GetActorLocation(),Q->Player->GetActorLocation())>=1750;}
  Q->Check(Valid&&Count>0,TEXT("origin_uses_low_tier_and_safe_distance"));
  Q->Results->SetNumberField(TEXT("originPopulation"),Count);
  Q->Player->SetActorLocation(FVector(4100,0,98));Q->Terrain->RefreshForPlayer();
  Q->Check(Q->Terrain->CenterCoordinates==FIntPoint(1,0)&&Q->Terrain->GetLoadedChunkCount()==9,TEXT("crossing_boundary_moves_window_with_nine_tiles"));
  Q->Player->SetActorLocation(FVector(40000,0,98));Q->Terrain->RefreshForPlayer();
  Q->Check(Q->Terrain->CenterCoordinates==FIntPoint(5,0)&&Q->Terrain->GetLoadedChunkCount()==9,TEXT("far_teleport_rebuilds_only_nine_tiles"));
  Q->Check(Q->Director->GetManagedPopulation()==0&&Q->Director->GetTotalDespawned()>=Count,TEXT("unloaded_region_cleans_old_mobs"));
  Q->Check(PS->Progression->Experience==0,TEXT("despawn_does_not_award_experience"));
  Later(6.f,[W,Q,Later]()
  {
   Q->Director->bEnabled=true;for(int32 I=0;I<10;++I)Q->Director->RunSpawnCycle();Q->Director->bEnabled=false;
   bool Strong=true;int32 Count=0;
   for(TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It)if(It->ActorHasTag(TEXT("CC.AmbientSpawned"))&&It->IsAlive())
   {
    ++Count;float BaseHP=0,BaseAttack=0;
    Strong&=It->StatProfileId!=TEXT("Nature_T1")&&It->CombatBalance&&It->CombatBalance->Resolve(It->StatProfileId,BaseHP,BaseAttack);
    auto* ASC=It->GetAbilitySystemComponent();
    Strong&=ASC&&FMath::IsNearlyEqual(ASC->GetNumericAttribute(UCC_AttributeSet::GetMaxHealthAttribute()),BaseHP*1.12f,.2f)&&FMath::IsNearlyEqual(ASC->GetNumericAttribute(UCC_AttributeSet::GetAttackAttribute()),BaseAttack*1.04f,.2f);
   }
   Q->Check(Strong&&Count>0,TEXT("ring_five_uses_stronger_tiers_and_coordinate_scaling"));
   Q->Results->SetNumberField(TEXT("outerPopulation"),Count);
   Q->Check(Q->Director->GetManagedPopulation()<=12,TEXT("outer_population_remains_capped"));
   Q->Player->SetActorLocation(FVector(0,0,98));Q->Terrain->RefreshForPlayer();
   Q->Check(Q->Terrain->GetLoadedChunkCount()==9&&Q->Terrain->CenterCoordinates==FIntPoint::ZeroValue,TEXT("return_to_origin_keeps_nine_tiles"));
   Q->Save();
   if(!FParse::Param(FCommandLine::Get(),TEXT("ShanheCapture"))){GEngine->Exec(W,TEXT("QUIT"));return;}
   Q->Director->bEnabled=false;Q->Controller->SetControlRotation(FRotator(-10,180,0));
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ShanheBlackInk.png"),false,false);
   Later(2.f,[W,Q,Later]()
   {
    auto* Camera=W->SpawnActor<ACameraActor>(FVector(0,0,22000),FRotator(-90,0,0));
    Camera->GetCameraComponent()->FieldOfView=90;Q->Controller->SetViewTarget(Camera);
    Later(1.f,[W,Later](){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ShanheNineChunks.png"),false,false);Later(2.f,[W](){GEngine->Exec(W,TEXT("QUIT"));});});
   });
  });
 });
}
static FAutoConsoleCommandWithWorld ShanheCommand(TEXT("CC.ShanheWorldSmoke"),TEXT("Verify black ink, nine chunks, navigation and coordinate-scaled spawning without writing saves."),FConsoleCommandWithWorldDelegate::CreateStatic(&ShanheWorldSmoke));
#endif
