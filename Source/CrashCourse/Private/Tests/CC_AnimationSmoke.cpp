#if WITH_DEV_AUTOMATION_TESTS
#include "Characters/CC_PlayerCharacter.h"
#include "Characters/CC_EnemyCharacter.h"
#include "Characters/CC_PatrolEnemyCharacter.h"
#include "Characters/CC_CharacterAnimInstance.h"
#include "Characters/CC_LocomotionComponent.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_SkillProjectile.h"
#include "Progression/CC_GameplayData.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"

namespace
{
struct FAnimationActorProbe
{
	TWeakObjectPtr<ACC_BaseCharacter> Character;
	TSet<FName> States;
	bool bBlueprint = false;
	bool bClipsMatch = false;
	bool bMontage = false;
	bool bObservedRunBool = false;
	float MaxSpeed = 0.f;
	float MaxAnimSpeed = 0.f;
	TSet<FString> MovementModes;
	bool bHasController = false;
};
struct FAnimationProbe
{
	TWeakObjectPtr<ACC_PlayerCharacter> Player;
	TWeakObjectPtr<ACC_EnemyCharacter> Enemy;
	TWeakObjectPtr<ACC_SkillProjectile> Projectile;
	TArray<FAnimationActorProbe> Actors;
	TStrongObjectPtr<UCC_SkillDefinition> ProjectileSkill;
	TStrongObjectPtr<UCC_SkillDefinition> CancelSkill;
	TStrongObjectPtr<UCC_SkillLoadout> CancelLoadout;
	TStrongObjectPtr<UCC_SkillLoadout> OriginalLoadout;
	TWeakObjectPtr<ACC_EnemyCharacter> CancelTarget;
	float CancelTargetHealth = 0.f;
	bool bCancelAttackAccepted = false;
	bool bCancelMontageStarted = false;
	bool bCancelPreservedHealth = false;
	bool bCancelStoppedMontage = false;
	bool bForceEnemyRun = false;
	bool bPositiveAttackAccepted = false;
	bool bPositiveWaitedForWindup = false;
	bool bPositiveDealtDamage = false;
	bool bPlayerAttackAccepted = false;
	bool bEnemyAttackAccepted = false;
	bool bProjectileTrail = false;
	bool bCastEffect = false;
	bool bImpactEffect = false;
	FTimerHandle Monitor;
};
// Observe live components, rather than treating a configured asset reference as a played effect.
bool HasLiveEffect(UWorld* World, UCC_SkillDefinition* Skill, bool bImpact)
{
	if (!Skill) return false;
 if(Skill->ProjectileCoreMaterial)for(TObjectIterator<UStaticMeshComponent> It;It;++It)if(It->GetWorld()==World&&It->IsVisible()&&It->GetMaterial(0)==Skill->ProjectileCoreMaterial){const bool Impact=It->GetOwner()&&It->GetOwner()->ActorHasTag(TEXT("CC.ParticleImpactFX"));if(Impact==bImpact)return true;}
	const UNiagaraSystem* Niagara = bImpact ? Skill->ImpactNiagara : Skill->CastNiagara;
	const UParticleSystem* Particles = bImpact ? Skill->ImpactParticles : Skill->CastParticles;
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
		if (Niagara && It->GetWorld() == World && It->GetAsset() == Niagara && It->IsActive()) return true;
	for (TObjectIterator<UParticleSystemComponent> It; It; ++It)
		if (Particles && It->GetWorld() == World && It->Template == Particles && It->IsActive()) return true;
	return false;
}
bool HasJumpCycle(const FAnimationActorProbe* Actor)
{
	return Actor && Actor->States.Contains(TEXT("Jump")) && Actor->States.Contains(TEXT("Fall")) && Actor->States.Contains(TEXT("Land"));
}
void FinishAnimationSmoke(const TSharedRef<FAnimationProbe>& Probe, UWorld* World)
{
	World->GetTimerManager().ClearTimer(Probe->Monitor);
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	bool bAllBlueprints = !Probe->Actors.IsEmpty(), bAllClipsMatch = !Probe->Actors.IsEmpty(), bNPCRan = false;
	const FAnimationActorProbe* Player = nullptr;
	const FAnimationActorProbe* Enemy = nullptr;
	TArray<TSharedPtr<FJsonValue>> ActorResults;
	for (const FAnimationActorProbe& Actor : Probe->Actors)
	{
		bAllBlueprints &= Actor.bBlueprint;
		bAllClipsMatch &= Actor.bClipsMatch;
		if (Actor.Character.Get() == Probe->Player.Get()) Player = &Actor;
		if (Actor.Character.Get() == Probe->Enemy.Get()) Enemy = &Actor;
		if (Cast<ACC_EnemyCharacter>(Actor.Character.Get()) && Actor.States.Contains(TEXT("Run"))) bNPCRan = true;
		TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("actor"), Actor.Character.IsValid() ? Actor.Character->GetName() : TEXT("destroyed"));
		Item->SetBoolField(TEXT("correctAnimationBlueprint"), Actor.bBlueprint);
		Item->SetBoolField(TEXT("clipsMatchMeshSkeleton"), Actor.bClipsMatch);
		Item->SetBoolField(TEXT("playedAttackMontage"), Actor.bMontage);
		Item->SetBoolField(TEXT("observedRunBool"), Actor.bObservedRunBool);
		Item->SetNumberField(TEXT("maxSpeed"), Actor.MaxSpeed);
		Item->SetNumberField(TEXT("maxAnimationSpeed"), Actor.MaxAnimSpeed);
		Item->SetBoolField(TEXT("hadController"), Actor.bHasController);
		TArray<TSharedPtr<FJsonValue>> Modes;
		for (const FString& Mode : Actor.MovementModes) Modes.Add(MakeShared<FJsonValueString>(Mode));
		Item->SetArrayField(TEXT("movementModes"), Modes);
		TArray<TSharedPtr<FJsonValue>> States;
		TArray<FName> Names = Actor.States.Array(); Names.Sort(FNameLexicalLess());
		for (FName State : Names) States.Add(MakeShared<FJsonValueString>(State.ToString()));
		Item->SetArrayField(TEXT("observedGraphStates"), States);
		ActorResults.Add(MakeShared<FJsonValueObject>(Item));
	}
	const bool bPlayerJump = HasJumpCycle(Player), bEnemyJump = HasJumpCycle(Enemy);
	const bool bPlayerMontage = Player && Player->bMontage, bEnemyMontage = Enemy && Enemy->bMontage;
	const bool bVisualChecks = FApp::CanEverRender();
	const bool bCancelPass = Probe->bCancelAttackAccepted && Probe->bCancelMontageStarted && Probe->bCancelPreservedHealth && Probe->bCancelStoppedMontage && Probe->bPositiveAttackAccepted && Probe->bPositiveWaitedForWindup && Probe->bPositiveDealtDamage;
	const bool bPass = Probe->Player.IsValid() && Probe->Enemy.IsValid() && bAllBlueprints && bAllClipsMatch && bNPCRan && bPlayerJump && bEnemyJump && Probe->bPlayerAttackAccepted && Probe->bEnemyAttackAccepted && bPlayerMontage && bEnemyMontage && bCancelPass && (!bVisualChecks || (Probe->bProjectileTrail && Probe->bCastEffect && Probe->bImpactEffect));
	Json->SetBoolField(TEXT("passed"), bPass);
	Json->SetNumberField(TEXT("characterCount"), Probe->Actors.Num());
	Json->SetBoolField(TEXT("allUseCorrectAnimationBlueprint"), bAllBlueprints);
	Json->SetBoolField(TEXT("allClipsMatchMeshSkeleton"), bAllClipsMatch);
	Json->SetBoolField(TEXT("npcEnteredRunState"), bNPCRan);
	Json->SetBoolField(TEXT("playerJumpFallLand"), bPlayerJump);
	Json->SetBoolField(TEXT("npcJumpFallLand"), bEnemyJump);
	Json->SetBoolField(TEXT("playerBasicAttackAccepted"), Probe->bPlayerAttackAccepted);
	Json->SetBoolField(TEXT("npcSlotAttackAccepted"), Probe->bEnemyAttackAccepted);
	Json->SetBoolField(TEXT("playerPlayedAttackMontage"), bPlayerMontage);
	Json->SetBoolField(TEXT("npcPlayedAttackMontage"), bEnemyMontage);
	Json->SetBoolField(TEXT("projectileHasVisibleFlightEffect"), Probe->bProjectileTrail);
	Json->SetBoolField(TEXT("visualChecksSkippedWithoutRenderer"), !bVisualChecks);
	Json->SetBoolField(TEXT("castEffectSpawned"), Probe->bCastEffect);
	Json->SetBoolField(TEXT("impactEffectSpawned"), Probe->bImpactEffect);
	Json->SetBoolField(TEXT("windupCancellationPassed"), bCancelPass);
	Json->SetBoolField(TEXT("windupAttackAccepted"), Probe->bCancelAttackAccepted);
	Json->SetBoolField(TEXT("windupMontageStarted"), Probe->bCancelMontageStarted);
	Json->SetBoolField(TEXT("cancelledWindupPreservedTargetHealth"), Probe->bCancelPreservedHealth);
	Json->SetBoolField(TEXT("cancelledWindupStoppedMontage"), Probe->bCancelStoppedMontage);
	Json->SetBoolField(TEXT("uncancelledWindupAttackAccepted"), Probe->bPositiveAttackAccepted);
	Json->SetBoolField(TEXT("uncancelledAttackWaitedForWindup"), Probe->bPositiveWaitedForWindup);
	Json->SetBoolField(TEXT("uncancelledWindupDealtDamage"), Probe->bPositiveDealtDamage);
	Json->SetArrayField(TEXT("actors"), ActorResults);
	FString Text; FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text));
	FFileHelper::SaveStringToFile(Text, *(FPaths::ProjectSavedDir() / TEXT("AnimationSmoke.json")));
	UE_LOG(LogTemp, Display, TEXT("ANIMATION_SMOKE_%s: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Text);
	if (GEngine) GEngine->Exec(World, TEXT("QUIT"));
}
void RunAnimationSmoke(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client) return;
	const TSharedRef<FAnimationProbe> Probe = MakeShared<FAnimationProbe>();
	FTimerHandle Start;
	World->GetTimerManager().SetTimer(Start, [Probe, World]()
	{
		Probe->Player = Cast<ACC_PlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
		for (TActorIterator<ACC_BaseCharacter> It(World); It; ++It)
		{
			if (!It->GetMesh()->GetSkeletalMeshAsset()) continue;
			FAnimationActorProbe Actor; Actor.Character = *It; Probe->Actors.Add(MoveTemp(Actor));
			if (ACC_PatrolEnemyCharacter* Patrol = Cast<ACC_PatrolEnemyCharacter>(*It))
			{
				Patrol->bDetectPlayers = false;
				if (!Probe->Enemy.IsValid()) Probe->Enemy = Patrol;
			}
		}
		World->GetTimerManager().SetTimer(Probe->Monitor, [Probe, World]()
		{
			if (Probe->bForceEnemyRun && Probe->Enemy.IsValid())
			{
				UCharacterMovementComponent* Movement = Probe->Enemy->GetCharacterMovement();
				Movement->RequestDirectMove(FVector(0.f, 180.f, 0.f), false);
				Movement->Velocity = FVector(0.f, 180.f, Movement->Velocity.Z);
			}
			for (FAnimationActorProbe& Actor : Probe->Actors)
			{
				ACC_BaseCharacter* Character = Actor.Character.Get();
				if (!Character || !Character->Locomotion) continue;
				Actor.MaxSpeed = FMath::Max(Actor.MaxSpeed, Character->GetVelocity().Size2D());
				Actor.bHasController |= Character->GetController() != nullptr;
				Actor.MovementModes.Add(Character->GetCharacterMovement()->GetMovementName());
				UCC_CharacterAnimInstance* Anim = Cast<UCC_CharacterAnimInstance>(Character->GetMesh()->GetAnimInstance());
				const FCC_MeshLocomotion* Entry = Character->Locomotion->GetCurrentEntry();
				if (!Entry || !Anim || !Entry->AnimClass || !Anim->IsA(Entry->AnimClass)) continue;
				Actor.bObservedRunBool |= Anim->bRun;
				Actor.MaxAnimSpeed = FMath::Max(Actor.MaxAnimSpeed, Anim->Speed);
				const FAnimNode_StateMachine* Machine = Anim->GetStateMachineInstanceFromName(TEXT("CharacterLocomotion"));
				Actor.bBlueprint = Machine != nullptr;
				if (Machine) Actor.States.Add(Machine->GetCurrentStateName());
				Actor.bClipsMatch = true;
				const UAnimSequence* Clips[] = { Entry->Idle, Entry->Run, Entry->Jump, Entry->Fall, Entry->Land, Entry->Death, Entry->Jab, Entry->CrossJab, Entry->Uppercut };
				for (const UAnimSequence* Clip : Clips) Actor.bClipsMatch &= Clip && Clip->GetSkeleton() == Character->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
				Actor.bMontage |= Anim->IsAnyMontagePlaying();
			}
			if (ACC_SkillProjectile* Projectile = Probe->Projectile.Get())
			{
				TArray<UStaticMeshComponent*> Cores;Projectile->GetComponents(Cores);for(auto* Core:Cores)Probe->bProjectileTrail|=Probe->ProjectileSkill->ProjectileCoreMaterial&&Core->GetMaterial(0)==Probe->ProjectileSkill->ProjectileCoreMaterial&&Core->IsVisible();
                TArray<UNiagaraComponent*> Trails; Projectile->GetComponents(Trails);
				for (UNiagaraComponent* Trail : Trails) Probe->bProjectileTrail |= Trail->GetAsset() == Probe->ProjectileSkill->ProjectileNiagara && Trail->IsActive();
			}
			if (Probe->ProjectileSkill.Get())
			{
				Probe->bCastEffect |= HasLiveEffect(World, Probe->ProjectileSkill.Get(), false);
				Probe->bImpactEffect |= HasLiveEffect(World, Probe->ProjectileSkill.Get(), true);
			}
		}, 0.02f, true);
		// Leave patrol running briefly so the actual state machine can enter Run.
		FTimerHandle Jump;
		World->GetTimerManager().SetTimer(Jump, [Probe, World]()
		{
			for (TActorIterator<ACC_PatrolEnemyCharacter> It(World); It; ++It) It->StopPatrol();
			if (ACC_PlayerCharacter* Player = Probe->Player.Get())
			{
				Player->Skills->CancelActiveSkills();
				Player->SetActorLocation(FVector(-3000.f, -3000.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
				Player->GetCharacterMovement()->StopMovementImmediately();
				Player->Jump();
			}
			if (ACC_EnemyCharacter* Enemy = Probe->Enemy.Get())
			{
				Enemy->Skills->CancelActiveSkills();
				Enemy->SetActorLocation(FVector(-1500.f, -3000.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
				Enemy->GetCharacterMovement()->StopMovementImmediately();
				Enemy->LaunchCharacter(FVector(0.f, 0.f, 500.f), true, true);
			}
		}, 2.f, false);
		FTimerHandle ReleaseJump;
		World->GetTimerManager().SetTimer(ReleaseJump, [Probe]() { if (Probe->Player.IsValid()) Probe->Player->StopJumping(); }, 2.12f, false);
		FTimerHandle JumpCapture;
		World->GetTimerManager().SetTimer(JumpCapture, []() { if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/AnimationJump.png"), false, false); }, 2.25f, false);
		FTimerHandle StartRun;
		World->GetTimerManager().SetTimer(StartRun, [Probe]()
		{
			Probe->bForceEnemyRun = true;
			if (Probe->Enemy.IsValid()) Probe->Enemy->GetCharacterMovement()->bRunPhysicsWithNoController = true;
		}, 3.35f, false);
		FTimerHandle StopRun;
		World->GetTimerManager().SetTimer(StopRun, [Probe]()
		{
			Probe->bForceEnemyRun = false;
			if (Probe->Enemy.IsValid()) Probe->Enemy->GetCharacterMovement()->StopMovementImmediately();
		}, 4.2f, false);
		FTimerHandle Attack;
		World->GetTimerManager().SetTimer(Attack, [Probe]()
		{
			if (ACC_PlayerCharacter* Player = Probe->Player.Get())
			{
				Player->SetActorRotation(FRotator(0.f, 90.f, 0.f));
				if (AController* Controller = Player->GetController()) Controller->SetControlRotation(FRotator(0.f, 90.f, 0.f));
				Probe->bPlayerAttackAccepted = Player->Skills->ActivateBasicAttack();
			}
			if (ACC_EnemyCharacter* Enemy = Probe->Enemy.Get())
			{
				Enemy->SetActorRotation(FRotator(0.f, 90.f, 0.f));
				if (AController* Controller = Enemy->GetController()) Controller->SetControlRotation(FRotator(0.f, 90.f, 0.f));
				Probe->bEnemyAttackAccepted = Enemy->Skills->ActivateSlot(0);
			}
		}, 4.5f, false);
		FTimerHandle AttackCapture;
		World->GetTimerManager().SetTimer(AttackCapture, []() { if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/AnimationAttack.png"), false, false); }, 4.72f, false);
		FTimerHandle Projectile;
		World->GetTimerManager().SetTimer(Projectile, [Probe, World]()
		{
			ACC_PlayerCharacter* Player = Probe->Player.Get();
			if (!Player) return;
			UCC_SkillDefinition* ConfiguredProjectile = LoadObject<UCC_SkillDefinition>(nullptr, TEXT("/Game/HPWH/Skills/DA_Skill_01.DA_Skill_01"));
			if (!ConfiguredProjectile || (!ConfiguredProjectile->ProjectileNiagara&&!ConfiguredProjectile->ProjectileCoreMaterial)) return;
			Probe->ProjectileSkill.Reset(DuplicateObject<UCC_SkillDefinition>(ConfiguredProjectile, GetTransientPackage()));
			UCC_SkillDefinition* Skill = Probe->ProjectileSkill.Get();
			Skill->Behavior = ECC_SkillBehavior::Projectile; Skill->Power = 0.f; Skill->PowerPerLevel = 0.f; Skill->Range = 2000.f;
			const FTransform Spawn(FRotator(0.f, 90.f, 0.f), Player->GetActorLocation() + FVector(0.f, 100.f, 150.f));
			ACC_SkillProjectile* Shot = World->SpawnActorDeferred<ACC_SkillProjectile>(ACC_SkillProjectile::StaticClass(), Spawn, Player, Player, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Shot)
			{
				Shot->Initialize(Skill, 0.f, nullptr);
				// The visual probe cannot hit or reward any actor; the player's real ability was tested above.
				Shot->Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Shot->FinishSpawning(Spawn); Probe->Projectile = Shot;
			}
			Player->Locomotion->PresentSkill(Skill, Spawn.GetLocation());
			Player->Locomotion->PresentImpact(Skill, Player->GetActorLocation() + FVector(0.f, 160.f, 50.f));
		}, 7.f, false);
		FTimerHandle EffectCapture;
		World->GetTimerManager().SetTimer(EffectCapture, []() { if (FApp::CanEverRender()) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/AnimationEffect.png"), false, false); }, 7.2f, false);
		FTimerHandle BeginCancelProbe;
		World->GetTimerManager().SetTimer(BeginCancelProbe, [Probe, World]()
		{
			ACC_PlayerCharacter* Player = Probe->Player.Get();
			ACC_EnemyCharacter* Template = Probe->Enemy.Get();
			if (!Player || !Template || !Player->Skills->BasicAttack) return;
			const FTransform Spawn(FRotator::ZeroRotator, Player->GetActorLocation() + FVector(180.f, 0.f, 0.f));
			ACC_EnemyCharacter* Target = World->SpawnActorDeferred<ACC_EnemyCharacter>(Template->GetClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Target) return;
			Target->AutoPossessAI = EAutoPossessAI::Disabled;
			Target->ExperienceReward = 0;
			Target->Skills->DefaultLoadout = nullptr;
			Target->Skills->BasicAttack = nullptr;
			if (ACC_PatrolEnemyCharacter* Patrol = Cast<ACC_PatrolEnemyCharacter>(Target)) { Patrol->bStartPatrolling = false; Patrol->bDetectPlayers = false; }
			Target->FinishSpawning(Spawn);
			Probe->CancelTarget = Target;
			Probe->CancelTargetHealth = Target->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());
			Probe->OriginalLoadout.Reset(Player->Skills->CurrentLoadout);
			Probe->CancelSkill.Reset(DuplicateObject<UCC_SkillDefinition>(Player->Skills->BasicAttack, GetTransientPackage()));
			UCC_SkillDefinition* Skill = Probe->CancelSkill.Get();
			Skill->SkillId = TEXT("Smoke_WindupOnly");
			Skill->Behavior = ECC_SkillBehavior::Melee; Skill->PowerMode = ECC_SkillPowerMode::Flat; Skill->Power = 25.f; Skill->PowerPerLevel = 0.f;
			Skill->Windup = 0.6f; Skill->ManaCost = 0.f; Skill->Cooldown = 0.1f; Skill->Range = 300.f; Skill->Angle = 180.f;
			Probe->CancelLoadout.Reset(NewObject<UCC_SkillLoadout>(GetTransientPackage()));
			if (!Probe->OriginalLoadout.Get()) return;
			Probe->CancelLoadout->Slots = Probe->OriginalLoadout->Slots;
			Probe->CancelLoadout->Slots[0].Skill = Skill; Probe->CancelLoadout->Slots[0].Level = 1;
			Player->SetActorRotation(FRotator::ZeroRotator);
			if (AController* Controller = Player->GetController()) Controller->SetControlRotation(FRotator::ZeroRotator);
			if (Player->Skills->ApplyLoadout(Probe->CancelLoadout.Get())) Probe->bCancelAttackAccepted = Player->Skills->ActivateSlot(0, Target);
			if (UAnimInstance* Anim = Player->GetMesh()->GetAnimInstance()) Probe->bCancelMontageStarted = Anim->IsAnyMontagePlaying();
		}, 8.f, false);
		FTimerHandle CancelWindup;
		World->GetTimerManager().SetTimer(CancelWindup, [Probe]() { if (Probe->Player.IsValid()) Probe->Player->Skills->CancelActiveSkills(); }, 8.15f, false);
		FTimerHandle VerifyCancel;
		World->GetTimerManager().SetTimer(VerifyCancel, [Probe]()
		{
			if (ACC_EnemyCharacter* Target = Probe->CancelTarget.Get())
			{
				const float Health = Target->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute());
				Probe->bCancelPreservedHealth = Probe->CancelTargetHealth > 0.f && FMath::IsNearlyEqual(Probe->CancelTargetHealth, Health);
			}
			if (ACC_PlayerCharacter* Player = Probe->Player.Get())
			{
				if (UAnimInstance* Anim = Player->GetMesh()->GetAnimInstance()) Probe->bCancelStoppedMontage = !Anim->IsAnyMontagePlaying();
			}
		}, 8.9f, false);
		FTimerHandle PositiveWindup;
		World->GetTimerManager().SetTimer(PositiveWindup, [Probe]()
		{
			if (Probe->Player.IsValid() && Probe->CancelTarget.IsValid()) Probe->bPositiveAttackAccepted = Probe->Player->Skills->ActivateSlot(0, Probe->CancelTarget.Get());
		}, 9.1f, false);
		FTimerHandle BeforeWindup;
		World->GetTimerManager().SetTimer(BeforeWindup, [Probe]()
		{
			if (ACC_EnemyCharacter* Target = Probe->CancelTarget.Get()) Probe->bPositiveWaitedForWindup = FMath::IsNearlyEqual(Probe->CancelTargetHealth, Target->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()));
		}, 9.35f, false);
		FTimerHandle AfterWindup;
		World->GetTimerManager().SetTimer(AfterWindup, [Probe]()
		{
			if (ACC_EnemyCharacter* Target = Probe->CancelTarget.Get())
			{
				Probe->bPositiveDealtDamage = Target->GetAbilitySystemComponent()->GetNumericAttribute(UCC_AttributeSet::GetHealthAttribute()) < Probe->CancelTargetHealth;
				Target->Destroy();
			}
			if (Probe->Player.IsValid() && Probe->OriginalLoadout.Get()) Probe->Player->Skills->ApplyLoadout(Probe->OriginalLoadout.Get());
		}, 10.f, false);
		FTimerHandle Finish;
		World->GetTimerManager().SetTimer(Finish, [Probe, World]() { FinishAnimationSmoke(Probe, World); }, 11.f, false);
	}, 3.f, false);
}
FAutoConsoleCommandWithWorld AnimationSmokeCommand(TEXT("CC.AnimationSmokeTest"), TEXT("Dedicated flat-arena test: actual locomotion graph states, attack montages, projectile and VFX; write Saved/AnimationSmoke.json, then exit. Does not purchase, kill or save progression."), FConsoleCommandWithWorldDelegate::CreateStatic(&RunAnimationSmoke));
}
#endif
