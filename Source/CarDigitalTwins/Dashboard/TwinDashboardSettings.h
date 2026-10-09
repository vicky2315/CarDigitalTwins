// Project Settings > Game > Twin Dashboard. Saved to Config/DefaultGame.ini.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DeveloperSettings.h"
#include "TwinDashboardSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Twin Dashboard"))
class UTwinDashboardSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// The HUD ATwinOpsPlayerController adds to the screen at Play: WBP_TwinOpsHUD.
	UPROPERTY(config, EditAnywhere, Category = "HUD")
	TSoftClassPtr<UUserWidget> HudWidgetClass;

	// How long the camera takes to move to another view (SetViewTargetWithBlend).
	UPROPERTY(config, EditAnywhere, Category = "Camera", meta = (ClampMin = "0", ClampMax = "5", Units = "s"))
	float CameraBlendSeconds = 0.9f;

	// Ease-in-out curve steepness: 1 = linear, 2 = gentle start and stop (like the mock), higher = snappier middle.
	UPROPERTY(config, EditAnywhere, Category = "Camera", meta = (ClampMin = "1", ClampMax = "8"))
	float CameraBlendExponent = 2.f;

	// Camera actors are found by actor tag: this prefix plus the view name, e.g. TwinView.Overview.
	UPROPERTY(config, EditAnywhere, Category = "Camera")
	FString CameraActorTagPrefix = TEXT("TwinView.");
};