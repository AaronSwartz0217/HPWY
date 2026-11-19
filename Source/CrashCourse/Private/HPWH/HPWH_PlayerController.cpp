#include "HPWH/HPWH_PlayerController.h"

#include "HPWH/HPWH_Player.h"
#include "HPWH/HPWH_HUD.h"
#include "HPWH/HPWH_POI.h"
#include "HPWH/HPWH_GameMode.h"
#include "HPWH/HPWH_Upgrades.h"
#include "HPWH/HPWH_Menu.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// HUD 以 DPI 缩放后的 Slate 单位绘制按钮，触摸/鼠标判定必须换算到同一坐标系，
	// 否则高 DPI 设备（多数手机）上按钮显示位置与命中区域会错开，表现为「按钮点不动」。
	float HPWH_UIScale(UObject* WorldContext)
	{
		const float Scale = UWidgetLayoutLibrary::GetViewportScale(WorldContext);
		return Scale > KINDA_SMALL_NUMBER ? Scale : 1.f;
	}
}

AHPWH_PlayerController::AHPWH_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bEnableTouchEvents = true;
	bShowMouseCursor = true;
}

void AHPWH_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		HUDWidget = CreateWidget<UHPWH_HUDWidget>(this, UHPWH_HUDWidget::StaticClass());
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
			HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		FInputModeGameOnly Mode;
		SetInputMode(Mode);
		bShowMouseCursor = true;
	}
}

void AHPWH_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AHPWH_PlayerController::ForwardPressed);
	InputComponent->BindKey(EKeys::W, IE_Released, this, &AHPWH_PlayerController::ForwardReleased);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AHPWH_PlayerController::BackPressed);
	InputComponent->BindKey(EKeys::S, IE_Released, this, &AHPWH_PlayerController::BackReleased);
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AHPWH_PlayerController::RightPressed);
	InputComponent->BindKey(EKeys::D, IE_Released, this, &AHPWH_PlayerController::RightReleased);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AHPWH_PlayerController::LeftPressed);
	InputComponent->BindKey(EKeys::A, IE_Released, this, &AHPWH_PlayerController::LeftReleased);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AHPWH_PlayerController::InteractPressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AHPWH_PlayerController::AttackPressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AHPWH_PlayerController::AttackReleased);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AHPWH_PlayerController::LookPressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AHPWH_PlayerController::LookReleased);

	InputComponent->BindTouch(IE_Pressed, this, &AHPWH_PlayerController::TouchPressed);
	InputComponent->BindTouch(IE_Repeat, this, &AHPWH_PlayerController::TouchMoved);
	InputComponent->BindTouch(IE_Released, this, &AHPWH_PlayerController::TouchReleased);
}

