#include "MVVM/VehicleEventLogViewModel.h"

#include "Engine/GameInstance.h"

namespace VehicleEventLogText
{
	// · = middle dot, ° = degree sign: escaped so the file stays plain ASCII for every compiler setting.
	static const TCHAR* Separator = TEXT(" · ");

	const TCHAR* StatusName(EVehicleStatus Status)
	{
		switch (Status)
		{
		case EVehicleStatus::Warning: return TEXT("Warning");
		case EVehicleStatus::Critical: return TEXT("Critical");
		default: return TEXT("Normal");
		}
	}

	EVehicleEventSeverity SeverityOf(EVehicleStatus Status)
	{
		switch (Status)
		{
		case EVehicleStatus::Warning: return EVehicleEventSeverity::Warning;
		case EVehicleStatus::Critical: return EVehicleEventSeverity::Critical;
		default: return EVehicleEventSeverity::Information;
		}
	}

	// "RR 179 kPa": the lowest tyre, which is the one a tyre status is about (a slow puncture).
	FString LowestTyreText(const FTyrePressuresKpa& TyrePressures)
	{
		const TPair<const TCHAR*, float> Tyres[] = { { TEXT("FL"), TyrePressures.FL }, { TEXT("FR"), TyrePressures.FR },
			{ TEXT("RL"), TyrePressures.RL }, { TEXT("RR"), TyrePressures.RR } };
		const TPair<const TCHAR*, float>* LowestTyre = &Tyres[0];
		for (const TPair<const TCHAR*, float>& Tyre : Tyres)
		{
			if (Tyre.Value < LowestTyre->Value)
			{
				LowestTyre = &Tyre;
			}
		}
		return FString::Printf(TEXT("%s %.0f kPa"), LowestTyre->Key, LowestTyre->Value);
	}
}

void UVehicleEventLogViewModel::InitializeViewModel(UGameInstance& OwningGameInstance)
{
	UTelemetrySubsystem* TelemetrySubsystem = OwningGameInstance.GetSubsystem<UTelemetrySubsystem>();
	if (!TelemetrySubsystem)
	{
		return;
	}
	SubscribedTelemetrySubsystem = TelemetrySubsystem;
	TelemetryUpdatedDelegateHandle = TelemetrySubsystem->OnTelemetryUpdated.AddUObject(this, &UVehicleEventLogViewModel::HandleTelemetryUpdated);
	VehicleCommandUpdatedDelegateHandle = TelemetrySubsystem->OnVehicleCommandUpdated.AddUObject(this, &UVehicleEventLogViewModel::HandleVehicleCommandUpdated);
}

void UVehicleEventLogViewModel::DeinitializeViewModel()
{
	if (UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		TelemetrySubsystem->OnTelemetryUpdated.Remove(TelemetryUpdatedDelegateHandle);
		TelemetrySubsystem->OnVehicleCommandUpdated.Remove(VehicleCommandUpdatedDelegateHandle);
	}
	SubscribedTelemetrySubsystem.Reset();
}

void UVehicleEventLogViewModel::AcknowledgeAlarm()
{
	if (!ActiveAlarm.bIsActive || ActiveAlarm.bIsAcknowledged)
	{
		return;
	}
	FVehicleAlarm AcknowledgedAlarm = ActiveAlarm;
	AcknowledgedAlarm.bIsAcknowledged = true;
	SetField(EVehicleEventLogViewModelField::ActiveAlarm, ActiveAlarm, AcknowledgedAlarm);
	AddEvent(PreviousTelemetrySample.SampleTimeS, EVehicleEventSeverity::Information, TEXT("Alarm acknowledged"));
}

void UVehicleEventLogViewModel::AddEvent(double TripSeconds, EVehicleEventSeverity Severity, FString EventMessage)
{
	FVehicleEventLogEntry NewEvent;
	NewEvent.TripSeconds = TripSeconds;
	NewEvent.Severity = Severity;
	NewEvent.EventMessage = MoveTemp(EventMessage);
	RecentEvents.Insert(MoveTemp(NewEvent), 0);
	if (RecentEvents.Num() > MaxKeptEventCount)
	{
		RecentEvents.SetNum(MaxKeptEventCount);
	}
	MarkFieldChanged(EVehicleEventLogViewModelField::RecentEvents);
}

