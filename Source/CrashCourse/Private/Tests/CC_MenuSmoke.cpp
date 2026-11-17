#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
// The original menu probe assumed Nature was the native form and that artifact
// interaction immediately transformed the player. The mural probe now covers
// the replacement flow: InitialInk, acquisition into the bag, explicit use,
// all three evolution routes, skill equipment and the new menu artwork/pages.
// Keep this command as an alias for existing dedicated test-process scripts.
// Its report/captures are Saved/MuralMenuSmoke.json and Screenshots/MuralMenu/.
void MenuSmoke(UWorld* World)
{
    UE_LOG(LogTemp, Display, TEXT("CC.MenuSmokeTest is a compatibility alias for CC.MuralMenuSmokeTest; see Saved/MuralMenuSmoke.json."));
    if (GEngine && World) GEngine->Exec(World, TEXT("CC.MuralMenuSmokeTest"));
}

FAutoConsoleCommandWithWorld MenuSmokeCommand(
    TEXT("CC.MenuSmokeTest"),
    TEXT("Compatibility alias for CC.MuralMenuSmokeTest (InitialInk, acquired powers, mural menu and evolution). Requires a dedicated standalone game process with rendering; writes Saved/MuralMenuSmoke.json."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&MenuSmoke));
}
#endif
