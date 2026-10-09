#include "Telemetry/TelemetrySubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Telemetry/FileTelemetryReceiver.h"
#include "Telemetry/TelemetrySettings.h"
#include "Telemetry/WebSocketTelemetryReceiver.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTelemetry, Log, All);

// Twin.EngineDerate 1/0: sends the SPEC.md §2.5 command by hand, to the game world the console belongs to (each PIE window has its own).
static FAutoConsoleCommandWithWorldAndArgs EngineDerateConsoleCommand(
	TEXT("Twin.EngineDerate"),
	TEXT("Asks the vehicle to switch engine protection derate on (1) or off (0). Needs the WebSocket source and relay.py --source live."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& ConsoleArguments, UWorld* ConsoleWorld)
	{
		const bool bArgumentIsOn = ConsoleArguments.Num() == 1 && (ConsoleArguments[0] == TEXT("1") || ConsoleArguments[0] == TEXT("on"));
		const bool bArgumentIsOff = ConsoleArguments.Num() == 1 && (ConsoleArguments[0] == TEXT("0") || ConsoleArguments[0] == TEXT("off"));
		if (!bArgumentIsOn && !bArgumentIsOff)
		{
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("Usage: Twin.EngineDerate 1 (on) or Twin.EngineDerate 0 (off)."));
			return;
		}

		const UGameInstance* ConsoleGameInstance = ConsoleWorld ? ConsoleWorld->GetGameInstance() : nullptr;
		UTelemetrySubsystem* TelemetrySubsystem = ConsoleGameInstance ? ConsoleGameInstance->GetSubsystem<UTelemetrySubsystem>() : nullptr;
		if (!TelemetrySubsystem)
		{
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("Twin.EngineDerate: no running game here; use it during PIE, in the game window's console."));
			return;
		}
		TelemetrySubsystem->SendEngineDerateCommand(bArgumentIsOn, TEXT("console"));
	}));

void UTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// One setting picks the source; nothing downstream knows which one it is (SPEC.md §5–6).
	const UTelemetrySettings* TelemetrySettings = GetDefault<UTelemetrySettings>();
	bAutoEngineDerateOnCriticalCoolant = TelemetrySettings->bAutoEngineDerateOnCriticalCoolant;
	CommandAckTimeoutSeconds = TelemetrySettings->CommandAckTimeoutSeconds;
	if (TelemetrySettings->TelemetrySource == ETelemetrySource::WebSocket)
	{
		UWebSocketTelemetryReceiver* WebSocketTelemetryReceiver = NewObject<UWebSocketTelemetryReceiver>(this);
		WebSocketTelemetryReceiver->RelayUrl = TelemetrySettings->RelayUrl;
		WebSocketTelemetryReceiver->StaleAfterSeconds = TelemetrySettings->StaleAfterSeconds;
		WebSocketTelemetryReceiver->ConnectTimeoutSeconds = TelemetrySettings->ConnectTimeoutSeconds;
		WebSocketTelemetryReceiver->ReconnectInitialDelaySeconds = TelemetrySettings->ReconnectInitialDelaySeconds;
		WebSocketTelemetryReceiver->ReconnectMaxDelaySeconds = TelemetrySettings->ReconnectMaxDelaySeconds;
		ActiveTelemetryReceiver = WebSocketTelemetryReceiver;
	}
	else
	{
		UFileTelemetryReceiver* FileTelemetryReceiver = NewObject<UFileTelemetryReceiver>(this);
		FileTelemetryReceiver->TripFileRelativePath = TelemetrySettings->TripFileRelativePath;
		FileTelemetryReceiver->PlaybackSpeedMultiplier = TelemetrySettings->PlaybackSpeedMultiplier;
		ActiveTelemetryReceiver = FileTelemetryReceiver;
	}

	if (ActiveTelemetryReceiver->StartReceiving())
	{
		UE_LOG(LogVehicleTelemetry, Log, TEXT("Telemetry: receiving from %s."), *ActiveTelemetryReceiver->GetReceiverDisplayName());
	}
	else
	{
		// The receiver logged why. Keep it so the reason stays visible in one place; it delivers nothing.
		UE_LOG(LogVehicleTelemetry, Error, TEXT("Telemetry: %s failed to start, the car gets no telemetry this session."),
			*ActiveTelemetryReceiver->GetReceiverDisplayName());
	}

	bIsTelemetryTickingEnabled = true;
}

