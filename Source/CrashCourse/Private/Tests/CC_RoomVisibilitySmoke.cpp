#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Light.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "EngineUtils.h"
#include "Components/LightComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "World/CC_RoomVisibilityController.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
namespace {
void RunRoomVisibilitySmoke(UWorld* W)
{
 if(!W || !W->IsGameWorld() || W->GetNetMode()!=NM_Standalone) return;
 FTimerHandle Start; W->GetTimerManager().SetTimer(Start,[W]()
 {
  auto Checks=MakeShared<FJsonObject>();
  APawn* Player=UGameplayStatics::GetPlayerPawn(W,0);
  ACC_RoomVisibilityController* Room=nullptr;
  for(TActorIterator<ACC_RoomVisibilityController> It(W);It;++It){Room=*It;break;}
  Checks->SetBoolField(TEXT("player_and_installed_controller"),Player && Room);
  auto Finish=[W,Checks]()
  {
   bool Passed=true;for(const auto& Pair:Checks->Values) Passed &= Pair.Value->AsBool();
   auto Root=MakeShared<FJsonObject>();Root->SetBoolField(TEXT("passed"),Passed);Root->SetObjectField(TEXT("checks"),Checks);
   FString Json;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
   FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("RoomVisibilityRuntimeSmoke.json")));
   UE_LOG(LogTemp,Display,TEXT("ROOM_VISIBILITY_RUNTIME_%s"),Passed?TEXT("PASS"):TEXT("FAIL"));
   GEngine->Exec(W,TEXT("QUIT"));
  };
  if(!Player || !Room){Finish();return;}
  Checks->SetBoolField(TEXT("installed_136_interiors_2_lights"),Room->InteriorActors.Num()==136 && Room->RoomLights.Num()==2);
  Player->SetActorHiddenInGame(true);
  if(auto* Movement=Player->GetMovementComponent()) Movement->Deactivate();
  if(auto* PC=Cast<APlayerController>(Player->GetController()))
  {
   const FVector CameraPosition(1450,350,80);
   auto* Camera=W->SpawnActor<ACameraActor>(CameraPosition,(FVector(1000,350,-20)-CameraPosition).Rotation());
   if(Camera) PC->SetViewTarget(Camera);
  }
  const FTransform Original=Player->GetActorTransform();
  Player->SetActorLocation(Room->TriggerBox->GetComponentLocation(),false,nullptr,ETeleportType::TeleportPhysics);
  FTimerHandle Inside; W->GetTimerManager().SetTimer(Inside,[W,Player,Room,Original,Checks,Finish]()
  {
   bool Visible=true;for(AActor* A:Room->InteriorActors) Visible &= IsValid(A) && !A->IsHidden();
   bool Lights=true;for(ALight* A:Room->RoomLights) Lights &= IsValid(A) && A->GetLightComponent()->IsVisible();
   Checks->SetBoolField(TEXT("inside_poll_activates"),Room->bRoomActive);
   Checks->SetBoolField(TEXT("inside_interiors_and_lights_visible"),Visible && Lights);
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RoomVisibility_Inside.png"),false,false);
   FTimerHandle Move; W->GetTimerManager().SetTimer(Move,[W,Player,Room,Original,Checks,Finish]()
   {
    Player->SetActorLocation(Room->GetActorLocation()+FVector(5000,0,0),false,nullptr,ETeleportType::TeleportPhysics);
    FTimerHandle Outside; W->GetTimerManager().SetTimer(Outside,[W,Player,Room,Original,Checks,Finish]()
    {
    bool Hidden=true;for(AActor* A:Room->InteriorActors) Hidden &= IsValid(A) && A->IsHidden();
    bool LightsOff=true;for(ALight* A:Room->RoomLights) LightsOff &= IsValid(A) && !A->GetLightComponent()->IsVisible();
    Checks->SetBoolField(TEXT("outside_poll_deactivates"),!Room->bRoomActive);
    Checks->SetBoolField(TEXT("outside_interiors_hidden_lights_off"),Hidden && LightsOff);
     FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("RoomVisibility_Outside.png"),false,false);
     FTimerHandle End;W->GetTimerManager().SetTimer(End,[Player,Original,Finish]()
     {Player->SetActorTransform(Original,false,nullptr,ETeleportType::TeleportPhysics);Finish();},2.f,false);
    },3.f,false);
   },2.f,false);
  },3.f,false);
 },3.f,false);
}
FAutoConsoleCommandWithWorld RoomVisibilitySmoke(TEXT("CC.RoomVisibilitySmokeTest"),TEXT("Tests the installed room controller in an isolated standalone trial map."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunRoomVisibilitySmoke));
}
#endif
