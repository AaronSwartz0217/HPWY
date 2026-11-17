#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CC_AdventureGameMode.generated.h"

// Applied to the three registered maps by the GameInstance, including old map overrides.
UCLASS()
class CRASHCOURSE_API ACC_AdventureGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ACC_AdventureGameMode();
	virtual void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
	virtual void StartPlay() override;
private:
	void ConfigureLegacyActors();
};
