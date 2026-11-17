// Copyright Druid Mechanics


#include "CrashCourse/Public/Characters/CC_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/CC_AttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/CC_PlayerState.h"
#include "Progression/CC_ProgressionComponent.h"
#include "World/CC_AdventureGameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Characters/CC_SprintEffect.h"
#include "Progression/CC_SkillComponent.h"
#include "Player/CC_PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"


ACC_PlayerCharacter::ACC_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetOffset = FVector(0.f, 0.f, 65.f);
	CameraBoom->SocketOffset = FVector(0.f, 45.f, 15.f);
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->ProbeSize = 16.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetAutoActivate(false);

	FirstPersonCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->SetRelativeLocation(FirstPersonEyeOffset);
	FirstPersonCamera->bUsePawnControlRotation=true;
	FirstPersonCamera->SetAutoActivate(false);

	TopDownBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("TopDownBoom"));
	TopDownBoom->SetupAttachment(GetRootComponent());
	TopDownBoom->TargetArmLength=TopDownArmLength;
	TopDownBoom->bUsePawnControlRotation=false;
	TopDownBoom->bInheritPitch=false;
	TopDownBoom->bInheritYaw=false;
	TopDownBoom->bInheritRoll=false;
	TopDownBoom->bDoCollisionTest=true;
	TopDownBoom->ProbeChannel=ECC_Camera;
	TopDownBoom->ProbeSize=24.f;
	TopDownBoom->SetRelativeRotation(FRotator(TopDownPitch,0.f,0.f));

	TopDownCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(TopDownBoom,USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation=false;
	TopDownCamera->SetAutoActivate(true);
	ApplyCameraMode();
	Tags.Add(CrashTags::Player);
 ChromaticBodyMaterial=FSoftObjectPath(TEXT("/Game/HPWH/Effects/ChromaticInk/MI_InkMatCap_Burning.MI_InkMatCap_Burning"));
 ChromaticOutlineMaterial=FSoftObjectPath(TEXT("/Game/HPWH/Effects/ChromaticInk/MI_InkOutline_Burning.MI_InkOutline_Burning"));
}

void ACC_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Repair inherited tutorial blueprint overrides as well as native defaults.
	CameraBoom->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	CameraBoom->SetRelativeLocation(FVector::ZeroVector);
	CameraBoom->TargetArmLength = FMath::Max(200.f, CameraBoom->TargetArmLength);
	CameraBoom->TargetOffset = FVector(0.f, 0.f, 65.f);
	CameraBoom->SocketOffset = FVector(0.f, 45.f, 15.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->ProbeSize = 16.f;
	CameraBoom->bEnableCameraLag = false;
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	FollowCamera->bUsePawnControlRotation = false;
	FirstPersonCamera->SetRelativeLocation(FirstPersonEyeOffset);
	TopDownBoom->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	TopDownBoom->SetRelativeLocation(FVector::ZeroVector);
	TopDownBoom->TargetArmLength=FMath::Max(300.f,TopDownArmLength);
	TopDownBoom->bUsePawnControlRotation=false;
	TopDownBoom->bInheritPitch=false;
	TopDownBoom->bInheritYaw=false;
	TopDownBoom->bInheritRoll=false;
	TopDownBoom->bDoCollisionTest=true;
	TopDownBoom->ProbeChannel=ECC_Camera;
	TopDownBoom->ProbeSize=24.f;
	TopDownCamera->AttachToComponent(TopDownBoom,FAttachmentTransformRules::KeepRelativeTransform,USpringArmComponent::SocketName);
	ApplyCameraMode();
 RefreshChromaticAppearance();
}

void ACC_PlayerCharacter::ClearChromaticAppearance()
{
 if (!bChromaticMaterialApplied) return;
 GetMesh()->OverrideMaterials=PreChromaticOverrides;
 GetMesh()->SetOverlayMaterial(PreChromaticOverlay);
 GetMesh()->MarkRenderStateDirty();
 PreChromaticOverrides.Empty();PreChromaticOverlay=nullptr;bChromaticMaterialApplied=false;
}
void ACC_PlayerCharacter::RefreshChromaticAppearance()
{
 const auto* PS=GetPlayerState<ACC_PlayerState>();
 const bool bEnabled=PS && PS->Progression && PS->Progression->IsChromaticAppearanceEnabled();
 if (!bEnabled) {ClearChromaticAppearance();return;}
 if (bChromaticMaterialApplied || !GetMesh()->GetSkeletalMeshAsset()) return;
 UMaterialInterface* Outline=ChromaticOutlineMaterial.LoadSynchronous();
 if (!Outline) return;
 PreChromaticOverrides=GetMesh()->OverrideMaterials;PreChromaticOverlay=GetMesh()->GetOverlayMaterial();
 GetMesh()->SetOverlayMaterial(Outline);
 bChromaticMaterialApplied=true;
}