void AHPWH_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsStoryActive())
		{
			// 鼠标点击由 AttackPressed 处理，这里只接键盘，避免同帧推进两页。
			if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::SpaceBar) ||
				WasInputKeyJustPressed(EKeys::One))
			{
				GM->AdvanceStory();
			}
			return;
		}
		if (GM->IsMainMenuActive())
		{
			if (WasInputKeyJustPressed(EKeys::One)) GM->HandleMenuSelection(0);
			if (WasInputKeyJustPressed(EKeys::Two)) GM->HandleMenuSelection(1);
			if (WasInputKeyJustPressed(EKeys::Three)) GM->HandleMenuSelection(2);
			if (WasInputKeyJustPressed(EKeys::Four)) GM->HandleMenuSelection(3);
			if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::BackSpace))
			{
				GM->MenuBack();
			}
			if (WasInputKeyJustPressed(EKeys::Enter) && GM->GetMenuPage() == EHPWH_MenuPage::Main)
			{
				GM->HandleMenuSelection(0);
			}
			return;
		}
		if (GM->IsPortalChoiceActive())
		{
			if (WasInputKeyJustPressed(EKeys::One) || WasInputKeyJustPressed(EKeys::Enter) ||
				WasInputKeyJustPressed(EKeys::SpaceBar))
			{
				GM->ConfirmFloorPortalChoice();
			}
			return;
		}
		if (GM->IsUpgradeSelectionActive())
		{
			if (WasInputKeyJustPressed(EKeys::One)) GM->ChooseUpgrade(0);
			if (WasInputKeyJustPressed(EKeys::Two)) GM->ChooseUpgrade(1);
			if (WasInputKeyJustPressed(EKeys::Three)) GM->ChooseUpgrade(2);
			if (WasInputKeyJustPressed(EKeys::R)) GM->RerollUpgrade();
			return;
		}
		if (GM->IsInGameMenuOpen())
		{
			if (WasInputKeyJustPressed(EKeys::One)) GM->HandleInGameMenuSelection(0);
			if (WasInputKeyJustPressed(EKeys::Two)) GM->HandleInGameMenuSelection(1);
			if (WasInputKeyJustPressed(EKeys::Three)) GM->HandleInGameMenuSelection(2);
			if (WasInputKeyJustPressed(EKeys::Escape) || WasInputKeyJustPressed(EKeys::BackSpace))
			{
				if (GM->IsInGameSettingsOpen())
				{
					GM->HandleInGameMenuSelection(2);
				}
				else
				{
					GM->CloseInGameMenu();
				}
			}
			return;
		}
	}

	APawn* ControlledPawn = GetPawn();
	AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(ControlledPawn);
	if (Body && !Body->IsRunLocked() && WasInputKeyJustPressed(EKeys::R))
	{
		AHPWH_CourtyardPOI* Best = nullptr;
		float BestDistance = TNumericLimits<float>::Max();
		for (TActorIterator<AHPWH_CourtyardPOI> It(GetWorld()); It; ++It)
		{
			const float Distance = FVector::Dist2D(Body->GetActorLocation(), It->GetActorLocation());
			if (Distance <= It->GetHPWHInteractRange() && Distance < BestDistance && It->CanHPWHInteract(Body))
			{
				Best = *It;
				BestDistance = Distance;
			}
		}
		if (Best) Best->HPWHInteract(Body);
	}

	float MouseDeltaX = 0.f;
	float MouseDeltaY = 0.f;
	GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	if (FMath::Abs(MouseDeltaX) + FMath::Abs(MouseDeltaY) > 0.05f)
	{
		bManualAim = true;
	}

	if (!ControlledPawn)
	{
		return;
	}

	FVector2D Move = HPWH_TouchToMoveInput(JoystickInput);
	if (bForward) Move.Y += 1.f;
	if (bBack) Move.Y -= 1.f;
	if (bRight) Move.X += 1.f;
	if (bLeft) Move.X -= 1.f;
	Move = Move.GetClampedToMaxSize(1.f);

	const bool bLocked = Body && (Body->IsRunLocked() || Body->IsSoulOut());
	if (!bLocked && Move.SizeSquared() > 0.01f)
	{
		ControlledPawn->AddMovementInput(FVector::ForwardVector, Move.Y);
		ControlledPawn->AddMovementInput(FVector::RightVector, Move.X);
	}

	if ((bMouseAttack || bTouchAttack) && Body && !bLocked)
	{
		Body->TryAttack();
	}

	if (GetWorld()->GetTimeSeconds() >= NextPromptUpdate)
	{
		NextPromptUpdate = GetWorld()->GetTimeSeconds() + 0.15f;
		UpdateInteractPrompt();
	}
}

void AHPWH_PlayerController::UpdateInteractPrompt()
{
	CachedPrompt.Empty();

	if (const AHPWH_SoulAvatar* Soul = Cast<AHPWH_SoulAvatar>(GetPawn()))
	{
		if (Soul->CanReturnToBody())
		{
			CachedPrompt = TEXT("回到肉身");
		}
		return;
	}

	if (AActor* Target = FindNearestInteractable())
	{
		if (const IHPWH_Interactable* Interactable = Cast<IHPWH_Interactable>(Target))
		{
			CachedPrompt = Interactable->GetHPWHInteractPrompt(GetPawn());
		}
	}
}

