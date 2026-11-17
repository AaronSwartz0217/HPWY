// Copyright Druid Mechanics


#include "CrashCourse/Public/Player/CC_PlayerController.h"
#include "World/CC_LevitationPlatform.h"
#include "World/CC_TownShop.h"
#include "World/CC_LoreStand.h"
#include "UI/CC_LoreReader.h"
#include "World/CC_InteractiveDoubleDoor.h"
#include "EngineUtils.h"
#include "World/CC_HeavenSceneRebuild.h"
#include "WFCMapGenerator.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Player/CC_ControlSettingsSave.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/CC_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameplayTags/CCTags.h"
#include "Progression/CC_SkillComponent.h"
#include "Progression/CC_ProgressionComponent.h"
#include "Progression/CC_ArtifactComponent.h"
#include "Progression/CC_ExperiencePack.h"
#include "Characters/CC_PlayerCharacter.h"
#include "Player/CC_PlayerState.h"
#include "UI/CC_ProgressionMenu.h"
#include "UI/CC_PlayerHealthHUDWidget.h"
#include "Progression/CC_ProgressionHUD.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "World/CC_AdventureGameInstance.h"
#include "World/CC_MinimapCapture.h"
#include "UI/CC_WorldNavigationWidget.h"
#include "UI/CC_OptionsWidget.h"
#include "Interaction/CC_PlayerScanComponent.h"

ACC_PlayerController::ACC_PlayerController(){Scanner=CreateDefaultSubobject<UCC_PlayerScanComponent>(TEXT("FirstPersonScanner"));}
void ACC_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController()) UCC_OptionsWidget::ApplySavedUpscaler();
	const auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>();
	if (!IsLocalController() || !Adventure || Adventure->RegionForMap(UGameplayStatics::GetCurrentLevelName(this,true)).IsNone()) return;
	UClass* CameraClass=MinimapClass ? MinimapClass.Get() : ACC_MinimapCapture::StaticClass();
	MinimapCamera=GetWorld()->SpawnActor<ACC_MinimapCapture>(CameraClass);
	if (MinimapCamera) MinimapCamera->FollowingController=this;
	UClass* NavigationClass=WorldNavigationClass ? WorldNavigationClass.Get() : UCC_WorldNavigationWidget::StaticClass();
	WorldNavigation=CreateWidget<UCC_WorldNavigationWidget>(this,NavigationClass);
	if (WorldNavigation) {WorldNavigation->AddToViewport(90);WorldNavigation->InitializeNavigation(MinimapCamera);}
}
void ACC_PlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	StopSprintInput();
	CloseLoreReader();
	ReleaseRuntimeInputContexts();
	SetTravelMenuOpen(false);
	if (WorldNavigation) {WorldNavigation->RemoveFromParent();WorldNavigation=nullptr;}
	if (MinimapCamera) {MinimapCamera->Destroy();MinimapCamera=nullptr;}
	Super::EndPlay(Reason);
}
bool ACC_PlayerController::IsGameplayInputBlocked() const
{
	const auto* Adventure=GetWorld() ? GetWorld()->GetGameInstance<UCC_AdventureGameInstance>() : nullptr;
	return LoreReader || IsProgressionMenuOpen() || bTravelInputLocked || (Adventure && Adventure->bTravelling);
}
void ACC_PlayerController::ToggleMinimap() {if (WorldNavigation) WorldNavigation->ToggleMinimap();}
void ACC_PlayerController::ToggleChromaticTest()
{
 if (!IsAlive() || IsGameplayInputBlocked()) return;
 if (const auto* PS=GetPlayerState<ACC_PlayerState>(); PS && PS->Progression) PS->Progression->ToggleChromaticAppearance();
}
void ACC_PlayerController::UseItemSlot()
{
 if(!IsAlive() || IsGameplayInputBlocked())return;
 if(auto* PS=GetPlayerState<ACC_PlayerState>();PS && PS->Progression)PS->Progression->UseEquippedItem();
}
void ACC_PlayerController::ToggleTravelMenu() {SetTravelMenuOpen(!bTravelInputLocked);}
void ACC_PlayerController::SetTravelMenuOpen(bool bOpen)
{
	if (!WorldNavigation || !IsLocalController()) return;
	const auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>();
	if (bOpen && (!IsAlive() || LoreReader || IsProgressionMenuOpen() || !Adventure || Adventure->bTravelling)) return;
	WorldNavigation->SetTravelOpen(bOpen);
	if (bOpen) {StopSprintInput();if(auto* C=Cast<ACC_BaseCharacter>(GetPawn()))C->Skills->CancelPendingInput();}
	if (bTravelInputLocked!=bOpen) {SetIgnoreMoveInput(bOpen);SetIgnoreLookInput(bOpen);bTravelInputLocked=bOpen;}
	bShowMouseCursor=bOpen;
	if (bOpen) {if (GetCharacter()) GetCharacter()->StopJumping();FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(WorldNavigation->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);}
	else if (!IsProgressionMenuOpen()) SetInputMode(FInputModeGameOnly());
}