void UTelemetrySubsystem::Deinitialize()
{
	bIsTelemetryTickingEnabled = false;

	if (ActiveTelemetryReceiver)
	{
		ActiveTelemetryReceiver->StopReceiving();
		ActiveTelemetryReceiver = nullptr;
	}
	OnTelemetryUpdated.Clear();

	Super::Deinitialize();
}

void UTelemetrySubsystem::Tick(float DeltaSeconds)
{
	if (!ActiveTelemetryReceiver)
	{
		return;
	}

	NewTelemetrySamplesThisTick.Reset();
	ActiveTelemetryReceiver->PollNewTelemetrySamples(DeltaSeconds, NewTelemetrySamplesThisTick);

	for (const FVehicleTelemetry& NewTelemetrySample : NewTelemetrySamplesThisTick)
	{
		const EVehicleDriveMode PreviousDriveMode = bHasReceivedAnyTelemetrySample ? LatestTelemetrySample.DriveMode : NewTelemetrySample.DriveMode;
		PreviousTelemetrySample = bHasReceivedAnyTelemetrySample ? LatestTelemetrySample : NewTelemetrySample;
		LatestTelemetrySample = NewTelemetrySample;
		bHasReceivedAnyTelemetrySample = true;

		const EVehicleStatus PreviousOverallStatus = CurrentVehicleStatusReport.OverallStatus;
		const EVehicleStatus PreviousCoolantTemperatureStatus = CurrentVehicleStatusReport.CoolantTemperatureStatus;
		CurrentVehicleStatusReport = VehicleStatusEvaluator.EvaluateTelemetrySample(LatestTelemetrySample);
		if (CurrentVehicleStatusReport.OverallStatus != PreviousOverallStatus)
		{
			LogOverallStatusChange(PreviousOverallStatus, LatestTelemetrySample);
		}
		if (LatestTelemetrySample.DriveMode != PreviousDriveMode)
		{
			LogDriveModeChange(PreviousDriveMode, LatestTelemetrySample);
		}
		SendAutoEngineDerateIfCoolantJustBecameCritical(PreviousCoolantTemperatureStatus);

		OnTelemetryUpdated.Broadcast(LatestTelemetrySample);
	}

	ProcessVehicleCommandAcks();
	ExpireUnansweredVehicleCommands(FPlatformTime::Seconds());

	LogTelemetrySummaryOncePerSecond(DeltaSeconds, NewTelemetrySamplesThisTick);
}

bool UTelemetrySubsystem::SendEngineDerateCommand(bool bEnabled, const FString& CommandSource, const FString& CommandTrigger)
{
	const FString CommandTriggerForLog = CommandTrigger.IsEmpty() ? CommandSource : FString::Printf(TEXT("%s: %s"), *CommandSource, *CommandTrigger);

	FVehicleCommand VehicleCommand;
	VehicleCommand.CommandId = NextVehicleCommandId++;
	VehicleCommand.CommandName = VehicleCommandNames::EngineDerate;
	VehicleCommand.bEnabled = bEnabled;

	FVehicleCommandRecord CommandRecord;
	CommandRecord.CommandId = VehicleCommand.CommandId;
	CommandRecord.CommandName = VehicleCommand.CommandName;
	CommandRecord.bEnabled = bEnabled;
	CommandRecord.CommandSource = CommandSource;
	CommandRecord.SentAtTripSeconds = LatestTelemetrySample.SampleTimeS;

	FString SendFailureReason = TEXT("no telemetry receiver");
	if (!ActiveTelemetryReceiver || !ActiveTelemetryReceiver->SendVehicleCommand(VehicleCommand, SendFailureReason))
	{
		UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s (%s) not sent: %s."), *DescribeVehicleCommand(VehicleCommand), *CommandTriggerForLog, *SendFailureReason);
		CommandRecord.Outcome = EVehicleCommandOutcome::NotSent;
		CommandRecord.OutcomeReason = SendFailureReason;
		PublishVehicleCommandRecord(CommandRecord);
		return false;
	}

	CommandRecord.Outcome = EVehicleCommandOutcome::WaitingForAck;
	PendingVehicleCommands.Add({ VehicleCommand, CommandTriggerForLog, FPlatformTime::Seconds(), CommandRecord });
	UE_LOG(LogVehicleTelemetry, Log, TEXT("%s sent (%s), waiting for the ack."), *DescribeVehicleCommand(VehicleCommand), *CommandTriggerForLog);
	PublishVehicleCommandRecord(CommandRecord);
	return true;
}