void UVehicleEventLogViewModel::LogSignalStatusChange(const TCHAR* SignalName, EVehicleStatus PreviousStatus, EVehicleStatus NewStatus,
	const FString& ValueText, double TripSeconds)
{
	if (NewStatus != PreviousStatus)
	{
		AddEvent(TripSeconds, VehicleEventLogText::SeverityOf(NewStatus),
			FString::Printf(TEXT("%s %s%s%s"), SignalName, VehicleEventLogText::StatusName(NewStatus), VehicleEventLogText::Separator, *ValueText));
	}
}

void UVehicleEventLogViewModel::HandleTelemetryUpdated(const FVehicleTelemetry& NewTelemetrySample)
{
	const UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get();
	if (!TelemetrySubsystem)
	{
		return;
	}
	const FVehicleStatusReport& NewStatusReport = TelemetrySubsystem->GetCurrentVehicleStatusReport();
	const double TripSeconds = NewTelemetrySample.SampleTimeS;

	if (bHasPreviousSample)
	{
		if (NewTelemetrySample.Seq < PreviousTelemetrySample.Seq)
		{
			AddEvent(TripSeconds, EVehicleEventSeverity::Information, TEXT("Trip restarted"));
		}

		LogSignalStatusChange(TEXT("Coolant"), PreviousVehicleStatusReport.CoolantTemperatureStatus, NewStatusReport.CoolantTemperatureStatus,
			FString::Printf(TEXT("%.1f °C"), NewTelemetrySample.CoolantTempC), TripSeconds);
		LogSignalStatusChange(TEXT("Tyres"), PreviousVehicleStatusReport.TyrePressureStatus, NewStatusReport.TyrePressureStatus,
			VehicleEventLogText::LowestTyreText(NewTelemetrySample.TyreKpa), TripSeconds);
		LogSignalStatusChange(TEXT("Fuel"), PreviousVehicleStatusReport.FuelLevelStatus, NewStatusReport.FuelLevelStatus,
			FString::Printf(TEXT("%.0f %%"), NewTelemetrySample.FuelPct), TripSeconds);
		LogSignalStatusChange(TEXT("Battery"), PreviousVehicleStatusReport.BatteryVoltageStatus, NewStatusReport.BatteryVoltageStatus,
			FString::Printf(TEXT("%.2f V"), NewTelemetrySample.BatteryV), TripSeconds);
		LogSignalStatusChange(TEXT("Engine rpm"), PreviousVehicleStatusReport.EngineRpmStatus, NewStatusReport.EngineRpmStatus,
			FString::Printf(TEXT("%.0f"), NewTelemetrySample.EngineRpm), TripSeconds);

		if (NewTelemetrySample.DriveMode != PreviousTelemetrySample.DriveMode)
		{
			AddEvent(TripSeconds, EVehicleEventSeverity::Command, NewTelemetrySample.DriveMode == EVehicleDriveMode::EngineDerate
				? TEXT("Drive mode → Engine derate") : TEXT("Drive mode → Normal"));
		}

		const TPair<EVehicleOpening, const TCHAR*> Openings[] = { { EVehicleOpening::DoorFL, TEXT("Door FL") },
			{ EVehicleOpening::DoorFR, TEXT("Door FR") }, { EVehicleOpening::DoorRL, TEXT("Door RL") }, { EVehicleOpening::DoorRR, TEXT("Door RR") },
			{ EVehicleOpening::Hood, TEXT("Hood") }, { EVehicleOpening::Tailgate, TEXT("Tailgate") } };
		for (const TPair<EVehicleOpening, const TCHAR*>& Opening : Openings)
		{
			const bool bWasOpen = PreviousTelemetrySample.IsOpen(Opening.Key);
			const bool bIsOpen = NewTelemetrySample.IsOpen(Opening.Key);
			if (bIsOpen != bWasOpen)
			{
				AddEvent(TripSeconds, EVehicleEventSeverity::Information, FString::Printf(TEXT("%s %s"), Opening.Value, bIsOpen ? TEXT("opened") : TEXT("closed")));
			}
		}
	}

	// The alarm: raised when the coolant turns Critical, cleared when it leaves Critical.
	const bool bCoolantIsCritical = NewStatusReport.CoolantTemperatureStatus == EVehicleStatus::Critical;
	if (bCoolantIsCritical && !ActiveAlarm.bIsActive)
	{
		FVehicleAlarm RaisedAlarm;
		RaisedAlarm.bIsActive = true;
		RaisedAlarm.AlarmTitle = FString::Printf(TEXT("CRITICAL%sCOOLANT %.1f °C"), VehicleEventLogText::Separator, NewTelemetrySample.CoolantTempC);
		RaisedAlarm.AlarmDetail = TEXT("Engine overheating");
		SetField(EVehicleEventLogViewModelField::ActiveAlarm, ActiveAlarm, RaisedAlarm);
	}
	else if (!bCoolantIsCritical && ActiveAlarm.bIsActive)
	{
		SetField(EVehicleEventLogViewModelField::ActiveAlarm, ActiveAlarm, FVehicleAlarm());
	}

	bHasPreviousSample = true;
	PreviousTelemetrySample = NewTelemetrySample;
	PreviousVehicleStatusReport = NewStatusReport;
}

