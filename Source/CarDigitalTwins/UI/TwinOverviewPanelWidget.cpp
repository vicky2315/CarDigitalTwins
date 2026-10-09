#include "UI/TwinOverviewPanelWidget.h"

#include "Components/TextBlock.h"
#include "Telemetry/VehicleStatusEvaluator.h"
#include "UI/TwinHmiStyle.h"
#include "UI/TwinRangeBarWidget.h"
#include "UI/TwinSystemRowWidget.h"

void UTwinOverviewPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinOverviewPanelWidget::HandleTelemetryFieldsChanged);
}

void UTwinOverviewPanelWidget::HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!TelemetryViewModel)
	{
		return;
	}
	using TelemetryField = EVehicleTelemetryViewModelField;
	const UVehicleTelemetryViewModel& Telemetry = *TelemetryViewModel;
	const bool bIsDerated = Telemetry.GetDriveMode() == EVehicleDriveMode::EngineDerate;

	// Only what changed is redrawn: this is the point of the field mask.
	if (ChangedFieldMask.HasField(TelemetryField::SpeedKmh) && SpeedValueText)
	{
		SpeedValueText->SetText(TwinHmiFormat::FormatFixed(Telemetry.GetSpeedKmh(), 0));
	}
	if (ChangedFieldMask.HasField(TelemetryField::Gear) && GearValueText)
	{
		GearValueText->SetText(TwinHmiFormat::GearText(Telemetry.GetGear()));
	}
	if (ChangedFieldMask.HasField(TelemetryField::EngineRpm) || ChangedFieldMask.HasField(TelemetryField::EngineRpmStatus)
		|| ChangedFieldMask.HasField(TelemetryField::DriveMode))
	{
		if (EngineRpmValueText)
		{
			EngineRpmValueText->SetText(TwinHmiFormat::FormatFixed(FMath::RoundToFloat(Telemetry.GetEngineRpm() / 10.f) * 10.f, 0));
		}
		if (EngineRpmBar)
		{
			EngineRpmBar->SetBarValue(Telemetry.GetEngineRpm(), Telemetry.GetEngineRpmStatus());
			EngineRpmBar->SetLimitLine(bIsDerated, 2500.f);
		}
	}

	if (PowertrainSystemRow)
	{
		const EVehicleStatus PowertrainStatus = FVehicleStatusEvaluator::WorseStatus(
			FVehicleStatusEvaluator::WorseStatus(Telemetry.GetCoolantTemperatureStatus(), Telemetry.GetEngineRpmStatus()),
			FVehicleStatusEvaluator::WorseStatus(Telemetry.GetFuelLevelStatus(), Telemetry.GetBatteryVoltageStatus()));
		PowertrainSystemRow->ShowSystemState(FText::FromString(FString::Printf(TEXT("Coolant %.1f °C · %s"), Telemetry.GetCoolantTempC(),
			bIsDerated ? TEXT("derated") : TEXT("normal mode"))), PowertrainStatus);
	}
	if (TyresSystemRow)
	{
		const TPair<const TCHAR*, float> Tyres[] = { { TEXT("FL"), Telemetry.GetTyrePressureFrontLeftKpa() },
			{ TEXT("FR"), Telemetry.GetTyrePressureFrontRightKpa() }, { TEXT("RL"), Telemetry.GetTyrePressureRearLeftKpa() },
			{ TEXT("RR"), Telemetry.GetTyrePressureRearRightKpa() } };
		const TPair<const TCHAR*, float>* LowestTyre = &Tyres[0];
		for (const TPair<const TCHAR*, float>& Tyre : Tyres)
		{
			LowestTyre = Tyre.Value < LowestTyre->Value ? &Tyre : LowestTyre;
		}
		TyresSystemRow->ShowSystemState(FText::FromString(FString::Printf(TEXT("Lowest %s %.0f kPa"), LowestTyre->Key, LowestTyre->Value)),
			Telemetry.GetTyrePressureStatus());
	}
	if (BodySystemRow && ChangedFieldMask.HasField(TelemetryField::Openings))
	{
		const TPair<EVehicleOpening, const TCHAR*> Openings[] = { { EVehicleOpening::DoorFL, TEXT("Door FL") },
			{ EVehicleOpening::DoorFR, TEXT("Door FR") }, { EVehicleOpening::DoorRL, TEXT("Door RL") }, { EVehicleOpening::DoorRR, TEXT("Door RR") },
			{ EVehicleOpening::Hood, TEXT("Hood") }, { EVehicleOpening::Tailgate, TEXT("Tailgate") } };
		TArray<FString> OpenNames;
		for (const TPair<EVehicleOpening, const TCHAR*>& Opening : Openings)
		{
			if (Telemetry.IsOpen(Opening.Key))
			{
				OpenNames.Add(Opening.Value);
			}
		}
		const bool bAnythingOpen = !OpenNames.IsEmpty();
		BodySystemRow->ShowSystemStateWithLabel(
			bAnythingOpen ? FText::FromString(FString::Join(OpenNames, TEXT(", ")) + TEXT(" open")) : NSLOCTEXT("TwinHmi", "AllClosed", "All closed"),
			bAnythingOpen ? NSLOCTEXT("TwinHmi", "StatusOpen", "Open") : NSLOCTEXT("TwinHmi", "StatusClosed", "Closed"),
			bAnythingOpen ? EVehicleStatus::Warning : EVehicleStatus::Normal);
	}
}