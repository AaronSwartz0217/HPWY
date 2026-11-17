#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "World/CC_InteractiveDoubleDoor.h"
#include "World/CC_DoorVisibilityManager.h"
#include "World/CC_RoomVisibilityController.h"
#include "Player/CC_PlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "Characters/CC_PlayerCharacter.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Progression/CC_ProgressionHUD.h"
#include "UI/CC_FirstPersonScanWidget.h"
#include "Misc/App.h"
namespace {
void DoorSmoke(UWorld* W)
{
 if(!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone)return;
 FTimerHandle Start;W->GetTimerManager().SetTimer(Start,[W]()
 {
  auto Checks=MakeShared<FJsonObject>();auto Doors=MakeShared<TArray<ACC_InteractiveDoubleDoor*>>();
  for(TActorIterator<ACC_InteractiveDoubleDoor> It(W);It;++It)Doors->Add(*It);
  auto* PC=Cast<ACC_PlayerController>(W->GetFirstPlayerController());APawn* Pawn=PC?PC->GetPawn():nullptr;
  Checks->SetBoolField(TEXT("three_doors_and_player_ready"),Doors->Num()==3 && Pawn);
  ACC_DoorVisibilityManager* Manager=nullptr;for(TActorIterator<ACC_DoorVisibilityManager> It(W);It;++It){Manager=*It;break;}
  if(Manager)
  {
   if(auto* Movement=Pawn?Pawn->GetMovementComponent():nullptr)Movement->Deactivate();
   if(Pawn)
   {
    const FVector Old=Pawn->GetActorLocation();Pawn->SetActorLocation(FVector(3300,350,50),false,nullptr,ETeleportType::TeleportPhysics);Manager->RefreshVisibility();
    bool Hidden=true;for(auto& R:Manager->Rooms)Hidden &= !R.Visibility->bRoomActive;
    Checks->SetBoolField(TEXT("outside_closed_hides_all_three_interiors"),Hidden && Manager->ExteriorDetails->bRoomActive);
    for(int32 I=0;I<Manager->Rooms.Num();++I)
    {
     Pawn->SetActorLocation(Manager->Rooms[I].Center,false,nullptr,ETeleportType::TeleportPhysics);Manager->RefreshVisibility();
     bool Correct=!Manager->ExteriorDetails->bRoomActive;
     for(int32 J=0;J<Manager->Rooms.Num();++J)Correct &= Manager->Rooms[J].Visibility->bRoomActive==(J==I);
     Checks->SetBoolField(FString::Printf(TEXT("inside_closed_room_%d_only_and_exterior_off"),I+1),Correct);
    }
    Manager->SetOptimizationEnabled(false);bool All=Manager->ExteriorDetails->bRoomActive;for(auto& R:Manager->Rooms)All &= R.Visibility->bRoomActive;
    Checks->SetBoolField(TEXT("disable_optimization_restores_all"),All);Manager->SetOptimizationEnabled(true);
    Pawn->SetActorLocation(Old,false,nullptr,ETeleportType::TeleportPhysics);Manager->RefreshVisibility();
   }
  }
  auto Finish=[W,Checks]() {bool Passed=true;for(const auto& P:Checks->Values)Passed &= P.Value->AsBool();
   auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Passed);Root->SetObjectField(TEXT("checks"),Checks);
   FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("DoorInteractionSmoke.json")));
   UE_LOG(LogTemp,Display,TEXT("DOOR_INTERACTION_%s"),Passed?TEXT("PASS"):TEXT("FAIL"));GEngine->Exec(W,TEXT("QUIT"));};
  if(!Pawn || Doors->Num()!=3){Finish();return;}
  if(auto* Movement=Pawn->GetMovementComponent())Movement->Deactivate();Pawn->SetActorEnableCollision(false);
  auto Run=MakeShared<TFunction<void(int32)>>();TWeakPtr<TFunction<void(int32)>> WeakRun=Run;
  *Run=[W,PC,Pawn,Doors,Checks,Finish,WeakRun](int32 Index)
  {
   auto KeepAlive=WeakRun.Pin();if(Index>=Doors->Num()*2){Finish();return;}
   if(auto* Character=Cast<ACC_PlayerCharacter>(Pawn))Character->SetFirstPerson(Index>=Doors->Num());
   auto* Door=(*Doors)[Index%Doors->Num()];TInlineComponentArray<USceneComponent*> Components(Door);USceneComponent* L=nullptr;USceneComponent* R=nullptr;
   for(auto* C:Components){if(C->GetFName()==Door->LeftDoorComponentName)L=C;if(C->GetFName()==Door->RightDoorComponentName)R=C;}
   if(!L||!R){Checks->SetBoolField(TEXT("components_exist"),false);Finish();return;}
   const FRotator LC=L->GetRelativeRotation(),RC=R->GetRelativeRotation();const FString Prefix=FString::Printf(TEXT("door_%d_"),Index+1);
   Pawn->SetActorLocation(Door->GetActorLocation()+Door->GetActorRightVector()*180+FVector(0,0,100),false,nullptr,ETeleportType::TeleportPhysics);
   PC->SetControlRotation((Door->GetActorLocation()+FVector(0,0,150)-Pawn->GetPawnViewLocation()).Rotation());
   auto PressE=[PC]() {for(const auto& B:PC->InputComponent->KeyBindings)if(B.Chord.Key==EKeys::E && B.KeyEvent==IE_Pressed){B.KeyDelegate.Execute(EKeys::E);return true;}return false;};
   FTimerHandle Aim;W->GetTimerManager().SetTimer(Aim,[W,PC,Door,L,R,LC,RC,Prefix,Checks,PressE,KeepAlive,Index]()
   {
    Checks->SetBoolField(Prefix+TEXT("target_selected"),PC->FindInteractionTarget()==Door);
    if(FApp::CanEverRender())
    {
     auto* HUD=Cast<ACC_ProgressionHUD>(PC->GetHUD());
     Checks->SetBoolField(Prefix+TEXT("visible_e_prompt_ready"),HUD && HUD->FirstPersonScanHUD && HUD->FirstPersonScanHUD->GetDisplayedTarget()==Door && HUD->FirstPersonScanHUD->IsInteractionReady());
    }
    Checks->SetBoolField(Prefix+TEXT("e_bound"),PressE());
    PressE();Checks->SetBoolField(Prefix+TEXT("repeat_during_animation_ignored"),Door->bOpen && Door->bAnimating);
    for(TActorIterator<ACC_DoorVisibilityManager> It(W);It;++It)
    {
     bool Visible=It->ExteriorDetails->bRoomActive;for(auto& Zone:It->Rooms)if(Zone.Door==Door)Visible &= Zone.Visibility->bRoomActive;
     Checks->SetBoolField(Prefix+TEXT("opening_restores_both_sides_immediately"),Visible);
    }
    FTimerHandle Open;W->GetTimerManager().SetTimer(Open,[W,Door,L,R,LC,RC,Prefix,Checks,PressE,KeepAlive,Index]()
    {
     Checks->SetBoolField(Prefix+TEXT("relative_90_degree_open"),Door->bOpen && !Door->bAnimating && FMath::IsNearlyEqual(FMath::FindDeltaAngleDegrees(LC.Yaw,L->GetRelativeRotation().Yaw),90.f,.1f) && FMath::IsNearlyEqual(FMath::FindDeltaAngleDegrees(RC.Yaw,R->GetRelativeRotation().Yaw),-90.f,.1f));
     PressE();FTimerHandle Close;W->GetTimerManager().SetTimer(Close,[Door,L,R,LC,RC,Prefix,Checks,KeepAlive,Index]()
     {Checks->SetBoolField(Prefix+TEXT("returns_to_original_rotation"),!Door->bOpen && !Door->bAnimating && L->GetRelativeRotation().Equals(LC,.1f) && R->GetRelativeRotation().Equals(RC,.1f) && !Door->IsActorTickEnabled());(*KeepAlive)(Index+1);},1.1f,false);
    },1.1f,false);
   },.5f,false);
  };
  (*Run)(0);
 },3.f,false);
}
FAutoConsoleCommandWithWorld DoorSmokeCommand(TEXT("CC.DoorInteractionSmokeTest"),TEXT("Test the three prepared doors via actual E bindings."),FConsoleCommandWithWorldDelegate::CreateStatic(&DoorSmoke));
FAutoConsoleCommandWithWorldAndArgs DoorRenderBenchmarkToggle(TEXT("CC.RenderOptimization"),TEXT("0/1: temporarily disable/enable door-driven visibility for profiling."),FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args,UWorld* W)
{
 if(!W || !W->IsGameWorld() || Args.Num()!=1)return;
 const bool Enabled=Args[0].ToBool();int32 Count=0;
 for(TActorIterator<ACC_DoorVisibilityManager> It(W);It;++It){It->SetOptimizationEnabled(Enabled);++Count;}
 CSV_EVENT_GLOBAL(TEXT("DOOR_RENDER_OPT_%s"),Enabled?TEXT("ON"):TEXT("OFF"));
 UE_LOG(LogTemp,Display,TEXT("DOOR_RENDER_BENCH: enabled=%d managers=%d"),Enabled,Count);
}));
}
#endif