void UVehicleEventLogViewModel::HandleVehicleCommandUpdated(const FVehicleCommandRecord& UpdatedCommandRecord)
{
	const TCHAR* OnOrOff = UpdatedCommandRecord.bEnabled ? TEXT("on") : TEXT("off");
	const double TripSeconds = PreviousTelemetrySample.SampleTimeS;
	switch (UpdatedCommandRecord.Outcome)
	{
	case EVehicleCommandOutcome::WaitingForAck:
		AddEvent(TripSeconds, EVehicleEventSeverity::Command, FString::Printf(TEXT("Command %d derate %s sent (%s)"), UpdatedCommandRecord.CommandId,
			OnOrOff, *UpdatedCommandRecord.CommandSource));
		// The automatic derate belongs to the alarm that caused it: say so on the banner.
		if (ActiveAlarm.bIsActive && UpdatedCommandRecord.CommandSource == TEXT("auto"))
		{
			FVehicleAlarm UpdatedAlarm = ActiveAlarm;
			UpdatedAlarm.AlarmDetail = TEXT("Engine derate requested automatically");
			SetField(EVehicleEventLogViewModelField::ActiveAlarm, ActiveAlarm, UpdatedAlarm);
		}
		break;
	case EVehicleCommandOutcome::Applied:
		AddEvent(TripSeconds, EVehicleEventSeverity::Command, FString::Printf(TEXT("Vehicle applied derate %s from seq %lld"), OnOrOff,
			UpdatedCommandRecord.AppliedAtSeq));
		break;
	case EVehicleCommandOutcome::Rejected:
		AddEvent(TripSeconds, EVehicleEventSeverity::Warning, FString::Printf(TEXT("Command %d rejected: %s"), UpdatedCommandRecord.CommandId,
			*UpdatedCommandRecord.OutcomeReason));
		break;
	case EVehicleCommandOutcome::NoAck:
		AddEvent(TripSeconds, EVehicleEventSeverity::Warning, FString::Printf(TEXT("Command %d: %s"), UpdatedCommandRecord.CommandId,
			*UpdatedCommandRecord.OutcomeReason));
		break;
	case EVehicleCommandOutcome::NotSent:
		AddEvent(TripSeconds, EVehicleEventSeverity::Warning, FString::Printf(TEXT("Derate %s not sent: %s"), OnOrOff, *UpdatedCommandRecord.OutcomeReason));
		break;
	default:
		break;
	}
}