void ACC_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && SprintState.bActive)
	{
		const auto* PC=Cast<ACC_PlayerController>(GetController());
		if (!IsAlive() || !PC || PC->IsGameplayInputBlocked() || Skills->IsStunned() || Skills->IsRooted()
			|| GetCharacterMovement()->MovementMode==MOVE_None || GetSprintClock()>=SprintState.ActiveUntil
			|| !SprintASC.IsValid() || !SprintASC->GetActiveGameplayEffect(SprintEffect)) FinishSprint();
	}
	CheckDamageCamera();
	if (CameraMode==ECC_CameraMode::TopDown && IsLocallyControlled()) UpdateTopDownRotation();
	if (!IsLocallyControlled() || !FollowCamera) return;
	const bool bClose = CameraMode==ECC_CameraMode::FirstPerson || FVector::DistSquared(FollowCamera->GetComponentLocation(), GetMesh()->Bounds.Origin) < FMath::Square(100.f);
	if (bCameraHidesOwner != bClose)
	{
		bCameraHidesOwner = bClose;
		GetMesh()->SetOwnerNoSee(bClose);
	}
}

UAbilitySystemComponent* ACC_PlayerCharacter::GetAbilitySystemComponent() const
{
	ACC_PlayerState* CCPlayerState = Cast<ACC_PlayerState>(GetPlayerState());
	if (!IsValid(CCPlayerState)) return nullptr;

	return CCPlayerState->GetAbilitySystemComponent();
}

UAttributeSet* ACC_PlayerCharacter::GetAttributeSet() const
{
	ACC_PlayerState* CCPlayerState = Cast<ACC_PlayerState>(GetPlayerState());
	if (!IsValid(CCPlayerState)) return nullptr;

	return CCPlayerState->GetAttributeSet();
}

