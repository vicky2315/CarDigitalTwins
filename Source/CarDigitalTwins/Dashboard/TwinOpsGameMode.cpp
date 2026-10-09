#include "Dashboard/TwinOpsGameMode.h"

#include "Dashboard/TwinOpsPlayerController.h"

ATwinOpsGameMode::ATwinOpsGameMode()
{
	PlayerControllerClass = ATwinOpsPlayerController::StaticClass();
	// The camera comes from the view cameras placed in the level; a default pawn would only be something to fly around with.
	DefaultPawnClass = nullptr;
}