AActor* AHPWH_PlayerController::FindNearestInteractable() const
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !GetWorld())
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	const AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	const float RangeMultiplier = GM ? 1.f + GM->GetRunStats().InteractRangeBonus : 1.f;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		IHPWH_Interactable* Interactable = Cast<IHPWH_Interactable>(*It);
		if (!Interactable || !Interactable->CanHPWHInteract(ControlledPawn))
		{
			continue;
		}
		const float Distance = FVector::Dist2D(ControlledPawn->GetActorLocation(), It->GetActorLocation());
		if (Distance <= Interactable->GetHPWHInteractRange() * RangeMultiplier && Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	return Best;
}

void AHPWH_PlayerController::Interact()
{
	if (AHPWH_SoulAvatar* Soul = Cast<AHPWH_SoulAvatar>(GetPawn()))
	{
		Soul->TryReturnToBody();
		return;
	}
	if (AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(GetPawn()))
	{
		if (AActor* Target = FindNearestInteractable())
		{
			if (IHPWH_Interactable* Interactable = Cast<IHPWH_Interactable>(Target))
			{
				Interactable->HPWHInteract(Body);
			}
		}
	}
}

bool AHPWH_PlayerController::TryInteractNearby()
{
	if (AHPWH_SoulAvatar* Soul = Cast<AHPWH_SoulAvatar>(GetPawn()))
	{
		if (Soul->CanReturnToBody())
		{
			Soul->TryReturnToBody();
			return true;
		}
		return false;
	}
	AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(GetPawn());
	if (!Body)
	{
		return false;
	}
	AActor* Target = FindNearestInteractable();
	IHPWH_Interactable* Interactable = Cast<IHPWH_Interactable>(Target);
	if (!Interactable)
	{
		return false;
	}
	Interactable->HPWHInteract(Body);
	return true;
}

bool AHPWH_PlayerController::GetAttackAimPoint(FVector& OutPoint) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}
	if (!bManualAim)
	{
		return false;
	}

	FVector2D ScreenPosition = TouchAimScreen;
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (JoystickFinger < 0 && LookFinger < 0 && GetMousePosition(MouseX, MouseY))
	{
		ScreenPosition = FVector2D(MouseX, MouseY);
	}
	else if (ScreenPosition.IsNearlyZero())
	{
		return false;
	}

	FVector Origin;
	FVector Direction;
	if (!DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, Origin, Direction))
	{
		return false;
	}
	const float PlaneZ = ControlledPawn->GetActorLocation().Z;
	if (FMath::Abs(Direction.Z) < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float T = (PlaneZ - Origin.Z) / Direction.Z;
	if (T <= 0.f)
	{
		return false;
	}
	OutPoint = Origin + Direction * T;
	return true;
}

void AHPWH_PlayerController::HandleMenuPoint(const FVector2D& Point)
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || !GM->IsMainMenuActive())
	{
		return;
	}
	const FHPWH_MenuLayout Layout = HPWH_MakeMenuLayout(GetViewportSize2D());
	if (GM->GetMenuPage() == EHPWH_MenuPage::Main)
	{
		const int32 ButtonCount = GM->HasSaveGame() ? 4 : 3;
		for (int32 Index = 0; Index < ButtonCount; ++Index)
		{
			if (HPWH_IsPointInMenuButton(Point, Layout.MainButtons[Index], Layout.ButtonSize))
			{
				GM->HandleMenuSelection(Index);
				return;
			}
		}
	}
	else if (GM->GetMenuPage() == EHPWH_MenuPage::Settings)
	{
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (HPWH_IsPointInMenuButton(Point, Layout.SettingsRows[Index], Layout.SettingsRowSize))
			{
				GM->HandleMenuSelection(Index);
				return;
			}
		}
		if (HPWH_IsPointInMenuButton(Point, Layout.BackCenter, Layout.ButtonSize))
		{
			GM->HandleMenuSelection(2);
		}
	}
	else if (HPWH_IsPointInMenuButton(Point, Layout.BackCenter, Layout.ButtonSize))
	{
		GM->HandleMenuSelection(0);
	}
}

