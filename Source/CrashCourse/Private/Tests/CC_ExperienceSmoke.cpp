#if WITH_DEV_AUTOMATION_TESTS
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Player/CC_PlayerController.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ExperienceOrb.h"
#include "Progression/CC_ExperiencePack.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ConfiguredAbility.h"
#include "Progression/CC_SkillComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstanceController.h"
#include "Materials/MaterialInterface.h"
#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"

namespace
{
struct FExperienceProbe
{
 TWeakObjectPtr<ACC_PlayerState> State;
 TWeakObjectPtr<ACC_PlayerController> Controller;
 TWeakObjectPtr<ACC_PlayerCharacter> Player;
 TWeakObjectPtr<ACC_PlayerCharacter> OriginalPlayer;
 TWeakObjectPtr<ACC_EnemyCharacter> Enemy;
 TWeakObjectPtr<ACC_ExperiencePack> Pack;
 TWeakObjectPtr<ACameraActor> Camera;
 TSet<TWeakObjectPtr<ACC_ExperienceOrb>> SeenOrbs;
 TSet<TWeakObjectPtr<ACC_ExperienceOrb>> PhysicsOrbs;
 TSet<TWeakObjectPtr<ACC_ExperienceOrb>> SeekingOrbs;
 TSet<TWeakObjectPtr<ACC_ExperienceOrb>> NiagaraOrbs;
 TSet<TWeakObjectPtr<ACC_ExperienceOrb>> SingleSpriteOrbs;
 int32 MaximumSpritesPerOrb = 0;
 int32 InitialCount = 0;
 int32 PackCount = 0;
 int32 XPAfterEnemy = -1;
 int32 XPAfterPack = -1;
 bool bNoImmediateEnemyXP = false;
 bool bNoImmediatePackXP = false;
 bool bEarlyCollectionRejected = false;
 bool bWrongRecipientRejected = false;
 bool bEnemyRagdoll = false;
 bool bPlayerRagdoll = false;
 bool bRespawnRestored = false;
 bool bFarPackRejected = false;
 bool bPackConsumedOnce = false;
 bool bLevelHasBlackPack = false;
 bool bEnemyRemovedAfterTenSeconds = false;
 bool bRagdollBoundsSane = true;
 FString SetupError;
 FTimerHandle Monitor;
};
TArray<ACC_ExperienceOrb*> FindOwnedOrbs(UWorld* World, ACC_PlayerState* State)
{
 TArray<ACC_ExperienceOrb*> Orbs;
 for (TActorIterator<ACC_ExperienceOrb> It(World); It; ++It) if (It->Recipient == State) Orbs.Add(*It);
 return Orbs;
}
void FinishExperienceSmoke(const TSharedRef<FExperienceProbe>& Probe, UWorld* World)
{
 World->GetTimerManager().ClearTimer(Probe->Monitor);
 const bool bVisualChecks = FApp::CanEverRender();
 const int32 FinalXP = Probe->State.IsValid() ? Probe->State->Progression->Experience : -1;
 const int32 Remaining = Probe->State.IsValid() ? FindOwnedOrbs(World, Probe->State.Get()).Num() : -1;
 const bool bPass = Probe->SetupError.IsEmpty() && Probe->InitialCount == 10 && Probe->PackCount == 50 && Probe->bNoImmediateEnemyXP && Probe->bNoImmediatePackXP && Probe->bEarlyCollectionRejected && Probe->bWrongRecipientRejected && Probe->XPAfterEnemy == 10 && Probe->XPAfterPack == 60 && FinalXP == 60 && Remaining == 0 && Probe->SeenOrbs.Num() == 60 && Probe->PhysicsOrbs.Num() == 60 && Probe->SeekingOrbs.Num() == 60 && (!bVisualChecks || (Probe->NiagaraOrbs.Num() == 60 && Probe->SingleSpriteOrbs.Num() == 60 && Probe->MaximumSpritesPerOrb == 1)) && Probe->bEnemyRagdoll && Probe->bPlayerRagdoll && Probe->bRespawnRestored && Probe->bFarPackRejected && Probe->bPackConsumedOnce && Probe->bLevelHasBlackPack && Probe->bEnemyRemovedAfterTenSeconds && Probe->bRagdollBoundsSane;
 TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
 Json->SetBoolField(TEXT("passed"), bPass);
 Json->SetStringField(TEXT("setupError"), Probe->SetupError);
 Json->SetBoolField(TEXT("temporaryWalletOnlyNoSave"), true);
 Json->SetNumberField(TEXT("enemyBurstCountAfterRepeatedLethalDamage"), Probe->InitialCount);
 Json->SetNumberField(TEXT("packBurstCountAfterRepeatedInteraction"), Probe->PackCount);
 Json->SetBoolField(TEXT("enemyDoesNotCreditBeforeCollection"), Probe->bNoImmediateEnemyXP);
 Json->SetBoolField(TEXT("packDoesNotCreditBeforeCollection"), Probe->bNoImmediatePackXP);
 Json->SetBoolField(TEXT("scatteringCollectionRejected"), Probe->bEarlyCollectionRejected);
 Json->SetBoolField(TEXT("wrongRecipientCollectionRejected"), Probe->bWrongRecipientRejected);
 Json->SetNumberField(TEXT("experienceAfterEnemyCollection"), Probe->XPAfterEnemy);
 Json->SetNumberField(TEXT("experienceAfterPackCollection"), Probe->XPAfterPack);
 Json->SetNumberField(TEXT("finalExperience"), FinalXP);
 Json->SetNumberField(TEXT("remainingOwnedOrbs"), Remaining);
 Json->SetNumberField(TEXT("observedDistinctOrbActors"), Probe->SeenOrbs.Num());
 Json->SetNumberField(TEXT("observedPhysicsScatterOrbs"), Probe->PhysicsOrbs.Num());
 Json->SetNumberField(TEXT("observedSeekingOrbs"), Probe->SeekingOrbs.Num());
 Json->SetNumberField(TEXT("observedActiveNiagaraComponents"), Probe->NiagaraOrbs.Num());
 Json->SetNumberField(TEXT("orbsWithExactlyOneLiveNiagaraSprite"), Probe->SingleSpriteOrbs.Num());
 Json->SetNumberField(TEXT("maximumLiveSpritesPerOrb"), Probe->MaximumSpritesPerOrb);
 Json->SetBoolField(TEXT("visualChecksSkippedWithoutRenderer"), !bVisualChecks);
 Json->SetBoolField(TEXT("enemyEnteredPhysicalRagdoll"), Probe->bEnemyRagdoll);
 Json->SetBoolField(TEXT("playerEnteredPhysicalRagdoll"), Probe->bPlayerRagdoll);
 Json->SetBoolField(TEXT("respawnRestoredMeshAttachmentCollisionAndMovement"), Probe->bRespawnRestored);
 Json->SetBoolField(TEXT("farPackInteractionRejected"), Probe->bFarPackRejected);
 Json->SetBoolField(TEXT("packConsumedOnce"), Probe->bPackConsumedOnce);
 Json->SetBoolField(TEXT("flatMapHasBlackExperiencePack"), Probe->bLevelHasBlackPack);
 Json->SetBoolField(TEXT("enemyRemovedAfterTenSeconds"), Probe->bEnemyRemovedAfterTenSeconds);
 Json->SetBoolField(TEXT("ragdollBoundsRemainWithin1000cm"), Probe->bRagdollBoundsSane);
 FString Text; FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text));
 FFileHelper::SaveStringToFile(Text, *(FPaths::ProjectSavedDir() / TEXT("ExperienceSmoke.json")));
 UE_LOG(LogTemp, Display, TEXT("EXPERIENCE_SMOKE_%s: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Text);
 for (ACC_ExperienceOrb* Orb : FindOwnedOrbs(World, Probe->State.Get())) Orb->Destroy();
 if (Probe->Enemy.IsValid()) Probe->Enemy->Destroy();
 if (Probe->Pack.IsValid()) Probe->Pack->Destroy();
 if (Probe->Camera.IsValid()) Probe->Camera->Destroy();
 if (Probe->Player.IsValid()) Probe->Player->Destroy();
 if (Probe->Controller.IsValid()) Probe->Controller->Destroy();
 if (Probe->State.IsValid()) Probe->State->Destroy();
 if (GEngine) GEngine->Exec(World, TEXT("QUIT"));
}
void RunExperienceSmoke(UWorld* World)
{
 if (!World || World->GetNetMode() == NM_Client) return;
 const TSharedRef<FExperienceProbe> Probe = MakeShared<FExperienceProbe>();
 FTimerHandle Start;
 World->GetTimerManager().SetTimer(Start, [Probe, World]()
 {
  ACC_PlayerCharacter* Original = Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
  ACC_EnemyCharacter* EnemyTemplate = nullptr;
  for (TActorIterator<ACC_EnemyCharacter> It(World); It; ++It) { EnemyTemplate = *It; break; }
  for (TActorIterator<ACC_ExperiencePack> It(World); It; ++It)
   Probe->bLevelHasBlackPack |= It->ExperienceAmount == 50 && It->Mesh->GetStaticMesh() && It->Mesh->GetMaterial(0) && It->Mesh->GetMaterial(0)->GetName().Contains(TEXT("Black"));
  Probe->OriginalPlayer = Original;
  if (!Original || !EnemyTemplate)
  {
   Probe->SetupError = TEXT("The flat test map must contain the configured player and one enemy.");
   FinishExperienceSmoke(Probe, World); return;
  }
  // A separate controller, PlayerState and Pawn exercise the production pickup path without touching the user's wallet.
  ACC_PlayerState* State = World->SpawnActor<ACC_PlayerState>();
  ACC_PlayerController* Controller = World->SpawnActor<ACC_PlayerController>();
  if (!State || !Controller)
  {
   Probe->SetupError = TEXT("Could not spawn temporary wallet/controller.");
   FinishExperienceSmoke(Probe, World); return;
  }
  Probe->State = State; Probe->Controller = Controller;
  State->SetOwner(Controller); Controller->SetPlayerState(State);
  FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  ACC_PlayerCharacter* Player = World->SpawnActor<ACC_PlayerCharacter>(Original->GetClass(), FVector(-3000.f, -3000.f, 100.f), FRotator::ZeroRotator, Params);
  if (!Player)
  {
   Probe->SetupError = TEXT("Could not spawn the temporary configured player.");
   FinishExperienceSmoke(Probe, World); return;
  }
  Probe->Player = Player;
  Controller->Possess(Player);
  Player->GetCharacterMovement()->StopMovementImmediately();
  if (FApp::CanEverRender())
  {
   const FVector CameraLocation(-3350.f, -4000.f, 500.f);
   ACameraActor* Camera = World->SpawnActor<ACameraActor>(CameraLocation, (FVector(-2600.f, -3000.f, 65.f) - CameraLocation).Rotation());
   Probe->Camera = Camera;
   if (APlayerController* Viewer = World->GetFirstPlayerController(); Viewer && Camera)
   {
    Viewer->SetViewTarget(Camera);
    if (AHUD* HUD = Viewer->GetHUD()) HUD->bShowHUD = false;
   }
  }
  const FTransform EnemyTransform(FRotator::ZeroRotator, FVector(-2250.f, -3000.f, 100.f));
  ACC_EnemyCharacter* Enemy = World->SpawnActorDeferred<ACC_EnemyCharacter>(EnemyTemplate->GetClass(), EnemyTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!Enemy)
  {
   Probe->SetupError = TEXT("Could not spawn the temporary configured enemy.");
   FinishExperienceSmoke(Probe, World); return;
  }
  Probe->Enemy = Enemy;
  Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
  Enemy->ExperienceReward = 10;
  Enemy->ExperienceParticleSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/HPWH/Effects/NS_ExperienceOrb.NS_ExperienceOrb"));
  Enemy->Skills->DefaultLoadout = nullptr; Enemy->Skills->BasicAttack = nullptr;
  if (ACC_PatrolEnemyCharacter* Patrol = Cast<ACC_PatrolEnemyCharacter>(Enemy)) { Patrol->bStartPatrolling = false; Patrol->bDetectPlayers = false; }
  Enemy->FinishSpawning(EnemyTransform);
  World->GetTimerManager().SetTimer(Probe->Monitor, [Probe, World]()
  {
   if (!Probe->State.IsValid()) return;
   for (ACC_ExperienceOrb* Orb : FindOwnedOrbs(World, Probe->State.Get()))
   {
    Probe->SeenOrbs.Add(Orb);
    if (Orb->Phase == ECC_ExperienceOrbPhase::Scattering && Orb->Collision->IsSimulatingPhysics()) Probe->PhysicsOrbs.Add(Orb);
    if (Orb->Phase == ECC_ExperienceOrbPhase::Seeking)
    {
     Probe->SeekingOrbs.Add(Orb);
     if (Probe->OriginalPlayer.IsValid()) Probe->bWrongRecipientRejected |= !Orb->TryCollect(Probe->OriginalPlayer.Get());
    }
    if (Orb->Particles && Orb->Particles->GetAsset() && Orb->Particles->IsActive())
    {
     Probe->NiagaraOrbs.Add(Orb);
     if (FApp::CanEverRender() && !Probe->SingleSpriteOrbs.Contains(Orb))
     {
      const FNiagaraSystemInstanceControllerPtr Simulation = Orb->Particles->GetSystemInstanceController();
      if (Simulation.IsValid() && Simulation->IsValid())
      {
       // Do not inspect mutable emitter buffers while their concurrent tick is running.
       Simulation->WaitForConcurrentTickAndFinalize();
       if (const FNiagaraSystemInstance* Instance = Simulation->GetSystemInstance_Unsafe())
       {
        int32 Sprites = 0;
        for (const auto& Emitter : Instance->GetEmitters()) Sprites += Emitter->GetNumParticles();
        Probe->MaximumSpritesPerOrb = FMath::Max(Probe->MaximumSpritesPerOrb, Sprites);
        if (Sprites == 1) Probe->SingleSpriteOrbs.Add(Orb);
       }
      }
     }
    }
   }
   if (Probe->Enemy.IsValid()) Probe->bEnemyRagdoll |= Probe->Enemy->IsRagdollActive() && Probe->Enemy->GetMesh()->IsSimulatingPhysics();
   if (Probe->Player.IsValid()) Probe->bPlayerRagdoll |= Probe->Player->IsRagdollActive() && Probe->Player->GetMesh()->IsSimulatingPhysics();
   ACC_BaseCharacter* Characters[] = { Probe->Enemy.Get(), Probe->Player.Get() };
   for (ACC_BaseCharacter* Character : Characters)
    if (Character && Character->IsRagdollActive()) Probe->bRagdollBoundsSane &= FMath::IsFinite(Character->GetMesh()->Bounds.SphereRadius) && Character->GetMesh()->Bounds.SphereRadius > 1.f && Character->GetMesh()->Bounds.SphereRadius < 1000.f;
  }, 0.01f, true);
  UCC_ConfiguredAbility::ApplyEffect(Player, Enemy, UCC_DamageEffect::StaticClass(), 100000.f);
  UCC_ConfiguredAbility::ApplyEffect(Player, Enemy, UCC_DamageEffect::StaticClass(), 100000.f);
  const TArray<ACC_ExperienceOrb*> Initial = FindOwnedOrbs(World, State);
  Probe->InitialCount = Initial.Num();
  Probe->bNoImmediateEnemyXP = State->Progression->Experience == 0;
  Probe->bEarlyCollectionRejected = !Initial.IsEmpty() && !Initial[0]->TryCollect(Player);
  FTimerHandle EnemyCapture;
  World->GetTimerManager().SetTimer(EnemyCapture, []() { if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/ExperienceEnemy.png"), false, false); }, 0.2f, false);
  FTimerHandle RagdollCapture;
  World->GetTimerManager().SetTimer(RagdollCapture, []() { if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/ExperienceRagdoll.png"), false, false); }, 1.5f, false);
  FTimerHandle SpawnPack;
  World->GetTimerManager().SetTimer(SpawnPack, [Probe, World]()
  {
   if (!Probe->State.IsValid() || !Probe->Controller.IsValid() || !Probe->Player.IsValid()) return;
   ACC_PlayerCharacter* Player = Probe->Player.Get();
   Probe->XPAfterEnemy = Probe->State->Progression->Experience;
   ACC_ExperiencePack* Pack = World->SpawnActor<ACC_ExperiencePack>(Player->GetActorLocation() + FVector(800.f, 0.f, 0.f), FRotator::ZeroRotator);
   if (!Pack) return;
   Probe->Pack = Pack;
   Pack->ExperienceParticleSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/HPWH/Effects/NS_ExperienceOrb.NS_ExperienceOrb"));
   Probe->Controller->RequestInteraction(Pack);
   Probe->bFarPackRejected = !Pack->bConsumed && FindOwnedOrbs(World, Probe->State.Get()).IsEmpty();
   Pack->SetActorLocation(Player->GetActorLocation() + FVector(220.f, 0.f, -30.f));
   Probe->Controller->RequestInteraction(Pack);
   Probe->Controller->RequestInteraction(Pack);
   Probe->PackCount = FindOwnedOrbs(World, Probe->State.Get()).Num();
   Probe->bPackConsumedOnce = Pack->bConsumed && !Pack->Interact(Player) && Pack->IsHidden();
   Probe->bNoImmediatePackXP = Probe->State->Progression->Experience == 10;
  }, 5.f, false);
  FTimerHandle PackCapture;
  World->GetTimerManager().SetTimer(PackCapture, []() { if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/ExperiencePack.png"), false, false); }, 5.15f, false);
  FTimerHandle KillPlayer;
  World->GetTimerManager().SetTimer(KillPlayer, [Probe]()
  {
   if (!Probe->State.IsValid() || !Probe->Player.IsValid()) return;
   Probe->XPAfterPack = Probe->State->Progression->Experience;
   UCC_ConfiguredAbility::ApplyEffect(Probe->Player.Get(), Probe->Player.Get(), UCC_DamageEffect::StaticClass(), 100000.f);
  }, 9.f, false);
  FTimerHandle Respawn;
  World->GetTimerManager().SetTimer(Respawn, [Probe]()
  {
   if (!Probe->Player.IsValid()) return;
   ACC_PlayerCharacter* Player = Probe->Player.Get();
   Player->ResetAttributes(); Player->HandleRespawn();
   Probe->bRespawnRestored = Player->IsAlive() && !Player->IsRagdollActive() && !Player->GetMesh()->IsSimulatingPhysics() && Player->GetMesh()->GetAttachParent() == Player->GetCapsuleComponent() && Player->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision && Player->GetCharacterMovement()->MovementMode != MOVE_None;
  }, 11.f, false);
  FTimerHandle Finish;
  World->GetTimerManager().SetTimer(Finish, [Probe, World]()
  {
   Probe->bEnemyRemovedAfterTenSeconds = !Probe->Enemy.IsValid();
   FinishExperienceSmoke(Probe, World);
  }, 16.f, false);
 }, 3.f, false);
}
FAutoConsoleCommandWithWorld ExperienceSmokeCommand(TEXT("CC.ExperienceSmokeTest"), TEXT("Flat-map live physics/Niagara pickup and ragdoll test using a temporary wallet. Writes Saved/ExperienceSmoke.json, then exits without saving."), FConsoleCommandWithWorldDelegate::CreateStatic(&RunExperienceSmoke));
}
#endif
