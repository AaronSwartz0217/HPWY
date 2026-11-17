#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CC_MainMenuController.generated.h"
class UCC_MainMenuWidget;

UCLASS()
class CRASHCOURSE_API ACC_MainMenuController : public APlayerController
{
 GENERATED_BODY()
public:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="MainMenu") TSubclassOf<UCC_MainMenuWidget> MainMenuClass;
 UFUNCTION(BlueprintPure,Category="MainMenu") UCC_MainMenuWidget* GetMainMenu() const {return MainMenu;}
private:
 UPROPERTY(Transient) TObjectPtr<UCC_MainMenuWidget> MainMenu;
};