void ACC_PlayerController::ReleaseRuntimeInputContexts()
{
 if (auto* Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
  for (UInputMappingContext* Context:RuntimeContexts) if (Context) Subsystem->RemoveMappingContext(Context);
 RuntimeContexts.Empty();
}
void ACC_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
 InputComponent->BindKey(EKeys::Q,IE_Pressed,this,&ThisClass::ToggleChromaticTest);
 InputComponent->BindKey(EKeys::L,IE_Pressed,this,&ThisClass::ServerToggleHeavenMaze);
 InputComponent->BindKey(EKeys::Five,IE_Pressed,this,&ThisClass::UseItemSlot);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ThisClass::ToggleCombatHUD);
 InputComponent->BindKey(EKeys::Tab,IE_Pressed,this,&ThisClass::ToggleProgressionMenu);
 InputComponent->BindKey(EKeys::MouseScrollDown,IE_Pressed,this,&ThisClass::NextHUDSkill);
 InputComponent->BindKey(EKeys::MouseScrollUp,IE_Pressed,this,&ThisClass::PreviousHUDSkill);
 InputComponent->BindKey(EKeys::Enter,IE_Pressed,this,&ThisClass::PressHUDSkill);
 InputComponent->BindKey(EKeys::Enter,IE_Released,this,&ThisClass::ReleaseHUDSkill);
	InputComponent->BindKey(EKeys::M,IE_Pressed,this,&ThisClass::ToggleMinimap);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ThisClass::PressPrimaryMouse);
 InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ThisClass::ReleasePrimaryMouse);
	InputComponent->BindKey(EKeys::LeftShift,IE_Pressed,this,&ThisClass::StartSprintInput);
	InputComponent->BindKey(EKeys::LeftShift,IE_Released,this,&ThisClass::ReleaseSprintInput);
	InputComponent->BindKey(EKeys::RightShift,IE_Pressed,this,&ThisClass::StartSprintInput);
	InputComponent->BindKey(EKeys::RightShift,IE_Released,this,&ThisClass::ReleaseSprintInput);
 CombatKeys = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::J, EKeys::E};
 ReservedInputKeys = {EKeys::Tab, EKeys::Escape, EKeys::M, EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::SpaceBar};
 ReservedInputKeys.Add(EKeys::LeftShift);ReservedInputKeys.Add(EKeys::RightShift);
 ReservedInputKeys.Add(EKeys::Q);
 ReservedInputKeys.Add(EKeys::Five);
 ReservedInputKeys.Add(EKeys::Enter);ReservedInputKeys.Add(EKeys::MouseScrollUp);ReservedInputKeys.Add(EKeys::MouseScrollDown);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 InputComponent->BindKey(EKeys::Zero,IE_Pressed,this,&ThisClass::DeveloperReset);
 InputComponent->BindKey(EKeys::Nine,IE_Pressed,this,&ThisClass::DeveloperUnlockAll);
 ReservedInputKeys.Add(EKeys::Zero); ReservedInputKeys.Add(EKeys::Nine);