void UTelemetrySubsystem::PublishVehicleCommandRecord(const FVehicleCommandRecord& UpdatedCommandRecord)
{
	// Acks can arrive out of order across commands; the dashboard shows the newest command, the event log hears about all of them.
	if (UpdatedCommandRecord.CommandId >= LatestVehicleCommandRecord.CommandId)
	{
		LatestVehicleCommandRecord = UpdatedCommandRecord;
	}
	OnVehicleCommandUpdated.Broadcast(UpdatedCommandRecord);
}

FString UTelemetrySubsystem::GetActiveReceiverDisplayName() const
{
	return ActiveTelemetryReceiver ? ActiveTelemetryReceiver->GetReceiverDisplayName() : FString();
}

void UTelemetrySubsystem::ProcessVehicleCommandAcks()
{
	VehicleCommandAcksThisTick.Reset();
	ActiveTelemetryReceiver->PollVehicleCommandAcks(VehicleCommandAcksThisTick);

	for (const FVehicleCommandAck& VehicleCommandAck : VehicleCommandAcksThisTick)
	{
		const int32 PendingCommandIndex = PendingVehicleCommands.IndexOfByPredicate([&VehicleCommandAck](const FPendingVehicleCommand& PendingCommand)
		{
			return PendingCommand.VehicleCommand.CommandId == VehicleCommandAck.CommandId;
		});
		if (PendingCommandIndex == INDEX_NONE)
		{
			// Late (after its timeout) or for a command the relay couldn't read (commandId -1).
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("Ack for command %d, which isn't waiting for one: %s%s%s."), VehicleCommandAck.CommandId,
				*VehicleCommandAck.Status, VehicleCommandAck.Reason.IsEmpty() ? TEXT("") : TEXT(", "), *VehicleCommandAck.Reason);
			continue;
		}

		const FPendingVehicleCommand& PendingCommand = PendingVehicleCommands[PendingCommandIndex];
		const double AckDelayMs = (FPlatformTime::Seconds() - PendingCommand.SentTimeSeconds) * 1000.0;
		FVehicleCommandRecord AnsweredCommandRecord = PendingCommand.CommandRecord;
		AnsweredCommandRecord.AckDelayMs = static_cast<float>(AckDelayMs);
		AnsweredCommandRecord.AppliedAtSeq = VehicleCommandAck.AppliedAtSeq;
		if (VehicleCommandAck.WasApplied())
		{
			UE_LOG(LogVehicleTelemetry, Log, TEXT("%s applied by the vehicle from seq %lld (ack after %.0f ms); telemetry DriveMode shows the effect."),
				*DescribeVehicleCommand(PendingCommand.VehicleCommand), VehicleCommandAck.AppliedAtSeq, AckDelayMs);
			AnsweredCommandRecord.Outcome = EVehicleCommandOutcome::Applied;
		}
		else
		{
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s rejected: %s (ack after %.0f ms)."), *DescribeVehicleCommand(PendingCommand.VehicleCommand),
				*VehicleCommandAck.Reason, AckDelayMs);
			AnsweredCommandRecord.Outcome = EVehicleCommandOutcome::Rejected;
			AnsweredCommandRecord.OutcomeReason = VehicleCommandAck.Reason;
		}
		PendingVehicleCommands.RemoveAt(PendingCommandIndex);
		PublishVehicleCommandRecord(AnsweredCommandRecord);
	}
}

void UTelemetrySubsystem::ExpireUnansweredVehicleCommands(double NowSeconds)
{
	for (int32 PendingCommandIndex = PendingVehicleCommands.Num() - 1; PendingCommandIndex >= 0; --PendingCommandIndex)
	{
		const FPendingVehicleCommand& PendingCommand = PendingVehicleCommands[PendingCommandIndex];
		if (NowSeconds - PendingCommand.SentTimeSeconds > CommandAckTimeoutSeconds)
		{
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s (%s): no ack within %.1f s, treated as failed (not retried)."),
				*DescribeVehicleCommand(PendingCommand.VehicleCommand), *PendingCommand.CommandTrigger, CommandAckTimeoutSeconds);
			FVehicleCommandRecord UnansweredCommandRecord = PendingCommand.CommandRecord;
			UnansweredCommandRecord.Outcome = EVehicleCommandOutcome::NoAck;
			UnansweredCommandRecord.OutcomeReason = FString::Printf(TEXT("no ack within %.1f s"), CommandAckTimeoutSeconds);
			PendingVehicleCommands.RemoveAt(PendingCommandIndex);
			PublishVehicleCommandRecord(UnansweredCommandRecord);
		}
	}
}

