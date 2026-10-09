// Game mode for the twin studio level: no pawn to walk around, just ATwinOpsPlayerController with the dashboard and the view cameras.
// Pick it in the level's World Settings > GameMode Override.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TwinOpsGameMode.generated.h"

UCLASS()
class ATwinOpsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATwinOpsGameMode();
};