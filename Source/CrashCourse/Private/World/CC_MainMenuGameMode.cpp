#include "World/CC_MainMenuGameMode.h"
#include "Player/CC_MainMenuController.h"
#include "GameFramework/HUD.h"

ACC_MainMenuGameMode::ACC_MainMenuGameMode()
{
 DefaultPawnClass=nullptr;HUDClass=AHUD::StaticClass();PlayerControllerClass=ACC_MainMenuController::StaticClass();
}
void ACC_MainMenuGameMode::RestartPlayer(AController* NewPlayer)
{
 // Intentionally no pawn: menu input never activates combat or a character save.
}