#endif
 for (int32 I=1; I<=12; ++I) ReservedInputKeys.Add(FKey(*FString::Printf(TEXT("F%d"), I)));

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;
	// LocalPlayer and its input subsystem survive map travel; remove only the
	// runtime copies owned by this controller before installing replacements.
	ReleaseRuntimeInputContexts();

	for (UInputMappingContext* Context : InputMappingContexts)
	{
		if (!Context) continue;
  UInputMappingContext* Runtime = DuplicateObject<UInputMappingContext>(Context, this);
  const UInputAction* CombatActions[] = {PrimaryAction, SecondaryAction, TertiaryAction, FourthAction, BasicAttackAction, InteractAction};
  for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) {
   bool bCombat = false;
   for (const UInputAction* Action : CombatActions) if (Action && Mapping.Action == Action) bCombat = true;
   if (!bCombat) ReservedInputKeys.Add(Mapping.Key);
  }
  for (const UInputAction* Action : CombatActions) if (Action) Runtime->UnmapAllKeysFromAction(Action);
  RuntimeContexts.Add(Runtime);
  InputSubsystem->AddMappingContext(Runtime, 0);
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);


 if (UGameplayStatics::DoesSaveGameExist(TEXT("SpiritTalesControls"),0))
 if (const UCC_ControlSettingsSave* Save = Cast<UCC_ControlSettingsSave>(UGameplayStatics::LoadGameFromSlot(TEXT("SpiritTalesControls"),0)); Save && Save->CombatKeys.Num()==6) {
  TSet<FKey> Unique; bool bValid=true;
  for (FKey Key : Save->CombatKeys) { bValid &= IsAllowedCombatKey(Key) && !Unique.Contains(Key); Unique.Add(Key); }
  if (bValid) CombatKeys=Save->CombatKeys;
 }
 BindCombatKeys();

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	if (bShowProgressionPanel)
	{

		const FKey FormKeys[] = { EKeys::F3, EKeys::F4, EKeys::F5, EKeys::F6 };
		const FKey PurchaseKeys[] = { EKeys::F7, EKeys::F8, EKeys::F9, EKeys::F10, EKeys::F11, EKeys::F12 };
		auto BindIndexed = [this](FKey Key, void (ThisClass::*Function)(int32), int32 Index)
		{
			FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
			Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, Function, Index]() { (this->*Function)(Index); });
			InputComponent->KeyBindings.Add(MoveTemp(Binding));
		};
		for (int32 I = 0; I < 4; ++I) { BindIndexed(FormKeys[I], &ThisClass::DebugForm, I); }
		for (int32 I = 0; I < 6; ++I) BindIndexed(PurchaseKeys[I], &ThisClass::DebugPurchase, I);


		InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ThisClass::DebugEvolve);
		BindIndexed(EKeys::F2, &ThisClass::DebugForm, -1);
	}
#endif
}

void ACC_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (LoreReader && (!IsAlive() || !LoreReader->IsInViewport())) CloseLoreReader();
	// UI focus and input flushing can consume key-up. Never leave the old pawn sprinting.
	if (IsLocalController() && bSprintInputHeld && (IsGameplayInputBlocked() || !IsAlive() || SprintInputCharacter.Get()!=GetPawn()
		|| (!IsInputKeyDown(EKeys::LeftShift) && !IsInputKeyDown(EKeys::RightShift)))) StopSprintInput();
}
void ACC_PlayerController::StartSprintInput()
{
	if (bSprintInputHeld || IsGameplayInputBlocked() || !IsAlive()) return;
	if (auto* SprintAvatar=Cast<ACC_PlayerCharacter>(GetPawn()))
	{
		bSprintInputHeld=true;
		bSprintThresholdReached=false;
		SprintPressedAt=GetWorld()->GetTimeSeconds();
		SprintInputCharacter=SprintAvatar;
		GetWorldTimerManager().SetTimer(SprintHoldTimer,this,&ThisClass::BeginHeldSprint,FMath::Max(.05f,SprintHoldThreshold),false);
	}
}
void ACC_PlayerController::BeginHeldSprint()
{
 if(!bSprintInputHeld || IsGameplayInputBlocked() || !IsAlive() || SprintInputCharacter.Get()!=GetPawn())
 {
  StopSprintInput();
  return;
 }
 bSprintThresholdReached=true;
 if(auto* Avatar=SprintInputCharacter.Get())Avatar->SetSprintHeld(true);
}

