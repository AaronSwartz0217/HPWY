#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/CC_PlayerController.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Interaction/CC_PlayerScanComponent.h"
#include "Interaction/CC_ScanData.h"
#include "UI/CC_FirstPersonScanWidget.h"
#include "UI/CC_WidgetComponent.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Progression/CC_ExperiencePack.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "WFCMapGenerator.h"
#include "WFCTileSet.h"
namespace {
struct FScanProbe{
 TWeakObjectPtr<AWFCMapGenerator> Scene;TWeakObjectPtr<UWFCTileSet> SceneTiles;
 TWeakObjectPtr<ACC_EnemyCharacter> HealthEnemy;
 bool Pass=true;TWeakObjectPtr<ACC_PlayerController> PC;TWeakObjectPtr<ACC_PlayerCharacter> Player;TWeakObjectPtr<AActor> Box;TWeakObjectPtr<AActor> Wall;TWeakObjectPtr<UCC_ScanTargetComponent> Component;FVector Origin;
 TSharedPtr<FJsonObject> Checks=MakeShared<FJsonObject>();
 void Check(bool B,const TCHAR* Name){Pass&=B;Checks->SetBoolField(Name,B);UE_LOG(LogTemp,Display,TEXT("SCAN_CHECK %s: %d"),Name,B);}
};
AActor* ScanBox(UWorld* W,FVector Location,FVector Scale){
 auto* A=W->SpawnActor<AActor>();auto* Mesh=NewObject<UStaticMeshComponent>(A);A->AddInstanceComponent(Mesh);A->SetRootComponent(Mesh);Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetCollisionProfileName(TEXT("BlockAll"));Mesh->SetCanEverAffectNavigation(false);Mesh->RegisterComponent();Mesh->SetWorldLocation(Location);Mesh->SetWorldScale3D(Scale);return A;
}
void RunScanSmoke(UWorld* W){
 if(!W||!W->IsGameWorld()||W->GetNetMode()!=NM_Standalone)return;
 auto Q=MakeShared<FScanProbe>();auto Later=[W](float Delay,TFunction<void()> F){FTimerHandle H;W->GetTimerManager().SetTimer(H,MoveTemp(F),Delay,false);};
 Later(3.f,[W,Q,Later](){
  Q->PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());Q->Player=Q->PC.IsValid()?Cast<ACC_PlayerCharacter>(Q->PC->GetPawn()):nullptr;
  if(!Q->Player.IsValid()||!Q->PC->Scanner){GEngine->Exec(W,TEXT("QUIT"));return;}
  // Isolate generic object scanning from Heaven's intentionally global E command.
  if(auto* Scene=Cast<AWFCMapGenerator>(Q->PC->GetHeavenSceneTarget())){Q->Scene=Scene;Q->SceneTiles=Scene->TileSet;Scene->TileSet=nullptr;}
  auto* Scan=Q->PC->Scanner.Get();Q->Check(Scan->Catalog!=nullptr,TEXT("editable_catalog_loaded"));
  auto* HUD=Cast<ACC_ProgressionHUD>(Q->PC->GetHUD());Q->Check(HUD&&HUD->FirstPersonScanHUD&&HUD->FirstPersonScanHUD->IsInViewport(),TEXT("crosshair_widget_exists"));
  Q->Player->GetCharacterMovement()->DisableMovement();Q->Player->SetFirstPerson(false);Scan->ScanNow();Q->Check(!Scan->IsScanActive()&&!Scan->GetTarget(),TEXT("third_person_hides_scan_and_crosshair"));
  Q->Origin=Q->Player->GetActorLocation();Q->Box=ScanBox(W,Q->Origin+FVector(250,0,65),FVector(.5f));
  auto* Component=NewObject<UCC_ScanTargetComponent>(Q->Box.Get());Q->Box->AddInstanceComponent(Component);Component->Info=LoadObject<UCC_ScanDescription>(nullptr,TEXT("/Game/UI/Scan/DA_Scan_Example.DA_Scan_Example"));Component->RegisterComponent();Q->Component=Component;
  Q->Check(Component->Info!=nullptr,TEXT("object_component_has_description_asset"));Q->Player->SetFirstPerson(true);Q->PC->SetControlRotation(FRotator::ZeroRotator);
  Later(.3f,[W,Q](){auto* S=Q->PC->Scanner.Get();S->ScanNow();Q->Check(S->IsScanActive()&&S->GetTarget()==Q->Box.Get()&&S->GetInfo()==Q->Component->Info,TEXT("first_person_center_ray_reads_object_info"));Q->Check(!Q->PC->FindInteractionTarget(),TEXT("informational_object_is_not_a_fake_interaction"));FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Scan/FirstPersonInfo.png"),true,false);});
  Later(.6f,[W,Q](){auto* S=Q->PC->Scanner.Get();Q->Wall=ScanBox(W,Q->Origin+FVector(120,0,65),FVector(.2f,2,2));S->ScanNow();Q->Check(!S->GetTarget(),TEXT("opaque_wall_blocks_scan"));Q->Wall->Destroy();});
  Later(.8f,[Q](){auto* S=Q->PC->Scanner.Get();Q->Box->SetActorLocation(Q->Origin+FVector(750,0,65));S->ScanNow();Q->Check(!S->GetTarget(),TEXT("objects_beyond_six_meters_are_hidden"));Q->Box->SetActorLocation(Q->Origin+FVector(250,0,65));Q->Component->bEnabled=false;S->ScanNow();Q->Check(!S->GetTarget(),TEXT("per_object_scan_disable_is_honored"));Q->Component->bEnabled=true;Q->Box->SetActorHiddenInGame(true);S->ScanNow();Q->Check(!S->GetTarget(),TEXT("hidden_objects_have_no_scan_hint"));Q->Box->SetActorHiddenInGame(false);});
  Later(1.f,[Q](){auto* S=Q->PC->Scanner.Get();Q->PC->OpenProgressionMenu();S->ScanNow();Q->Check(!S->IsScanActive()&&!S->GetTarget(),TEXT("full_menu_hides_crosshair_and_target"));Q->PC->CloseProgressionMenu();S->ScanNow();Q->Check(S->GetTarget()==Q->Box.Get(),TEXT("scan_resumes_after_closing_menu"));});
  Later(1.1f,[W,Q](){
   UClass* Class=LoadClass<ACC_EnemyCharacter>(nullptr,TEXT("/Game/HPWH/Blueprints/BP_TestDummy.BP_TestDummy_C"));
   auto* Enemy=Class?W->SpawnActor<ACC_EnemyCharacter>(Class,Q->Origin+FVector(140,0,0),FRotator::ZeroRotator):nullptr;
   Q->PC->Scanner->ScanNow();Q->Check(Enemy&&Q->PC->Scanner->GetTarget()==Enemy,TEXT("pawn_visibility_ignore_still_scans_enemy"));if(Enemy)Enemy->Destroy();
  });
  Later(1.2f,[W,Q](){
   auto* Pack=W->SpawnActor<ACC_ExperiencePack>(ACC_ExperiencePack::StaticClass(),Q->Origin+FVector(175,0,65),FRotator::ZeroRotator);Q->Wall=Pack;Pack->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Pack->Mesh->SetWorldScale3D(FVector(.5f));
   auto* S=Q->PC->Scanner.Get();S->ScanNow();Q->Check(S->GetTarget()==Pack&&S->GetInfo()&&S->HasInteraction()&&S->CanInteractWithTarget(),TEXT("experience_object_uses_catalog_and_real_reach"));Q->Check(Q->PC->FindInteractionTarget()==Pack,TEXT("first_person_E_matches_crosshair_object"));
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Scan/InteractionReady.png"),true,false);
  });
  Later(1.4f,[Q](){Q->Player->SetFirstPerson(false);Q->PC->SetControlRotation(FRotator(65,90,0));});
  Later(1.6f,[W,Q](){auto* H=Cast<ACC_ProgressionHUD>(Q->PC->GetHUD());Q->Check(H&&!H->FirstPersonScanHUD->GetDisplayedTarget()&&!H->FirstPersonScanHUD->IsInteractionReady()&&!H->IsPickupPromptVisible(),TEXT("third_person_observation_and_E_hints_are_hidden"));Q->Check(!Q->PC->Scanner->IsScanActive(),TEXT("third_person_hint_does_not_enable_crosshair"));FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Scan/ThirdPersonInteraction.png"),true,false);});
  Later(1.8f,[Q](){const FKey Key=Q->PC->GetCombatKey(5);for(auto& B:Q->PC->InputComponent->KeyBindings)if(B.Chord.Key==Key&&B.KeyEvent==IE_Pressed){B.KeyDelegate.Execute(Key);break;}auto* Pack=Cast<ACC_ExperiencePack>(Q->Wall.Get());Q->Check(Pack&&Pack->bConsumed,TEXT("third_person_existing_interaction_behavior_preserved"));});
  Later(2.f,[Q](){Q->Wall->Destroy();Q->Box->Destroy();Q->PC->Scanner->ScanNow();Q->Check(!Q->PC->Scanner->GetTarget(),TEXT("destroyed_target_clears_hint"));Q->Player->SetFirstPerson(false);Q->PC->Scanner->ScanNow();Q->Check(!Q->PC->Scanner->IsScanActive(),TEXT("return_to_third_person_clears_scan"));});
  Later(2.2f,[W,Q](){
   Q->Origin+=FVector(0,0,2500);
   Q->Player->SetActorLocation(Q->Origin);
   Q->PC->SetControlRotation(FRotator::ZeroRotator);
   Q->Box=ScanBox(W,Q->Origin+FVector(250,0,65),FVector(.5f));
   auto* Description=NewObject<UCC_ScanTargetComponent>(Q->Box.Get());
   Q->Box->AddInstanceComponent(Description);
   Description->Info=LoadObject<UCC_ScanDescription>(nullptr,TEXT("/Game/UI/Scan/DA_Scan_Example.DA_Scan_Example"));
   Description->RegisterComponent();
   UClass* Class=LoadClass<ACC_EnemyCharacter>(nullptr,TEXT("/Game/HPWH/Blueprints/BP_TestDummy.BP_TestDummy_C"));
   if(Class)Q->HealthEnemy=W->SpawnActor<ACC_EnemyCharacter>(Class,Q->Origin+FVector(225,140,0),FRotator::ZeroRotator);
   Q->Check(Q->HealthEnemy.IsValid()&&!Q->HealthEnemy->HealthDisplay->IsVisible(),TEXT("enemy_health_hidden_on_spawn"));
   if(Q->HealthEnemy.IsValid())Q->HealthEnemy->GetCharacterMovement()->DisableMovement();
   Q->Player->SetFirstPerson(true);
  });
  Later(2.65f,[Q](){
   auto* HUD=Q->PC->GetHUD<ACC_ProgressionHUD>();
   Q->Check(HUD&&HUD->FirstPersonScanHUD->GetDisplayedTarget()==Q->Box.Get(),TEXT("first_person_observation_card_appears"));
   Q->Check(Q->HealthEnemy.IsValid()&&Q->HealthEnemy->HealthDisplay->IsVisible(),TEXT("nearby_enemy_health_visible_off_crosshair"));
  });
  Later(4.55f,[Q](){
   auto* HUD=Q->PC->GetHUD<ACC_ProgressionHUD>();
   Q->Check(HUD&&!HUD->FirstPersonScanHUD->GetDisplayedTarget()&&Q->PC->Scanner->GetTarget()==Q->Box.Get(),TEXT("observation_card_expires_while_still_aimed"));
   Q->Check(Q->HealthEnemy.IsValid()&&Q->HealthEnemy->HealthDisplay->IsVisible(),TEXT("enemy_health_outlasts_card"));
   Q->PC->SetControlRotation(FRotator(0,-90,0));
  });
  Later(4.85f,[Q](){Q->PC->SetControlRotation(FRotator::ZeroRotator);});
  Later(5.2f,[Q](){
   auto* HUD=Q->PC->GetHUD<ACC_ProgressionHUD>();
   Q->Check(HUD&&HUD->FirstPersonScanHUD->GetDisplayedTarget()==Q->Box.Get(),TEXT("aiming_back_retriggers_card"));
   if(Q->HealthEnemy.IsValid())Q->HealthEnemy->SetActorLocation(Q->Origin+FVector(900,140,0));
   Q->PC->Scanner->ScanNow();
   Q->Check(Q->HealthEnemy.IsValid()&&!Q->HealthEnemy->HealthDisplay->IsVisible(),TEXT("enemy_health_hides_out_of_range"));
  });
  Later(7.2f,[Q](){
   auto* HUD=Q->PC->GetHUD<ACC_ProgressionHUD>();
   Q->Check(HUD&&!HUD->FirstPersonScanHUD->GetDisplayedTarget(),TEXT("retriggered_card_also_expires"));
   if(Q->HealthEnemy.IsValid())Q->HealthEnemy->SetActorLocation(Q->Origin+FVector(225,140,0));
   Q->PC->Scanner->ScanNow();
   Q->Player->SetFirstPerson(false);
   Q->PC->Scanner->ScanNow();
   Q->Check(Q->HealthEnemy.IsValid()&&!Q->HealthEnemy->HealthDisplay->IsVisible(),TEXT("third_person_hides_nearby_enemy_health_again"));
  });
  Later(7.5f,[W,Q](){if(Q->Scene.IsValid())Q->Scene->TileSet=Q->SceneTiles.Get();auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Q->Pass);Root->SetObjectField(TEXT("checks"),Q->Checks);FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("ScanSmoke.json")));UE_LOG(LogTemp,Display,TEXT("FIRST_PERSON_SCAN_SMOKE_%s"),Q->Pass?TEXT("PASS"):TEXT("FAIL"));GEngine->Exec(W,TEXT("QUIT"));});
 });
}
FAutoConsoleCommandWithWorld ScanSmokeCommand(TEXT("CC.FirstPersonScanSmokeTest"),TEXT("Explicit first-person reticle, editable scan descriptions, occlusion, distance and interaction smoke; exits isolated process."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunScanSmoke));
}
#endif