void ACC_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	if (!bStartupGrantedOnThisAvatar) { GiveStartupAbilities(); bStartupGrantedOnThisAvatar = true; }
	InitializeAttributes();
	GetPlayerState<ACC_PlayerState>()->Progression->InitializeAvatar();

	UCC_AttributeSet* CC_AttributeSet = Cast<UCC_AttributeSet>(GetAttributeSet());
	if (!IsValid(CC_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CC_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void ACC_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	GetPlayerState<ACC_PlayerState>()->Progression->InitializeAvatar();

	UCC_AttributeSet* CC_AttributeSet = Cast<UCC_AttributeSet>(GetAttributeSet());
	if (!IsValid(CC_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CC_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void ACC_PlayerCharacter::HandleDeath()
{
	const bool WasAlive=IsAlive();
	Skills->CancelPendingInput();
 if(auto* PS=GetPlayerState<ACC_PlayerState>();PS && PS->Progression)PS->Progression->ClearConsumableEffects();
	if (HasAuthority()) {bSprintHeld=false;FinishSprint();}
	Super::HandleDeath();
	if (!WasAlive || !HasAuthority() || GetWorld()->GetNetMode()!=NM_Standalone) return;
	GetWorldTimerManager().SetTimer(RestReturnTimer,FTimerDelegate::CreateWeakLambda(this,[this](){
		if (!IsAlive()) if (auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>()) Adventure->RespawnAtRest(Cast<APlayerController>(GetController()));
	}),FMath::Max(.1f,RestRespawnDelay),false);
}
void ACC_PlayerCharacter::HandleRespawn()
{
	ResetSprintState();
	Super::HandleRespawn();
	GetWorldTimerManager().ClearTimer(RestReturnTimer);
	GetWorldTimerManager().ClearTimer(DamageFirstPersonTimer);
	LastObservedHealth=-1.f;
	if (CameraMode!=ECC_CameraMode::TopDown) {CameraMode=ECC_CameraMode::TopDown;ApplyCameraMode();if (HasAuthority()) ForceNetUpdate();}
	if (HasAuthority())
		if (ACC_PlayerState* PS = GetPlayerState<ACC_PlayerState>()) PS->Progression->InitializeAvatar();
}

void ACC_PlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ThisClass,SprintState);
	DOREPLIFETIME(ThisClass,CameraMode);
}
float ACC_PlayerCharacter::GetSprintClock() const
{
	if (!GetWorld()) return 0.f;
	const auto* State=GetWorld()->GetGameState();
	return !HasAuthority() && State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}
float ACC_PlayerCharacter::GetSprintTimeRemaining() const
{
	return SprintState.bActive ? FMath::Max(0.f,SprintState.ActiveUntil-GetSprintClock()) : 0.f;
}
float ACC_PlayerCharacter::GetSprintCooldownRemaining() const
{
	return FMath::Max(0.f,SprintState.CooldownUntil-GetSprintClock());
}
bool ACC_PlayerCharacter::CanSprint() const
{
	const auto* PC=Cast<ACC_PlayerController>(GetController());
	const auto Mode=GetCharacterMovement()->MovementMode;
	return IsAlive() && PC && !PC->IsGameplayInputBlocked() && !SprintState.bActive && GetSprintCooldownRemaining()<=0.f
		&& Skills && !Skills->IsStunned() && !Skills->IsRooted() && GetAbilitySystemComponent()
		&& (Mode==MOVE_Walking || Mode==MOVE_NavWalking || Mode==MOVE_Falling);
}
void ACC_PlayerCharacter::SetSprintHeld(bool bHeld)
{
	if (HasAuthority()) HandleSprintInput(bHeld);
	else if (IsLocallyControlled()) ServerSetSprintHeld(bHeld);
}
void ACC_PlayerCharacter::ServerSetSprintHeld_Implementation(bool bHeld) {HandleSprintInput(bHeld);}
void ACC_PlayerCharacter::HandleSprintInput(bool bHeld)
{
	if (!HasAuthority() || bSprintHeld==bHeld) return;
	bSprintHeld=bHeld;
	if (!bHeld) {FinishSprint();return;}
	if (!CanSprint()) return;
	auto* ASC=GetAbilitySystemComponent();
	auto Context=ASC->MakeEffectContext();Context.AddSourceObject(this);
	auto Spec=ASC->MakeOutgoingSpec(UCC_SprintEffect::StaticClass(),1.f,Context);if (!Spec.IsValid()) return;
	Spec.Data->SetSetByCallerMagnitude(UCC_SprintEffect::MultiplierName,FMath::Max(1.f,SprintSpeedMultiplier));
	SprintEffect=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);if (!SprintEffect.IsValid()) return;
	SprintASC=ASC;SprintState.bActive=true;SprintState.ActiveUntil=GetSprintClock()+FMath::Max(.01f,SprintMaxDuration);SprintState.CooldownUntil=0.f;
	OnRep_SprintState();ForceNetUpdate();
}
void ACC_PlayerCharacter::RemoveSprintEffect()
{
	if (SprintASC.IsValid() && SprintEffect.IsValid()) SprintASC->RemoveActiveGameplayEffect(SprintEffect);
	SprintEffect.Invalidate();SprintASC.Reset();
}
void ACC_PlayerCharacter::FinishSprint()
{
	if (!HasAuthority() || !SprintState.bActive) return;
	SprintState.bActive=false;SprintState.ActiveUntil=0.f;SprintState.CooldownUntil=GetSprintClock()+FMath::Max(0.f,SprintCooldown);
	RemoveSprintEffect();OnRep_SprintState();ForceNetUpdate();
}
void ACC_PlayerCharacter::OnRep_SprintState() {OnSprintStateChanged.Broadcast(SprintState.bActive);}
void ACC_PlayerCharacter::ResetSprintState()
{
	if (!HasAuthority()) return;
	bSprintHeld=false;DodgeReadyAt=0.f;SprintState=FCC_SprintState();RemoveSprintEffect();OnRep_SprintState();ForceNetUpdate();
}
void ACC_PlayerCharacter::UnPossessed()
{
	if (HasAuthority()) {bSprintHeld=false;FinishSprint();}
	Super::UnPossessed();
}
void ACC_PlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(DamageFirstPersonTimer);
	if (HasAuthority()) RemoveSprintEffect();
	Super::EndPlay(Reason);
}