void AHPWH_PlayerController::HandleInGameMenuPoint(const FVector2D& Point)
{
	auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || GM->IsMainMenuActive())
	{
		return;
	}
	const FHPWH_InGameMenuLayout Layout = HPWH_MakeInGameMenuLayout(GetViewportSize2D());
	if (!GM->IsInGameMenuOpen())
	{
		if (HPWH_IsPointInMenuButton(Point, Layout.ButtonCenter, Layout.ButtonSize))
		{
			GM->ToggleInGameMenu();
		}
		return;
	}
	// 全屏菜单下，顶部按钮切换为关闭。
	if (HPWH_IsPointInMenuButton(Point, Layout.ButtonCenter, Layout.ButtonSize))
	{
		GM->CloseInGameMenu();
		return;
	}
	if (GM->IsInGameSettingsOpen())
	{
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (HPWH_IsPointInMenuButton(Point, Layout.SettingsRows[Index], Layout.SettingsRowSize))
			{
				GM->HandleInGameMenuSelection(Index);
				return;
			}
		}
		if (HPWH_IsPointInMenuButton(Point, Layout.BackCenter, Layout.RowSize))
		{
			GM->HandleInGameMenuSelection(2);
		}
		return;
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (HPWH_IsPointInMenuButton(Point, Layout.Rows[Index], Layout.RowSize))
		{
			GM->HandleInGameMenuSelection(Index);
			return;
		}
	}
}

void AHPWH_PlayerController::TouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	const FVector2D Size = GetViewportSize2D();
	const float UIScale = HPWH_UIScale(this);
	const FVector2D Point(Location.X / UIScale, Location.Y / UIScale);

	if (auto* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsStoryActive())
		{
			GM->AdvanceStory();
			return;
		}
		if (GM->IsMainMenuActive())
		{
			HandleMenuPoint(Point);
			return;
		}
		if (GM->IsPortalChoiceActive())
		{
			const FHPWH_PortalCardLayout PortalLayout = HPWH_MakePortalCardLayout(Size);
			if (HPWH_IsPointInMenuButton(Point, PortalLayout.ButtonCenter, PortalLayout.ButtonSize))
			{
				GM->ConfirmFloorPortalChoice();
			}
			return;
		}
		if (!GM->IsUpgradeSelectionActive())
		{
			const FHPWH_InGameMenuLayout InGameLayout = HPWH_MakeInGameMenuLayout(Size);
			if (GM->IsInGameMenuOpen() ||
				HPWH_IsPointInMenuButton(Point, InGameLayout.ButtonCenter, InGameLayout.ButtonSize))
			{
				HandleInGameMenuPoint(Point);
				return;
			}
		}
	}

	if (AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsUpgradeSelectionActive())
		{
			const FHPWH_UpgradeCardLayout Layout = HPWH_MakeUpgradeCardLayout(Size);
			if (HPWH_IsPointInMenuButton(Point, Layout.RerollCenter, Layout.RerollSize))
			{
				GM->RerollUpgrade();
				return;
			}
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FVector2D Min = Layout.Centers[Index] - Layout.CardSize * 0.5f;
				const FVector2D Max = Layout.Centers[Index] + Layout.CardSize * 0.5f;
				if (Point.X >= Min.X && Point.X <= Max.X && Point.Y >= Min.Y && Point.Y <= Max.Y)
				{
					GM->ChooseUpgrade(Index);
					return;
				}
			}
			return;
		}
	}

	const FHPWH_TouchLayout Layout = HPWH_MakeTouchLayout(Size);
	const int32 Finger = static_cast<int32>(FingerIndex);

	if (FVector2D::Distance(Point, Layout.JoystickCenter) <= Layout.JoystickRadius)
	{
		if (JoystickFinger != INDEX_NONE) return;
		JoystickFinger = Finger;
		JoystickOrigin = Point;
		return;
	}
	if (FVector2D::Distance(Point, Layout.AttackCenter) <= Layout.AttackRadius)
	{
		if (AttackFinger != INDEX_NONE) return;
		AttackFinger = Finger;
		bTouchAttack = true;
		bManualAim = false;
		AttackOrigin = Point;
		AttackVisualOffset = FVector2D::ZeroVector;
		// 攻击键优先触发附近交互，没有可交互对象时才开火。
		if (!TryInteractNearby())
		{
			if (AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(GetPawn()))
			{
				Body->TryAttack();
			}
		}
		return;
	}
	if (Point.X > Size.X * 0.42f)
	{
		if (LookFinger != INDEX_NONE) return;
		LookFinger = Finger;
		LookLast = Point;
		TouchAimScreen = FVector2D(Location.X, Location.Y);
		bManualAim = true;
	}
}

