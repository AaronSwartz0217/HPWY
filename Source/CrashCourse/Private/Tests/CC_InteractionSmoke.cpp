#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_FormPowerOrb.h"
#include "Progression/CC_ExperiencePack.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Progression/CC_ProgressionHUD.h"
#include "UnrealClient.h"
namespace {
void RunInteractionSmoke(UWorld* W) {
 if (!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) return;
 FTimerHandle Timer;
 W->GetTimerManager().SetTimer(Timer,[W]() {
  bool Passed=true;
  auto Check=[&](bool V,const TCHAR* Name) { Passed &= V; UE_LOG(LogTemp,Display,TEXT("INTERACTION_%s: %d"),Name,V); };
  auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());
  auto* C=PC ? Cast<ACC_PlayerCharacter>(PC->GetPawn()) : nullptr;
  auto* PS=PC ? PC->GetPlayerState<ACC_PlayerState>() : nullptr;
  Check(PC && C && PS,TEXT("player_ready"));
  if (PC && C && PS) {
   for (TActorIterator<ACC_PatrolEnemyCharacter> It(W);It;++It) It->StopPatrol();
   PC->CloseProgressionMenu(); C->GetCharacterMovement()->DisableMovement();
   auto* Camera=W->SpawnActor<ACameraActor>();
   PC->SetViewTarget(Camera);
   auto LookAway=[PC,C,Camera]() { Camera->SetActorLocation(C->GetActorLocation()+FVector(0,0,120)); Camera->SetActorRotation(FRotator(70,0,0)); PC->PlayerCameraManager->UpdateCamera(.1f); };
   ACC_FormArtifact* Target=nullptr;
   for (TActorIterator<ACC_FormArtifact> It(W);It;++It) if (It->Artifact->Form && It->Artifact->Form->FormId==TEXT("Native")) { Target=*It; break; }
   Check(Target!=nullptr,TEXT("white_cube_exists"));
   if (Target) {
    C->SetActorLocation(Target->GetActorLocation()+FVector(300,0,60),false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation(FRotator(70,0,0)); LookAway();
    Check(FVector::Distance(C->GetActorLocation(),Target->GetActorLocation())>Target->Artifact->InteractionDistance && Target->Artifact->CanInteract(C),TEXT("reach_uses_cube_surface"));
    Check(PC->FindInteractionTarget()==Target,TEXT("nearby_cube_found_without_aiming"));
    auto* P=PS->Progression.Get(); auto* Before=P->GetActiveForm();
    const FVector OriginalPosition=C->GetActorLocation();
    PC->RequestInteraction(PC->FindInteractionTarget());
    // Acquisition now releases a physical power orb instead of granting at E.
    bool OrbFound=false;
    for(TActorIterator<ACC_FormPowerOrb> Orb(W);Orb;++Orb)if(Orb->Recipient==PS&&Orb->RewardForm==Target->Artifact->Form&&!Orb->IsActorBeingDestroyed()){
     OrbFound=true;Orb->BeginSeeking();C->SetActorLocation(Orb->GetActorLocation());if(!Orb->IsActorBeingDestroyed())Orb->TryCollect(C);break;
    }
    Check(OrbFound,TEXT("interaction_releases_physical_power_orb"));
    Check(P->Forms.ContainsByPredicate([Target](const FCC_FormProgress& F) { return F.Form==Target->Artifact->Form; }) && P->GetActiveForm()==Before,TEXT("pickup_stores_without_transforming"));
    // Restore this presentation fixture for the independent reach/visibility checks.
    C->SetActorLocation(OriginalPosition);Target->Artifact->bConsumed=false;Target->SetActorHiddenInGame(false);Target->SetActorEnableCollision(true);
    auto* Wall=W->SpawnActor<AActor>();
    auto* Box=NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box); Box->SetBoxExtent(FVector(20,100,180)); Box->SetCollisionProfileName(TEXT("BlockAll")); Box->RegisterComponent();
    Wall->SetActorLocation(Target->GetActorLocation()+FVector(150,0,120));
    Check(!Target->Artifact->CanInteract(C) && PC->FindInteractionTarget()!=Target,TEXT("wall_blocks_pickup"));
    Wall->Destroy();
    C->SetActorLocation(Target->GetActorLocation()+FVector(650,0,60),false,nullptr,ETeleportType::TeleportPhysics);
    Check(!Target->Artifact->CanInteract(C) && PC->FindInteractionTarget()!=Target,TEXT("out_of_reach_rejected"));
    Target->Artifact->bEnabled=false;
    C->SetActorLocation(Target->GetActorLocation()+FVector(180,0,60),false,nullptr,ETeleportType::TeleportPhysics);
    Check(PC->FindInteractionTarget()!=Target,TEXT("disabled_cube_rejected"));
    Target->Artifact->bEnabled=true;
   }
   ACC_ExperiencePack* Pack=nullptr;
   for (TActorIterator<ACC_ExperiencePack> It(W);It;++It) if (!It->bConsumed) { Pack=*It; break; }
   Check(Pack!=nullptr,TEXT("experience_pack_exists"));
   if (Pack) {
    C->SetActorLocation(Pack->GetActorLocation()+FVector(180,0,60),false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation(FRotator(70,0,0)); LookAway();
    // Other permanent pickups are next to the pack in this arena. Try each
    // approach direction so the pack can be the nearest candidate.
    bool PackSelected=false;
    for (const FVector Offset : {FVector(180,0,60),FVector(-180,0,60),FVector(0,180,60),FVector(0,-180,60)}) {
     C->SetActorLocation(Pack->GetActorLocation()+Offset,false,nullptr,ETeleportType::TeleportPhysics); LookAway();
     if (PC->FindInteractionTarget()==Pack) { PackSelected=true; break; }
    }
    Check(PackSelected,TEXT("experience_pack_found_without_aiming"));
    Pack->bConsumed=true;
    Check(PC->FindInteractionTarget()!=Pack,TEXT("consumed_pack_rejected"));
   }
   if (Target) {
    C->SetActorLocation(Target->GetActorLocation()+FVector(300,0,60),false,nullptr,ETeleportType::TeleportPhysics); LookAway();
    auto* HUD=Cast<ACC_ProgressionHUD>(PC->GetHUD());
    Check(HUD && HUD->PickupPromptTexture.LoadSynchronous() && !HUD->IsPickupPromptVisible(),TEXT("third_person_nearby_prompt_hidden"));
    PC->OpenProgressionMenu(); Check(HUD && !HUD->IsPickupPromptVisible(),TEXT("prompt_hidden_in_menu")); PC->CloseProgressionMenu();
    C->SetActorLocation(Target->GetActorLocation()+FVector(1500,0,60),false,nullptr,ETeleportType::TeleportPhysics); LookAway();
    Check(HUD && !HUD->IsPickupPromptVisible(),TEXT("prompt_hidden_out_of_range"));
    C->SetActorLocation(Target->GetActorLocation()+FVector(300,0,60),false,nullptr,ETeleportType::TeleportPhysics);
    Camera->SetActorLocation(C->GetActorLocation()+FVector(200,0,160)); Camera->SetActorRotation(FRotator(-20,180,0)); PC->PlayerCameraManager->UpdateCamera(.1f);
    Check(HUD && !HUD->IsPickupPromptVisible() && HUD->bShowHUD,TEXT("third_person_capture_has_no_observation_prompt"));
    FTimerHandle CaptureTimer; W->GetTimerManager().SetTimer(CaptureTimer,[]() { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/PickupPrompt.png"),true,false); },.7f,false);
   }
  }
  FFileHelper::SaveStringToFile(Passed ? TEXT("{\"passed\":true}") : TEXT("{\"passed\":false}"),*(FPaths::ProjectSavedDir()/TEXT("InteractionSmoke.json")));
  UE_LOG(LogTemp,Display,TEXT("INTERACTION_SMOKE_%s"),Passed ? TEXT("PASS") : TEXT("FAIL"));
  FTimerHandle QuitTimer; W->GetTimerManager().SetTimer(QuitTimer,[W]() { GEngine->Exec(W,TEXT("QUIT")); },1.5f,false);
 },3.f,false);
}
FAutoConsoleCommandWithWorld InteractionSmoke(TEXT("CC.InteractionSmokeTest"),TEXT("Standalone probe: nearby selection, surface reach, wall occlusion, disabled/consumed objects and inventory acquisition. No saves."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunInteractionSmoke));
}
#endif