void ACC_PlayerCharacter::SetCameraMode(ECC_CameraMode NewMode)
{
	// The demo locks the camera to top-down; first person is reserved for taking damage.
	if (NewMode==ECC_CameraMode::ThirdPerson) NewMode=ECC_CameraMode::TopDown;
	if (!IsAlive()) return;
	if (CameraMode==NewMode) return;
	CameraMode=NewMode;
	ApplyCameraMode();
	if (!HasAuthority()) ServerSetCameraMode(NewMode);else ForceNetUpdate();
}
void ACC_PlayerCharacter::SetFirstPerson(bool bEnabled)
{
	SetCameraMode(bEnabled ? ECC_CameraMode::FirstPerson : ECC_CameraMode::TopDown);
}
void ACC_PlayerCharacter::CheckDamageCamera()
{
	if (!IsLocallyControlled()) return;
	const auto* Attributes=Cast<UCC_AttributeSet>(GetAttributeSet());
	if (!Attributes) return;
	const float CurrentHealth=Attributes->GetHealth();
	if (LastObservedHealth>=0.f && CurrentHealth<LastObservedHealth-0.01f && IsAlive())
	{
		SetCameraMode(ECC_CameraMode::FirstPerson);
		if (UWorld* World=GetWorld())
		{
			World->GetTimerManager().ClearTimer(DamageFirstPersonTimer);
			World->GetTimerManager().SetTimer(DamageFirstPersonTimer,this,&ThisClass::ReturnToTopDown,FMath::Max(0.2f,DamageFirstPersonDuration),false);
		}
	}
	LastObservedHealth=CurrentHealth;
}
void ACC_PlayerCharacter::ReturnToTopDown()
{
	if (IsAlive()) SetCameraMode(ECC_CameraMode::TopDown);
}
void ACC_PlayerCharacter::ServerSetCameraMode_Implementation(ECC_CameraMode NewMode) {SetCameraMode(NewMode);}
void ACC_PlayerCharacter::OnRep_CameraMode() {ApplyCameraMode();}
void ACC_PlayerCharacter::ApplyCameraMode()
{
	const bool bFirst=CameraMode==ECC_CameraMode::FirstPerson;
	const bool bTop=CameraMode==ECC_CameraMode::TopDown;
	if (FollowCamera) FollowCamera->SetActive(!bFirst && !bTop);
	if (FirstPersonCamera) FirstPersonCamera->SetActive(bFirst);
	if (TopDownCamera) TopDownCamera->SetActive(bTop);
	bUseControllerRotationYaw=bFirst;
	GetCharacterMovement()->bOrientRotationToMovement=!bFirst;
	bCameraHidesOwner=bFirst;GetMesh()->SetOwnerNoSee(bFirst);
	if (bTop)
	{
		TopDownBoom->TargetArmLength=FMath::Max(300.f,TopDownArmLength);
		TopDownBoom->TargetOffset=FVector(0.f,0.f,TopDownTargetHeight);
		if (AController* OwnerController=GetController())
		{
			FRotator ViewRotation=OwnerController->GetControlRotation();
			if (!FMath::IsNearlyZero(ViewRotation.Pitch))
			{
				ViewRotation.Pitch=0.f;
				OwnerController->SetControlRotation(ViewRotation);
			}
		}
		UpdateTopDownRotation();
	}
}
void ACC_PlayerCharacter::UpdateTopDownRotation()
{
	if (!TopDownBoom) return;
	if (AController* OwnerController=GetController())
	{
		// Keep the top-down view level so the damage camera never inherits stale pitch.
		FRotator ControlRotation=OwnerController->GetControlRotation();
		if (!FMath::IsNearlyZero(ControlRotation.Pitch))
		{
			ControlRotation.Pitch=0.f;
			OwnerController->SetControlRotation(ControlRotation);
		}
	}
	const float Yaw=Controller ? Controller->GetControlRotation().Yaw : GetActorRotation().Yaw;
	TopDownBoom->SetWorldRotation(FRotator(TopDownPitch,Yaw,0.f));
}
