// The Powertrain view's panel: drive mode, coolant and engine rpm against their ranges, fuel, battery, and the engine derate control
// with the latest command's outcome. The buttons only send the command (UConnectionViewModel::RequestEngineDerate); what the panel
// shows always comes back from telemetry (SPEC.md §5).

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ConnectionViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinPowertrainPanelWidget.generated.h"

class UTextBlock;
class UTwinHmiButtonWidget;
class UTwinRangeBarWidget;
class UTwinStatusTextWidget;

UCLASS()
class UTwinPowertrainPanelWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DriveModeValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CoolantValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinStatusTextWidget> CoolantStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinRangeBarWidget> CoolantBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EngineRpmValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinStatusTextWidget> EngineRpmStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinRangeBarWidget> EngineRpmBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FuelValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BatteryValueText;

	// "V", or "V · engine off" while the engine is off (battery not monitored then).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BatteryUnitText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinStatusTextWidget> BatteryStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CommandStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinHmiButtonWidget> RequestDerateButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinHmiButtonWidget> CancelDerateButton;

private:
	void HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void HandleConnectionFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void HandleRequestDerateClicked();
	void HandleCancelDerateClicked();

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UConnectionViewModel> ConnectionViewModel;
};