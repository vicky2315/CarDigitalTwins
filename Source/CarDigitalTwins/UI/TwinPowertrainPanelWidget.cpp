#include "UI/TwinPowertrainPanelWidget.h"

#include "Components/TextBlock.h"
#include "UI/TwinHmiButtonWidget.h"
#include "UI/TwinHmiStyle.h"
#include "UI/TwinRangeBarWidget.h"
#include "UI/TwinStatusTextWidget.h"

void UTwinPowertrainPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (RequestDerateButton)
	{
		RequestDerateButton->OnButtonClicked.AddUObject(this, &UTwinPowertrainPanelWidget::HandleRequestDerateClicked);
	}
	if (CancelDerateButton)
	{
		CancelDerateButton->OnButtonClicked.AddUObject(this, &UTwinPowertrainPanelWidget::HandleCancelDerateClicked);
	}
}

void UTwinPowertrainPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinPowertrainPanelWidget::HandleTelemetryFieldsChanged);
	SubscribeToViewModel(ConnectionViewModel, this, &UTwinPowertrainPanelWidget::HandleConnectionFieldsChanged);
}

void UTwinPowertrainPanelWidget::HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!TelemetryViewModel)
	{
		return;
	}
	using TelemetryField = EVehicleTelemetryViewModelField;
	const UVehicleTelemetryViewModel& Telemetry = *TelemetryViewModel;
	const bool bIsDerated = Telemetry.GetDriveMode() == EVehicleDriveMode::EngineDerate;

	if (ChangedFieldMask.HasField(TelemetryField::DriveMode) && DriveModeValueText)
	{
		DriveModeValueText->SetText(bIsDerated ? NSLOCTEXT("TwinHmi", "PanelDriveModeDerate", "Engine derate") : NSLOCTEXT("TwinHmi", "PanelDriveModeNormal", "Normal"));
		DriveModeValueText->SetColorAndOpacity(FSlateColor(bIsDerated ? TwinHmiColors::Warning() : TwinHmiColors::TextPrimary()));
	}
	if (ChangedFieldMask.HasField(TelemetryField::CoolantTempC) || ChangedFieldMask.HasField(TelemetryField::CoolantTemperatureStatus))
	{
		const EVehicleStatus CoolantStatusValue = Telemetry.GetCoolantTemperatureStatus();
		if (CoolantValueText)
		{
			CoolantValueText->SetText(TwinHmiFormat::FormatFixed(Telemetry.GetCoolantTempC(), 1));
			CoolantValueText->SetColorAndOpacity(FSlateColor(TwinHmiColors::ForStatus(CoolantStatusValue, TwinHmiColors::TextPrimary())));
		}
		if (CoolantStatus)
		{
			CoolantStatus->SetStatus(CoolantStatusValue);
		}
		if (CoolantBar)
		{
			CoolantBar->SetBarValue(Telemetry.GetCoolantTempC(), CoolantStatusValue);
		}
	}
	if (ChangedFieldMask.HasField(TelemetryField::EngineRpm) || ChangedFieldMask.HasField(TelemetryField::EngineRpmStatus)
		|| ChangedFieldMask.HasField(TelemetryField::DriveMode))
	{
		if (EngineRpmValueText)
		{
			EngineRpmValueText->SetText(TwinHmiFormat::FormatFixed(FMath::RoundToFloat(Telemetry.GetEngineRpm() / 10.f) * 10.f, 0));
		}
		if (EngineRpmStatus)
		{
			EngineRpmStatus->SetStatus(Telemetry.GetEngineRpmStatus());
		}
		if (EngineRpmBar)
		{
			EngineRpmBar->SetBarValue(Telemetry.GetEngineRpm(), Telemetry.GetEngineRpmStatus());
			EngineRpmBar->SetLimitLine(bIsDerated, 2500.f);
		}
	}
	if (ChangedFieldMask.HasField(TelemetryField::FuelPct) && FuelValueText)
	{
		FuelValueText->SetText(TwinHmiFormat::FormatFixed(Telemetry.GetFuelPct(), 0));
	}
	if (ChangedFieldMask.HasField(TelemetryField::BatteryV) || ChangedFieldMask.HasField(TelemetryField::BatteryVoltageStatus)
		|| ChangedFieldMask.HasField(TelemetryField::EngineRpm))
	{
		if (BatteryValueText)
		{
			BatteryValueText->SetText(TwinHmiFormat::FormatFixed(Telemetry.GetBatteryV(), 2));
		}
		if (BatteryUnitText)
		{
			BatteryUnitText->SetText(Telemetry.GetEngineRpm() > 0.f ? NSLOCTEXT("TwinHmi", "UnitVolt", "V")
				: FText::FromString(TEXT("V · engine off")));
		}
		if (BatteryStatus)
		{
			BatteryStatus->SetStatus(Telemetry.GetBatteryVoltageStatus());
		}
	}
}

void UTwinPowertrainPanelWidget::HandleConnectionFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!ConnectionViewModel || !CommandStatusText || !ChangedFieldMask.HasField(EConnectionViewModelField::LatestVehicleCommand))
	{
		return;
	}
	const FVehicleCommandRecord& Command = ConnectionViewModel->GetLatestVehicleCommand();
	const FString CommandName = FString::Printf(TEXT("Command %d derate %s (%s)"), Command.CommandId, Command.bEnabled ? TEXT("on") : TEXT("off"),
		*Command.CommandSource);
	const TCHAR* Separator = TEXT(" · ");
	FString CommandLine;
	switch (Command.Outcome)
	{
	case EVehicleCommandOutcome::None:
		CommandLine = TEXT("No command sent this trip");
		break;
	case EVehicleCommandOutcome::WaitingForAck:
		CommandLine = CommandName + Separator + TEXT("sent, waiting for the ack");
		break;
	case EVehicleCommandOutcome::Applied:
		CommandLine = CommandName + Separator + FString::Printf(TEXT("applied by the vehicle from seq %lld%sack %.0f ms"), Command.AppliedAtSeq, Separator,
			Command.AckDelayMs);
		break;
	case EVehicleCommandOutcome::Rejected:
		CommandLine = CommandName + Separator + TEXT("rejected: ") + Command.OutcomeReason;
		break;
	case EVehicleCommandOutcome::NoAck:
		CommandLine = CommandName + Separator + Command.OutcomeReason;
		break;
	case EVehicleCommandOutcome::NotSent:
		CommandLine = FString::Printf(TEXT("Derate %s not sent: %s"), Command.bEnabled ? TEXT("on") : TEXT("off"), *Command.OutcomeReason);
		break;
	}
	CommandStatusText->SetText(FText::FromString(CommandLine));
	const bool bProblem = Command.Outcome == EVehicleCommandOutcome::Rejected || Command.Outcome == EVehicleCommandOutcome::NoAck
		|| Command.Outcome == EVehicleCommandOutcome::NotSent;
	CommandStatusText->SetColorAndOpacity(FSlateColor(bProblem ? TwinHmiColors::Warning() : TwinHmiColors::TextSecondary()));
}

void UTwinPowertrainPanelWidget::HandleRequestDerateClicked()
{
	if (ConnectionViewModel)
	{
		ConnectionViewModel->RequestEngineDerate(true);
	}
}

void UTwinPowertrainPanelWidget::HandleCancelDerateClicked()
{
	if (ConnectionViewModel)
	{
		ConnectionViewModel->RequestEngineDerate(false);
	}
}