void AHPWH_PlayerController::TouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
	const int32 Finger = static_cast<int32>(FingerIndex);
	const float UIScale = HPWH_UIScale(this);
	const FVector2D Point(Location.X / UIScale, Location.Y / UIScale);

	if (Finger == JoystickFinger)
	{
		const FHPWH_TouchLayout Layout = HPWH_MakeTouchLayout(GetViewportSize2D());
		FVector2D Delta = (Point - JoystickOrigin) / FMath::Max(1.f, Layout.JoystickRadius);
		if (Delta.SizeSquared() > 1.f)
		{
			Delta = Delta.GetSafeNormal();
		}
		else if (Delta.SizeSquared() < 0.012f)
		{
			// 约 11% 的死区，避免手指轻微抖动导致角色漂移。
			Delta = FVector2D::ZeroVector;
		}
		JoystickInput = Delta;
		return;
	}
	if (Finger == AttackFinger)
	{
		const float Travel = HPWH_MakeTouchLayout(GetViewportSize2D()).AttackRadius * 0.45f;
		AttackVisualOffset = (Point - AttackOrigin).GetClampedToMaxSize(Travel);
		return;
	}
	if (Finger == LookFinger)
	{
		const FVector2D Delta = Point - LookLast;
		LookLast = Point;
		TouchAimScreen = FVector2D(Location.X, Location.Y);
		bManualAim = true;
	}
}

void AHPWH_PlayerController::TouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
	const int32 Finger = static_cast<int32>(FingerIndex);
	if (Finger == JoystickFinger)
	{
		JoystickFinger = -1;
		JoystickInput = FVector2D::ZeroVector;
	}
	if (Finger == AttackFinger)
	{
		AttackFinger = -1;
		bTouchAttack = false;
		AttackVisualOffset = FVector2D::ZeroVector;
	}
	if (Finger == LookFinger)
	{
		LookFinger = -1;
	}
}

void AHPWH_PlayerController::ForwardPressed() { bForward = true; }
void AHPWH_PlayerController::ForwardReleased() { bForward = false; }
void AHPWH_PlayerController::BackPressed() { bBack = true; }
void AHPWH_PlayerController::BackReleased() { bBack = false; }
void AHPWH_PlayerController::RightPressed() { bRight = true; }
void AHPWH_PlayerController::RightReleased() { bRight = false; }
void AHPWH_PlayerController::LeftPressed() { bLeft = true; }
void AHPWH_PlayerController::LeftReleased() { bLeft = false; }

