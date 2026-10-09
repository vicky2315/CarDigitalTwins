// The Overview view's panel: speed, gear, engine rpm against its range, and the Systems list (Powertrain, Tyres, Body) with status.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinOverviewPanelWidget.generated.h"

class UTextBlock;
class UTwinRangeBarWidget;
class UTwinSystemRowWidget;

UCLASS()
class UTwinOverviewPanelWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeedValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GearValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EngineRpmValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinRangeBarWidget> EngineRpmBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinSystemRowWidget> PowertrainSystemRow;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinSystemRowWidget> TyresSystemRow;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinSystemRowWidget> BodySystemRow;

private:
	void HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;
};