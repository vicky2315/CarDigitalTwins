#include "UI/TwinViewRailWidget.h"

#include "Telemetry/VehicleStatusEvaluator.h"
#include "UI/TwinViewTabWidget.h"

void UTwinViewRailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(DashboardViewModel, this, &UTwinViewRailWidget::HandleDashboardFieldsChanged);
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinViewRailWidget::HandleTelemetryFieldsChanged);
}

void UTwinViewRailWidget::HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!DashboardViewModel || !ChangedFieldMask.HasField(EDashboardViewModelField::ActiveView))
	{
		return;
	}
	const ETwinDashboardView ActiveView = DashboardViewModel->GetActiveView();
	for (UTwinViewTabWidget* ViewTab : { ViewTabOverview.Get(), ViewTabPowertrain.Get(), ViewTabTyres.Get(), ViewTabBody.Get() })
	{
		if (ViewTab)
		{
			ViewTab->SetTabSelected(ViewTab->GetTabView() == ActiveView);
		}
	}
}

void UTwinViewRailWidget::HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!TelemetryViewModel)
	{
		return;
	}
	using TelemetryField = EVehicleTelemetryViewModelField;
	const bool bAnyStatusChanged = ChangedFieldMask.HasField(TelemetryField::CoolantTemperatureStatus)
		|| ChangedFieldMask.HasField(TelemetryField::EngineRpmStatus) || ChangedFieldMask.HasField(TelemetryField::FuelLevelStatus)
		|| ChangedFieldMask.HasField(TelemetryField::BatteryVoltageStatus) || ChangedFieldMask.HasField(TelemetryField::TyrePressureStatus);
	if (!bAnyStatusChanged)
	{
		return;
	}

	const UVehicleTelemetryViewModel& Telemetry = *TelemetryViewModel;
	const EVehicleStatus PowertrainStatus = FVehicleStatusEvaluator::WorseStatus(
		FVehicleStatusEvaluator::WorseStatus(Telemetry.GetCoolantTemperatureStatus(), Telemetry.GetEngineRpmStatus()),
		FVehicleStatusEvaluator::WorseStatus(Telemetry.GetFuelLevelStatus(), Telemetry.GetBatteryVoltageStatus()));
	if (ViewTabPowertrain)
	{
		ViewTabPowertrain->SetTabStatus(PowertrainStatus);
	}
	if (ViewTabTyres)
	{
		ViewTabTyres->SetTabStatus(Telemetry.GetTyrePressureStatus());
	}
}