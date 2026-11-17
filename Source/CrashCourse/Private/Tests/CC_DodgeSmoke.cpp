#if WITH_DEV_AUTOMATION_TESTS
#include "Characters/CC_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "Player/CC_PlayerController.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
void RunDodgeSmoke(UWorld* World)
{
 if(!World || !World->IsGameWorld())return;
 FTimerHandle Initial;
 World->GetTimerManager().SetTimer(Initial,[World]()
 {
  auto* PC=Cast<ACC_PlayerController>(World->GetFirstPlayerController());
  auto* Player=PC?Cast<ACC_PlayerCharacter>(PC->GetPawn()):nullptr;
  if(!Player)return;
  Player->SetActorLocation(FVector(-2400,-2400,110));
  Player->SetActorRotation(FRotator::ZeroRotator);
  PC->SetControlRotation(FRotator(-8.f,90.f,0.f));
  Player->GetCameraBoom()->TargetArmLength=900.f;
  auto Check=[](const TCHAR* Name,bool bPassed){UE_LOG(LogTemp,Display,TEXT("DODGE_SMOKE %s %d"),Name,bPassed);};
  auto Later=[World](float Delay,TFunction<void()> Action)
  {
   FTimerHandle Handle;
   World->GetTimerManager().SetTimer(Handle,MoveTemp(Action),Delay,false);
  };
  auto Key=[PC](FKey Input,EInputEvent Event)
  {
   PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,Input,Event,Event==IE_Pressed?1.f:0.f,false,FPlatformTime::Cycles64()));
  };
  auto Start=MakeShared<FVector>();
  Later(.4f,[Player,Start,Key](){*Start=Player->GetActorLocation();Key(EKeys::LeftShift,IE_Pressed);});
  Later(.49f,[Player,Check,Key](){Check(TEXT("tap_never_starts_sprint"),!Player->IsSprinting());Key(EKeys::LeftShift,IE_Released);});
  Later(.56f,[World,Player,Start,Check]()
  {
   Check(TEXT("tap_moves_three_metres"),FMath::IsNearlyEqual(FVector::Dist2D(*Start,Player->GetActorLocation()),300.0,8.0));
   int32 Poses=0;
   for(TActorIterator<AActor> It(World);It;++It)
   {
    if(!It->ActorHasTag(TEXT("CC.DodgeTrail")))continue;
    TInlineComponentArray<UPoseableMeshComponent*> Components(*It);
    Poses+=Components.Num();
   }
   Check(TEXT("afterimage_contains_two_pose_snapshots"),Poses==2);
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/DodgeAfterimage.png"),true,false);
   const FVector Before=Player->GetActorLocation();
   Player->RequestDodge(FVector::ForwardVector);
   Check(TEXT("cooldown_blocks_immediate_repeat"),FVector::Dist2D(Before,Player->GetActorLocation())<1.0);
  });
  Later(1.0f,[Player,Key,Start](){*Start=Player->GetActorLocation();Key(EKeys::RightShift,IE_Pressed);});
  Later(1.32f,[Player,Check](){Check(TEXT("hold_starts_sprint"),Player->IsSprinting());});
  Later(1.40f,[Key](){Key(EKeys::RightShift,IE_Released);});
  Later(1.49f,[Player,Start,Check]()
  {
   Check(TEXT("hold_release_stops_without_dodging"),!Player->IsSprinting() && FVector::Dist2D(*Start,Player->GetActorLocation())<8.0);
  });
  Later(1.75f,[World,Player,Check]()
  {
   const FVector Before=Player->GetActorLocation();
   auto* Wall=World->SpawnActor<AActor>();
   auto* Box=NewObject<UBoxComponent>(Wall);
   Wall->SetRootComponent(Box);
   Box->SetBoxExtent(FVector(12,150,160));
   Box->SetCollisionProfileName(TEXT("BlockAll"));
   Box->RegisterComponent();
   Wall->SetActorLocation(Before+FVector(165,0,0));
   Player->RequestDodge(FVector::ForwardVector);
   const float Distance=FVector::Dist2D(Before,Player->GetActorLocation());
   Check(TEXT("wall_limits_the_dodge"),Distance>10.f && Distance<165.f);
  });
  Later(2.2f,[World](){GEngine->Exec(World,TEXT("QUIT"));});
 },3.f,false);
}
FAutoConsoleCommandWithWorld DodgeSmoke(TEXT("CC.DodgeSmokeTest"),TEXT("Tests Shift tap, hold, afterimage, cooldown and wall collision."),FConsoleCommandWithWorldDelegate::CreateStatic(&RunDodgeSmoke));
}
#endif