void ACC_PlayerController::ReleaseSprintInput()
{
 if(IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))return;
 auto* Avatar=SprintInputCharacter.Get();
 const bool bTap=bSprintInputHeld && !bSprintThresholdReached && GetWorld()->GetTimeSeconds()-SprintPressedAt<FMath::Max(.05f,SprintHoldThreshold) && Avatar && Avatar==GetPawn() && !IsGameplayInputBlocked() && IsAlive();
 StopSprintInput();
 if(bTap)
 {
  FVector Direction=Avatar->GetPendingMovementInputVector().GetSafeNormal2D();
  if(Direction.IsNearlyZero())Direction=Avatar->GetLastMovementInputVector().GetSafeNormal2D();
  Avatar->RequestDodge(Direction);
 }
}

void ACC_PlayerController::StopSprintInput()
{
 GetWorldTimerManager().ClearTimer(SprintHoldTimer);
 if(bSprintThresholdReached)
  if(auto* Avatar=SprintInputCharacter.Get())Avatar->SetSprintHeld(false);
 bSprintInputHeld=false;
 bSprintThresholdReached=false;
 SprintInputCharacter.Reset();
}

void ACC_PlayerController::Jump()
{
	if (IsGameplayInputBlocked()) return;
	if (!IsValid(GetCharacter())) return;
	if (!IsAlive()) return;

	GetCharacter()->Jump();
}

void ACC_PlayerController::StopJumping()
{
	if (!IsValid(GetCharacter())) return;
	if (!IsAlive()) return;

	GetCharacter()->StopJumping();
}

void ACC_PlayerController::Move(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (!IsValid(GetPawn())) return;
	if (!IsAlive()) return;

	if(auto* C=Cast<ACC_BaseCharacter>(GetPawn()); C && C->Skills->IsRooted()) return;
 const FVector2D MovementVector = Value.Get<FVector2D>();

	// Find which way is forward
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void ACC_PlayerController::Look(const FInputActionValue& Value)
{
	if (IsGameplayInputBlocked()) return;
	if (!IsAlive()) return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void ACC_PlayerController::Primary()
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(GetPawn()); C && C->Skills->CurrentLoadout) { C->Skills->ActivateSlot(0); return; }
	ActivateAbility(CCTags::CCAbilities::Primary);
}

void ACC_PlayerController::Secondary()
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(GetPawn()); C && C->Skills->CurrentLoadout) { C->Skills->ActivateSlot(1); return; }
	ActivateAbility(CCTags::CCAbilities::Secondary);
}

void ACC_PlayerController::Tertiary()
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(GetPawn()); C && C->Skills->CurrentLoadout) { C->Skills->ActivateSlot(2); return; }
	ActivateAbility(CCTags::CCAbilities::Tertiary);
}

void ACC_PlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{
	if (!IsAlive()) return;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return;

	ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

bool ACC_PlayerController::IsAlive() const
{
	ACC_BaseCharacter* BaseCharacter = Cast<ACC_BaseCharacter>(GetPawn());
	if (!IsValid(BaseCharacter)) return false;
	return BaseCharacter->IsAlive();
}

void ACC_PlayerController::Fourth()
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(GetPawn())) C->Skills->ActivateSlot(3);
}
void ACC_PlayerController::BasicAttack()
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(GetPawn())) C->Skills->ActivateBasicAttack();
}
void ACC_PlayerController::DebugSlot(int32 Slot) {
	if (IsGameplayInputBlocked()) return; if (ACC_BaseCharacter* C = Cast<ACC_BaseCharacter>(GetPawn())) C->Skills->ActivateSlot(Slot); }
void ACC_PlayerController::DebugEvolve() {
	if (IsGameplayInputBlocked()) return; if (ACC_PlayerState* PS = GetPlayerState<ACC_PlayerState>()) PS->Progression->Evolve(); }
