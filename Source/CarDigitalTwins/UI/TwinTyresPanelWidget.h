// The Tyres view's panel: four tyre cards laid out like the car (FL FR on top, RL RR below).

#pragma once

#include "CoreMinimal.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinTyresPanelWidget.generated.h"

class UTwinTyreCardWidget;

UCLASS()
class UTwinTyresPanelWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinTyreCardWidget> TyreCardFrontLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinTyreCardWidget> TyreCardFrontRight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinTyreCardWidget> TyreCardRearLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinTyreCardWidget> TyreCardRearRight;

private:
	void HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;
};