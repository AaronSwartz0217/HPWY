#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CC_MainMenuGameMode.generated.h"

// The front end has a UI controller only, with no gameplay pawn or progression HUD.
UCLASS()
class CRASHCOURSE_API ACC_MainMenuGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 ACC_MainMenuGameMode();
 virtual void RestartPlayer(AController* NewPlayer) override;
};
