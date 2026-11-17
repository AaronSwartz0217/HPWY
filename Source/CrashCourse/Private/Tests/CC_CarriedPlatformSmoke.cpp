#include "Camera/PlayerCameraManager.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerController.h"
#include "World/CC_LevitationPlatform.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Serialization/JsonSerializer.h"
namespace {
struct FCarriedProbe {
 TWeakObjectPtr<ACC_PlayerCharacter> Player;
 TWeakObjectPtr<ACC_LevitationPlatform> Platform;
 TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>();
 int32 Seed=0;
 float BeforeDrop=0;
};
void PressKey(ACC_PlayerController* PC,FKey Key)
{
 PC->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,IE_Pressed,1.f,false,FPlatformTime::Cycles64()));
 FTimerHandle Release;PC->GetWorldTimerManager().SetTimer(Release,FTimerDelegate::CreateWeakLambda(PC,[PC,Key]{PC->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,IE_Released,0.f,false,FPlatformTime::Cycles64()));}),.06f,false);
}
void RunCarriedProbe(UWorld* W)
{
 if(!W||W->GetNetMode()==NM_Client)return;
 auto S=MakeShared<FCarriedProbe>();
 auto Finish=[S,W]{
  bool Pass=S->Report->Values.Num()>=8;for(const auto& V:S->Report->Values)if(V.Value->Type==EJson::Boolean)Pass&=V.Value->AsBool();
  S->Report->SetBoolField(TEXT("passed"),Pass);FString Text;FJsonSerializer::Serialize(S->Report.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));
  const FString Dir=FPaths::ProjectDir()/TEXT("Saved/FirstPerson");IFileManager::Get().MakeDirectory(*Dir,true);FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("CarriedPlatformGameSmoke.json")));
  UE_LOG(LogTemp,Display,TEXT("CARRIED_PLATFORM_GAME_SMOKE %s"),*Text);GEngine->Exec(W,TEXT("QUIT"));
 };
 FTimerHandle Start;W->GetTimerManager().SetTimer(Start,[S,W,Finish]{
  S->Player=Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerPawn(W,0));for(TActorIterator<ACC_LevitationPlatform> It(W);It;++It){S->Platform=*It;break;}
  if(!S->Player.IsValid()||!S->Platform.IsValid()){S->Report->SetBoolField(TEXT("realMapHasPlayerAndPlatform"),false);Finish();return;}
  auto* C=S->Player.Get();auto* P=S->Platform.Get();auto* PC=Cast<ACC_PlayerController>(C->GetController());
  if(!PC){S->Report->SetBoolField(TEXT("nativePlayerController"),false);Finish();return;}
  S->Report->SetBoolField(TEXT("realMapHasPlayerAndPlatform"),true);
  C->TeleportTo(P->GetActorLocation()+FVector(0,0,116),FRotator::ZeroRotator);C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);PC->SetControlRotation(FRotator(-35,0,0));
  FTimerHandle Ridden;W->GetTimerManager().SetTimer(Ridden,[S,W,Finish]{
   auto* C=S->Player.Get();auto* P=S->Platform.Get();auto* PC=Cast<ACC_PlayerController>(C->GetController());
   S->Report->SetBoolField(TEXT("platformCarriesRealPlayer"),P->IsCarrying(C)&&P->CurrentLift>550);
   S->Report->SetBoolField(TEXT("groundedNotFlying"),C->GetCharacterMovement()->MovementMode==MOVE_Walking);
   S->Report->SetNumberField(TEXT("raisedPlayerZ"),C->GetActorLocation().Z);S->BeforeDrop=C->GetActorLocation().Z;
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("Saved/FirstPerson/PlatformThirdPerson.png"),true,false);
   if(auto* Rider=S->Player.Get())Rider->SetFirstPerson(true);
   FTimerHandle First;W->GetTimerManager().SetTimer(First,[S,W,Finish]{
    auto* C=S->Player.Get();auto* P=S->Platform.Get();auto* PC=Cast<ACC_PlayerController>(C->GetController());PC->SetControlRotation(FRotator(-65,-90,0));if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(.1f);
    S->Report->SetBoolField(TEXT("firstPersonOnDemandActivates"),C->IsFirstPerson()&&C->GetFirstPersonCamera()->IsActive()&&C->GetMesh()->bOwnerNoSee);
    S->Report->SetBoolField(TEXT("interactionFinderSelectsWholeScene"),PC->FindInteractionTarget()==PC->GetHeavenSceneTarget()&&PC->GetHeavenSceneTarget()!=nullptr);S->Seed=P->LayoutSeed;PressKey(PC,PC->GetCombatKey(5));
    FTimerHandle Rebuilt;W->GetTimerManager().SetTimer(Rebuilt,[S,W,Finish]{
     auto* C=S->Player.Get();auto* P=S->Platform.Get();auto* PC=Cast<ACC_PlayerController>(C->GetController());
     S->Report->SetBoolField(TEXT("interactionKeyReassemblesPlatform"),P->LayoutSeed!=S->Seed);
     S->Report->SetBoolField(TEXT("reassemblyKeepsPassengerOnDeck"),P->IsCarrying(C));
     FScreenshotRequest::RequestScreenshot(FPaths::ProjectDir()/TEXT("Saved/FirstPerson/PlatformFirstPerson.png"),true,false);
     C->SetFirstPerson(false);PC->SetControlRotation(FRotator(-35,0,0));
     FTimerHandle Restore;W->GetTimerManager().SetTimer(Restore,[S,W,Finish]{
      auto* C=S->Player.Get();S->Report->SetBoolField(TEXT("firstPersonReturnsToTopDown"),!C->IsFirstPerson()&&C->GetTopDownCamera()->IsActive());
      FTimerHandle Drive;W->GetTimerManager().SetTimer(Drive,[S]{if(S->Player.IsValid())S->Player->AddMovementInput(FVector::ForwardVector);},.01f,true);
      FTimerHandle Left;W->GetTimerManager().SetTimer(Left,[S,W,Finish,Drive]() mutable {
       W->GetTimerManager().ClearTimer(Drive);auto* C=S->Player.Get();
       S->Report->SetBoolField(TEXT("walkingOffPhysicallyFalls"),!S->Platform->IsCarrying(C)&&C->GetActorLocation().Z<S->BeforeDrop-50);
       S->Report->SetNumberField(TEXT("afterLeavingZ"),C->GetActorLocation().Z);Finish();
      },2.5f,false);
     },.3f,false);
    },2.5f,false);
   },.4f,false);
  },4.5f,false);
 },3.f,false);
}
FAutoConsoleCommandWithWorld CarriedProbeCommand(TEXT("CC.CarriedPlatformSmokeTest"),TEXT("Explicit real-map input, camera, physical lift and reassembly smoke test; exits its separate process."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunCarriedProbe));
}
#endif