void ACC_PlayerController::DebugForm(int32 Index)
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_PlayerState* PS = GetPlayerState<ACC_PlayerState>())
		if (Index == -1 || PS->Progression->ArtifactForms.IsValidIndex(Index)) PS->Progression->SwitchForm(Index == -1 ? nullptr : PS->Progression->ArtifactForms[Index].Get());
}
void ACC_PlayerController::DebugPurchase(int32 Index)
{
	if (IsGameplayInputBlocked()) return;
	if (ACC_PlayerState* PS = GetPlayerState<ACC_PlayerState>(); PS && PS->Progression->UpgradeCatalog.IsValidIndex(Index)) PS->Progression->Purchase(PS->Progression->UpgradeCatalog[Index]);
}
AActor* ACC_PlayerController::FindInteractionTarget() const
{
 if (IsGameplayInputBlocked() || !IsAlive()) return nullptr;
 auto* Interactor=Cast<ACC_PlayerCharacter>(GetPawn());
 if (!Interactor || !GetWorld()) return nullptr;
 ACC_LoreStand* NearbyBook = nullptr;
 float BookDistance = MAX_flt;
 for (TActorIterator<ACC_LoreStand> It(GetWorld()); It; ++It)
 {
  if (!It->CanRead(Interactor))
   continue;
  const float Distance = FVector::DistSquared(It->GetActorLocation(), Interactor->GetActorLocation());
  if (Distance < BookDistance)
  {
   NearbyBook = *It;
   BookDistance = Distance;
  }
 }
 if (NearbyBook) return NearbyBook;
 if(auto* Scene=GetHeavenSceneTarget())return Cast<AWFCMapGenerator>(Scene)->CanRegenerateRandomScene()?Scene:nullptr;
 auto CanUse=[Interactor](AActor* Actor) {
  if (!IsValid(Actor) || Actor->IsHidden()) return false;
  if(auto* Door=Cast<ACC_InteractiveDoubleDoor>(Actor))return Door->CanInteract(Interactor);
  if(auto* Platform=Cast<ACC_LevitationPlatform>(Actor))return Platform->CanInteract(Interactor);
  if (auto* Pack=Cast<ACC_ExperiencePack>(Actor)) return Pack->CanInteract(Interactor);
  auto* Artifact=Actor->FindComponentByClass<UCC_ArtifactComponent>();
  return Artifact && Artifact->CanInteract(Interactor);
 };
 FVector Eye; FRotator Rotation; GetPlayerViewPoint(Eye,Rotation);
 FCollisionQueryParams Params; Params.AddIgnoredActor(Interactor);
 TArray<AActor*> Attached;Interactor->GetAttachedActors(Attached,true,true);Params.AddIgnoredActors(Attached);
 FHitResult Hit;
 if(Interactor->IsFirstPerson()&&Scanner)Scanner->TraceCrosshair(Hit);
 else GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Rotation.Vector()*InteractionTraceDistance,ECC_Visibility,Params);
 if (CanUse(Hit.GetActor())) return Hit.GetActor();
 // Door meshes need not have query collision for the nearby interaction fallback.
 ACC_InteractiveDoubleDoor* NearbyDoor=nullptr;float DoorDistance=MAX_flt;
 for(TActorIterator<ACC_InteractiveDoubleDoor> It(GetWorld());It;++It) if(It->CanInteract(Interactor))
 {
  if(Interactor->IsFirstPerson() && FVector::DotProduct(Rotation.Vector(),(It->GetActorLocation()+FVector(0,0,120)-Eye).GetSafeNormal())<.2f)continue;
  const float Distance=FVector::DistSquared(It->GetActorLocation(),Interactor->GetActorLocation());if(Distance<DoorDistance){NearbyDoor=*It;DoorDistance=Distance;}
 }
 if(NearbyDoor)return NearbyDoor;
 if(Interactor->IsFirstPerson())return nullptr;
 // The camera can look above a small cube at close range. Search around the
 // character as well; CanInteract still enforces reach, state and line of sight.
 TArray<FOverlapResult> Nearby;
 FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
 GetWorld()->OverlapMultiByObjectType(Nearby,Interactor->GetActorLocation(),FQuat::Identity,Objects,FCollisionShape::MakeSphere(NearbyInteractionSearchRadius),Params);
 AActor* Best=nullptr; float BestScore=TNumericLimits<float>::Max();
 TSet<AActor*> Seen;
 for (const auto& Overlap:Nearby) {
  AActor* Actor=Overlap.GetActor(); if (Seen.Contains(Actor)) continue; Seen.Add(Actor);
  if (!CanUse(Actor)) continue;
  const FBox Bounds=Actor->GetComponentsBoundingBox(false);
  const FVector Point=Bounds.IsValid ? Bounds.GetClosestPointTo(Interactor->GetActorLocation()) : Actor->GetActorLocation();
  const float Facing=FVector::DotProduct(Rotation.Vector().GetSafeNormal2D(),(Point-Interactor->GetActorLocation()).GetSafeNormal2D());
  const float Score=FVector::Distance(Point,Interactor->GetActorLocation())+(1.f-Facing)*40.f;
  if (Score<BestScore) { Best=Actor; BestScore=Score; }
 }
 return Best;
}
void ACC_PlayerController::Interact()
{
 RequestInteraction(FindInteractionTarget());
}
void ACC_PlayerController::RequestInteraction(AActor* Target)
{
	if (IsGameplayInputBlocked() || !IsAlive() || !IsValid(Target)) return;
 if(auto* Book=Cast<ACC_LoreStand>(Target)){OpenLoreReader(Book);return;}
 if(auto* Door=Cast<ACC_InteractiveDoubleDoor>(Target)){Door->Interact(GetPawn());return;}
 if(Target==GetHeavenSceneTarget()){ServerRegenerateHeavenScene();return;}
	if(auto* Platform=Cast<ACC_LevitationPlatform>(Target)){ServerReassemblePlatform(Platform);return;}
	if (ACC_ExperiencePack* Pack = Cast<ACC_ExperiencePack>(Target))
	{
		ServerInteractExperiencePack(Pack);
		return;
	}
	if (ACC_PlayerState* PS = GetPlayerState<ACC_PlayerState>(); PS && Target->FindComponentByClass<UCC_ArtifactComponent>()) PS->Progression->Interact(Target);
}
void ACC_PlayerController::ServerReassemblePlatform_Implementation(ACC_LevitationPlatform* Platform)
{
	if(IsGameplayInputBlocked()||!IsAlive()||!IsValid(Platform))return;
	Platform->Reassemble(Cast<ACC_PlayerCharacter>(GetPawn()));
}
void ACC_PlayerController::ServerInteractExperiencePack_Implementation(ACC_ExperiencePack* Pack)
{
	if (IsValid(Pack)) Pack->Interact(Cast<ACC_PlayerCharacter>(GetPawn()));
}