void AHPWH_PlayerController::AttackPressed()
{
	if (AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsStoryActive())
		{
			GM->AdvanceStory();
			return;
		}
		if (GM->IsMainMenuActive())
		{
			float MouseX = 0.f;
			float MouseY = 0.f;
			if (GetMousePosition(MouseX, MouseY))
			{
				HandleMenuPoint(FVector2D(MouseX, MouseY) / HPWH_UIScale(this));
			}
			return;
		}
		if (GM->IsPortalChoiceActive())
		{
			float MouseX = 0.f;
			float MouseY = 0.f;
			if (GetMousePosition(MouseX, MouseY))
			{
				const FHPWH_PortalCardLayout PortalLayout = HPWH_MakePortalCardLayout(GetViewportSize2D());
				if (HPWH_IsPointInMenuButton(FVector2D(MouseX, MouseY) / HPWH_UIScale(this),
					PortalLayout.ButtonCenter, PortalLayout.ButtonSize))
				{
					GM->ConfirmFloorPortalChoice();
				}
			}
			return;
		}
		if (GM->IsUpgradeSelectionActive())
		{
			float MouseX = 0.f;
			float MouseY = 0.f;
			if (GetMousePosition(MouseX, MouseY))
			{
				const FVector2D Point = FVector2D(MouseX, MouseY) / HPWH_UIScale(this);
				const FHPWH_UpgradeCardLayout Layout = HPWH_MakeUpgradeCardLayout(GetViewportSize2D());
				if (HPWH_IsPointInMenuButton(Point, Layout.RerollCenter, Layout.RerollSize))
				{
					GM->RerollUpgrade();
					return;
				}
				for (int32 Index = 0; Index < 3; ++Index)
				{
					const FVector2D Min = Layout.Centers[Index] - Layout.CardSize * 0.5f;
					const FVector2D Max = Layout.Centers[Index] + Layout.CardSize * 0.5f;
					if (Point.X >= Min.X && Point.X <= Max.X && Point.Y >= Min.Y && Point.Y <= Max.Y)
					{
						GM->ChooseUpgrade(Index);
						return;
					}
				}
			}
			return;
		}
		if (GM->IsInGameMenuOpen())
		{
			float MouseX = 0.f;
			float MouseY = 0.f;
			if (GetMousePosition(MouseX, MouseY))
			{
				HandleInGameMenuPoint(FVector2D(MouseX, MouseY) / HPWH_UIScale(this));
			}
			return;
		}
		float MenuMouseX = 0.f;
		float MenuMouseY = 0.f;
		if (!GM->IsUpgradeSelectionActive() && GetMousePosition(MenuMouseX, MenuMouseY))
		{
			const FHPWH_InGameMenuLayout InGameLayout = HPWH_MakeInGameMenuLayout(GetViewportSize2D());
			if (HPWH_IsPointInMenuButton(FVector2D(MenuMouseX, MenuMouseY) / HPWH_UIScale(this),
				InGameLayout.ButtonCenter, InGameLayout.ButtonSize))
			{
				GM->ToggleInGameMenu();
				return;
			}
		}
	}

	bMouseAttack = true;
	bManualAim = true;
	if (TryInteractNearby())
	{
		bMouseAttack = false;
		return;
	}
	if (AHPWH_PlayerCharacter* Body = Cast<AHPWH_PlayerCharacter>(GetPawn()))
	{
		Body->TryAttack();
	}
}

void AHPWH_PlayerController::AttackReleased() { bMouseAttack = false; }
void AHPWH_PlayerController::LookPressed() { bLookHeld = true; }
void AHPWH_PlayerController::LookReleased() { bLookHeld = false; }
void AHPWH_PlayerController::InteractPressed()
{
	if (const AHPWH_GameMode* GM = Cast<AHPWH_GameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsStoryActive())
		{
			return;
		}
	}
	Interact();
}

FVector2D AHPWH_PlayerController::GetViewportSize2D() const
{
	const FVector2D Size = UWidgetLayoutLibrary::GetViewportSize(const_cast<AHPWH_PlayerController*>(this));
	if (Size.X <= 0.f || Size.Y <= 0.f)
	{
		return FVector2D(1920.f, 1080.f);
	}
	return Size;
}