void UTelemetrySubsystem::SendAutoEngineDerateIfCoolantJustBecameCritical(EVehicleStatus PreviousCoolantTemperatureStatus)
{
	// Edge, not level: one command per change into Critical. Coolant falls back to Warning under derate, so this fires again only
	// after a trip restart (relay --loop starts a new vehicle with derate off).
	const bool bCoolantJustBecameCritical = CurrentVehicleStatusReport.CoolantTemperatureStatus == EVehicleStatus::Critical
		&& PreviousCoolantTemperatureStatus != EVehicleStatus::Critical;
	if (!bCoolantJustBecameCritical || !bAutoEngineDerateOnCriticalCoolant || LatestTelemetrySample.DriveMode == EVehicleDriveMode::EngineDerate)
	{
		return;
	}
	SendEngineDerateCommand(true, TEXT("auto"), FString::Printf(TEXT("coolant Critical at %.1f C, t=%.1f s, seq %lld"), LatestTelemetrySample.CoolantTempC,
		LatestTelemetrySample.SampleTimeS, LatestTelemetrySample.Seq));
}

void UTelemetrySubsystem::LogDriveModeChange(EVehicleDriveMode PreviousDriveMode, const FVehicleTelemetry& TelemetrySample) const
{
	const UEnum* DriveModeEnum = StaticEnum<EVehicleDriveMode>();
	UE_LOG(LogVehicleTelemetry, Log, TEXT("Vehicle drive mode: %s -> %s at t=%.1f s, seq %lld (reported by the vehicle)."),
		*DriveModeEnum->GetNameStringByValue(static_cast<int64>(PreviousDriveMode)),
		*DriveModeEnum->GetNameStringByValue(static_cast<int64>(TelemetrySample.DriveMode)), TelemetrySample.SampleTimeS, TelemetrySample.Seq);
}

FString UTelemetrySubsystem::DescribeVehicleCommand(const FVehicleCommand& VehicleCommand)
{
	return FString::Printf(TEXT("Command %d %s %s"), VehicleCommand.CommandId, *VehicleCommand.CommandName, VehicleCommand.bEnabled ? TEXT("on") : TEXT("off"));
}

FTelemetryConnectionStatus UTelemetrySubsystem::GetTelemetryConnectionStatus() const
{
	return ActiveTelemetryReceiver ? ActiveTelemetryReceiver->GetConnectionStatus() : FTelemetryConnectionStatus();
}

TStatId UTelemetrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTelemetrySubsystem, STATGROUP_Tickables);
}