bool ACC_PlayerController::IsProgressionMenuOpen() const { return ProgressionMenu && ProgressionMenu->IsInViewport(); }
void ACC_PlayerController::ToggleProgressionMenu()
{
 if (IsProgressionMenuOpen()) CloseProgressionMenu(); else OpenProgressionMenu();
}
void ACC_PlayerController::OpenProgressionMenu()
{
 if (LoreReader) return;
 if(auto* HUD=Cast<ACC_ProgressionHUD>(GetHUD());HUD&&HUD->PlayerHealthHUD)HUD->PlayerHealthHUD->SetExpanded(false);
	const auto* Adventure=GetWorld()->GetGameInstance<UCC_AdventureGameInstance>();
	if (Adventure && Adventure->bTravelling) return;
	SetTravelMenuOpen(false);
 if (!IsLocalController() || IsProgressionMenuOpen()) return;
 // A widget closed outside the controller still needs its previous input lock released.
 if (ProgressionMenu) CloseProgressionMenu();
 const TSubclassOf<UUserWidget> Class = ProgressionMenuClass ? ProgressionMenuClass.Get() : UCC_ProgressionMenu::StaticClass();
 ProgressionMenu = CreateWidget<UUserWidget>(this, Class);
 if (!ProgressionMenu) return;
 if (GetCharacter()) GetCharacter()->StopJumping();
 ProgressionMenu->SetIsFocusable(true);
 ProgressionMenu->AddToViewport(100);
 if (!IsProgressionMenuOpen()) return;
 if(auto* C=Cast<ACC_BaseCharacter>(GetPawn()))C->Skills->CancelPendingInput();
 StopSprintInput();
 SetIgnoreMoveInput(true); SetIgnoreLookInput(true); bMenuInputLocked = true;
 bShowMouseCursor = true;
 FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(ProgressionMenu->TakeWidget());
 Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); Mode.SetHideCursorDuringCapture(false);
 SetInputMode(Mode);
 OnMenuVisibilityChanged.Broadcast(true);
}
void ACC_PlayerController::CloseProgressionMenu()
{
 if (!IsLocalController() || !ProgressionMenu) return;
 UUserWidget* ClosingMenu = ProgressionMenu; ProgressionMenu = nullptr;
 ClosingMenu->RemoveFromParent();
 bShowMouseCursor = false; SetInputMode(FInputModeGameOnly());
 if (bMenuInputLocked) { SetIgnoreMoveInput(false); SetIgnoreLookInput(false); bMenuInputLocked = false; }
 OnMenuVisibilityChanged.Broadcast(false);
}

