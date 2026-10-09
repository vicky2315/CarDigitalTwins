// The Body view's panel: the six openings (four doors, hood, tailgate) and the odometer. Doors don't move on the 3D car in Phase A,
// so this panel and the callouts are the only place they show.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinBodyPanelWidget.generated.h"

class UTextBlock;
class UTwinOpeningCardWidget;

UCLASS()
class UTwinBodyPanelWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinOpeningCardWidget> OpeningCardDoorFrontLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinOpeningCardWidget> OpeningCardDoorFrontRight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinOpeningCardWidget> OpeningCardDoorRearLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinOpeningCardWidget> OpeningCardDoorRearRight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinOpeningCardWidget> OpeningCardHood;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinOpeningCardWidget> OpeningCardTailgate;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OdometerValueText;

private:
	void HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;
};