ETickableTickType UTelemetrySubsystem::GetTickableTickType() const
{
	// The class default object registers as a tickable too (FTickableGameObject's constructor does that); it must never tick.
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool UTelemetrySubsystem::IsTickable() const
{
	return bIsTelemetryTickingEnabled;
}

UWorld* UTelemetrySubsystem::GetTickableGameObjectWorld() const
{
	// Ties ticking to this game instance's world, so with two PIE windows each subsystem ticks once per frame of its own world.
	const UGameInstance* OwningGameInstance = GetGameInstance();
	return OwningGameInstance ? OwningGameInstance->GetWorld() : nullptr;
}

void UTelemetrySubsystem::LogOverallStatusChange(EVehicleStatus PreviousOverallStatus, const FVehicleTelemetry& TelemetrySample) const
{
	const UEnum* VehicleStatusEnum = StaticEnum<EVehicleStatus>();
	auto StatusName = [VehicleStatusEnum](EVehicleStatus Status)
	{
		return VehicleStatusEnum->GetNameStringByValue(static_cast<int64>(Status));
	};

	const FVehicleStatusReport& Report = CurrentVehicleStatusReport;
	TArray<FString> SignalsNotNormal;
	auto AddSignalIfNotNormal = [&SignalsNotNormal, &StatusName](const TCHAR* SignalName, EVehicleStatus SignalStatus)
	{
		if (SignalStatus != EVehicleStatus::Normal)
		{
			SignalsNotNormal.Add(FString::Printf(TEXT("%s %s"), SignalName, *StatusName(SignalStatus)));
		}
	};
	AddSignalIfNotNormal(TEXT("coolant"), Report.CoolantTemperatureStatus);
	AddSignalIfNotNormal(TEXT("tyres"), Report.TyrePressureStatus);
	AddSignalIfNotNormal(TEXT("fuel"), Report.FuelLevelStatus);
	AddSignalIfNotNormal(TEXT("battery"), Report.BatteryVoltageStatus);
	AddSignalIfNotNormal(TEXT("rpm"), Report.EngineRpmStatus);

	const FString SignalSummary = SignalsNotNormal.IsEmpty() ? TEXT("all signals Normal") : FString::Join(SignalsNotNormal, TEXT(", "));

	// Warning verbosity whenever the car isn't Normal, so the change stands out (yellow) between the once-a-second summary lines.
	if (Report.OverallStatus == EVehicleStatus::Normal)
	{
		UE_LOG(LogVehicleTelemetry, Log, TEXT("Vehicle status: %s -> %s at t=%.1f s, seq %lld (%s)."),
			*StatusName(PreviousOverallStatus), *StatusName(Report.OverallStatus), TelemetrySample.SampleTimeS, TelemetrySample.Seq, *SignalSummary);
	}
	else
	{
		UE_LOG(LogVehicleTelemetry, Warning, TEXT("Vehicle status: %s -> %s at t=%.1f s, seq %lld (%s)."),
			*StatusName(PreviousOverallStatus), *StatusName(Report.OverallStatus), TelemetrySample.SampleTimeS, TelemetrySample.Seq, *SignalSummary);
	}
}

void UTelemetrySubsystem::LogTelemetrySummaryOncePerSecond(float DeltaSeconds, const TArray<FVehicleTelemetry>& NewTelemetrySamples)
{
	if (!NewTelemetrySamples.IsEmpty())
	{
		if (SamplesSinceLastTelemetrySummaryLog == 0)
		{
			FirstSeqSinceLastTelemetrySummaryLog = NewTelemetrySamples[0].Seq;
		}
		SamplesSinceLastTelemetrySummaryLog += NewTelemetrySamples.Num();
	}

	SecondsSinceLastTelemetrySummaryLog += DeltaSeconds;
	if (SecondsSinceLastTelemetrySummaryLog < 1.0)
	{
		return;
	}

	// Seq range reads "1895..4" across a loop: expected, not dropped samples. Latency is 0 for the file receiver.
	const FTelemetryConnectionStatus TelemetryConnectionStatus = GetTelemetryConnectionStatus();
	UE_LOG(LogVehicleTelemetry, Log, TEXT("Telemetry: %d samples in %.2f s, seq %lld..%lld, t=%.1f s, %.1f km/h, %.0f rpm, coolant %.1f C, tyre RR %.0f kPa, openings %d, %s [%s, dropped %lld, latency %.1f ms avg / %.1f max]."),
		SamplesSinceLastTelemetrySummaryLog, SecondsSinceLastTelemetrySummaryLog, FirstSeqSinceLastTelemetrySummaryLog, LatestTelemetrySample.Seq,
		LatestTelemetrySample.SampleTimeS, LatestTelemetrySample.SpeedKmh, LatestTelemetrySample.EngineRpm, LatestTelemetrySample.CoolantTempC,
		LatestTelemetrySample.TyreKpa.RR, LatestTelemetrySample.Openings,
		*StaticEnum<EVehicleDriveMode>()->GetNameStringByValue(static_cast<int64>(LatestTelemetrySample.DriveMode)),
		*StaticEnum<ETelemetryConnectionState>()->GetNameStringByValue(static_cast<int64>(TelemetryConnectionStatus.ConnectionState)),
		TelemetryConnectionStatus.DroppedMessageCount, TelemetryConnectionStatus.AverageReceiveLatencyMs, TelemetryConnectionStatus.MaxReceiveLatencyMs);

	SecondsSinceLastTelemetrySummaryLog = 0.0;
	SamplesSinceLastTelemetrySummaryLog = 0;
}