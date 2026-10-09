// The operator's controller in the twin studio: shows the mouse, puts the dashboard on screen, moves the camera to the open view
// (one CameraActor per view, blended with SetViewTargetWithBlend) and tells the callout layer where car parts are on screen.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MVVM/DashboardViewModel.h"
#include "MVVM/ViewModelBase.h"
#include "TwinOpsPlayerController.generated.h"

class AVehicleTwinActor;
class UUserWidget;

UCLASS()
class ATwinOpsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATwinOpsPlayerController();

	//~ AActor / APlayerController
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaSeconds) override;

	// Where a callout anchor on the car is on screen, in the same space as a full-screen Canvas Panel in the HUD. AnchorOffsetInCarSpace
	// moves the point in the car's own axes (X forward, Y right, Z up), e.g. out from a wheel. False when there is no car, no such
	// anchor, or the point is behind the camera.
	bool ProjectCarAnchorToWidgetPosition(FName CalloutAnchorTag, const FVector& AnchorOffsetInCarSpace, FVector2D& OutWidgetPosition);

private:
	void HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	// Blends to the view's camera, or cuts to it without a blend for the first view at Play.
	void ShowViewCamera(ETwinDashboardView View, bool bCutWithoutBlend);

	AActor* FindViewCameraActor(ETwinDashboardView View) const;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HudWidget;

	TWeakObjectPtr<AVehicleTwinActor> TwinCarActor;
	FViewModelSubscription DashboardViewModelSubscription;
	bool bHasShownFirstView = false;
};