void ACC_PlayerController::OpenLoreReader(const ACC_LoreStand* Stand)
{
 if (!IsLocalController() || IsGameplayInputBlocked() || !IsValid(Stand) ||
     !Stand->CanRead(Cast<ACC_PlayerCharacter>(GetPawn())))
  return;

 LoreReader = CreateWidget<UCC_LoreReader>(this, UCC_LoreReader::StaticClass());
 if (!LoreReader) return;
 LoreReader->SetBook(Stand->Book);
 LoreReader->SetIsFocusable(true);
 LoreReader->AddToViewport(110);

 if (auto* ReadingCharacter = Cast<ACC_BaseCharacter>(GetPawn()))
 {
  ReadingCharacter->StopJumping();
  ReadingCharacter->Skills->CancelPendingInput();
 }
 StopSprintInput();
 SetIgnoreMoveInput(true);
 SetIgnoreLookInput(true);
 bShowMouseCursor = true;
 FInputModeUIOnly Mode;
 Mode.SetWidgetToFocus(LoreReader->TakeWidget());
 Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
 SetInputMode(Mode);
}

void ACC_PlayerController::CloseLoreReader()
{
 if (!LoreReader) return;
 UCC_LoreReader* Closing = LoreReader;
 LoreReader = nullptr;
 Closing->RemoveFromParent();
 SetIgnoreMoveInput(false);
 SetIgnoreLookInput(false);
 bShowMouseCursor = false;
 SetInputMode(FInputModeGameOnly());
}

FKey ACC_PlayerController::GetCombatKey(int32 Action) const
{
 return CombatKeys.IsValidIndex(Action) ? CombatKeys[Action] : FKey();
}
bool ACC_PlayerController::IsAllowedCombatKey(FKey Key) const
{
 return Key.IsValid() && !Key.IsGamepadKey() && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D() && !Key.IsModifierKey() && !ReservedInputKeys.Contains(Key);
}
bool ACC_PlayerController::SetCombatKey(int32 Action, FKey Key)
{
 if (!IsLocalController() || !CombatKeys.IsValidIndex(Action) || !IsAllowedCombatKey(Key)) return false;
 for (int32 I=0; I<CombatKeys.Num(); ++I) if (I!=Action && CombatKeys[I]==Key) return false;
 CombatKeys[Action]=Key; BindCombatKeys(); SaveCombatKeys(); OnCombatKeysChanged.Broadcast(); return true;
}
void ACC_PlayerController::ResetCombatKeys()
{
 CombatKeys = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::J, EKeys::E};
 BindCombatKeys(); SaveCombatKeys(); OnCombatKeysChanged.Broadcast();
}
void ACC_PlayerController::SaveCombatKeys()
{
 UCC_ControlSettingsSave* Save = NewObject<UCC_ControlSettingsSave>(); Save->CombatKeys=CombatKeys;
 UGameplayStatics::SaveGameToSlot(Save, TEXT("SpiritTalesControls"), 0);
}
void ACC_PlayerController::BindCombatKeys()
{
 if (!InputComponent) return;
 if(!BoundCombatKeys.IsEmpty())if(auto* C=Cast<ACC_BaseCharacter>(GetPawn()))C->Skills->CancelPendingInput();
 InputComponent->KeyBindings.RemoveAll([this](const FInputKeyBinding& B) { return (B.KeyEvent==IE_Pressed || B.KeyEvent==IE_Released) && BoundCombatKeys.Contains(B.Chord.Key); });
 BoundCombatKeys=CombatKeys;
 for (int32 I=0; I<CombatKeys.Num(); ++I) {
  FInputKeyBinding Binding(FInputChord(CombatKeys[I]), IE_Pressed);
  Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this,I]() {
   switch (I) { case 0: Primary(); break; case 1: Secondary(); break; case 2: Tertiary(); break; case 3: Fourth(); break; case 4: BasicAttack(); break; case 5: Interact(); break; }
  });
  InputComponent->KeyBindings.Add(MoveTemp(Binding));
  if(I<4) {
   FInputKeyBinding Release(FInputChord(CombatKeys[I]),IE_Released);
   Release.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this,[this,I](){ if(auto* C=Cast<ACC_BaseCharacter>(GetPawn())) C->Skills->ReleaseSlot(I); });
   InputComponent->KeyBindings.Add(MoveTemp(Release));
  }
 }
}

