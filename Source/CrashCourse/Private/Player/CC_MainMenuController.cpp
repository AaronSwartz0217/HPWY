#include "Player/CC_MainMenuController.h"
#include "UI/CC_MainMenuWidget.h"
#include "World/CC_AdventureGameInstance.h"

void ACC_MainMenuController::BeginPlay()
{
 Super::BeginPlay();if (!IsLocalController()) return;
 MainMenu=CreateWidget<UCC_MainMenuWidget>(this,MainMenuClass ? MainMenuClass.Get() : UCC_MainMenuWidget::StaticClass());
 if (!MainMenu) return;
 MainMenu->AddToViewport(100);bShowMouseCursor=true;
 if (auto* Adventure=GetGameInstance<UCC_AdventureGameInstance>()) Adventure->EnsureBackgroundMusic();
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(MainMenu->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
}
void ACC_MainMenuController::EndPlay(const EEndPlayReason::Type Reason)
{
 if (MainMenu) {MainMenu->RemoveFromParent();MainMenu=nullptr;}
 Super::EndPlay(Reason);
}