void ACC_PlayerController::DeveloperUnlockAll() {
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 if (auto* PS=GetPlayerState<ACC_PlayerState>()) PS->Progression->ApplyDeveloperSandbox();
#endif
}
void ACC_PlayerController::DeveloperReset() {
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
 if (auto* PS=GetPlayerState<ACC_PlayerState>()) PS->Progression->ApplyDeveloperPreset(false);
#endif
}


namespace {
UCC_PlayerHealthHUDWidget* CombatHUD(ACC_PlayerController* PC)
{
 auto* HUD=PC?Cast<ACC_ProgressionHUD>(PC->GetHUD()):nullptr;return HUD?HUD->PlayerHealthHUD.Get():nullptr;
}
}
void ACC_PlayerController::ToggleCombatHUD()
{
 if(!IsLocalController()||!IsAlive())return;
 if(bTravelInputLocked)SetTravelMenuOpen(false);
 if(IsGameplayInputBlocked())return;
 if(auto* HUD=CombatHUD(this))HUD->ToggleCommandMenu();
}
void ACC_PlayerController::NextHUDSkill(){if(!IsGameplayInputBlocked())if(auto* HUD=CombatHUD(this))HUD->CycleSkill(1);}
void ACC_PlayerController::PreviousHUDSkill(){if(!IsGameplayInputBlocked())if(auto* HUD=CombatHUD(this))HUD->CycleSkill(-1);}
void ACC_PlayerController::PressHUDSkill(){if(auto* HUD=CombatHUD(this))HUD->PressSelectedSkill(EKeys::Enter);}
void ACC_PlayerController::ReleaseHUDSkill(){if(auto* HUD=CombatHUD(this))HUD->ReleaseSelectedSkill(EKeys::Enter);}
void ACC_PlayerController::PressPrimaryMouse()
{
 if(IsGameplayInputBlocked()||!IsAlive())return;
 if(auto* HUD=CombatHUD(this);HUD&&HUD->IsExpanded()){HUD->PressSelectedSkill(EKeys::LeftMouseButton);return;}
 BasicAttack();
}
void ACC_PlayerController::ReleasePrimaryMouse(){if(auto* HUD=CombatHUD(this))HUD->ReleaseSelectedSkill(EKeys::LeftMouseButton);}

AActor* ACC_PlayerController::GetHeavenSceneTarget()const{return CCHeavenScene::IsHeaven(GetWorld())?CCHeavenScene::FindGenerator(GetWorld()):nullptr;}
void ACC_PlayerController::ServerToggleHeavenMaze_Implementation()
{
    if (IsHeavenCityUnlocked() && !IsGameplayInputBlocked() && IsAlive() && CCHeavenScene::IsHeaven(GetWorld()))
        CCHeavenScene::Rebuild(GetWorld(), true);
}

void ACC_PlayerController::CompleteHeavenMaze(int32 Revision)
{
    if (!HasAuthority() || Revision <= 0 || HasCompletedHeavenMaze(Revision))
        return;
    CompletedHeavenMazeRevision = Revision;
    ClientHeavenMazeCompleted(Revision);
}

void ACC_PlayerController::ClientHeavenMazeCompleted_Implementation(int32 Revision)
{
    CompletedHeavenMazeRevision = Revision;
}

void ACC_PlayerController::ServerRegenerateHeavenScene_Implementation(){if(IsGameplayInputBlocked()||!IsAlive()||!CCHeavenScene::IsHeaven(GetWorld()))return;CCHeavenScene::Rebuild(GetWorld());}
