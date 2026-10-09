// Owns the active telemetry receiver, polls it every frame and hands each new sample to the rest of the game.
// Data flow and who subscribes: docs/SPEC.md §5–6. Knows no UI class.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TelemetryReceiver.h"
#include "VehicleStatusEvaluator.h"
#include "VehicleTelemetry.h"
#include "TelemetrySubsystem.generated.h"

// Broadcast once per new sample, oldest first, on the game thread.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVehicleTelemetryUpdated, const FVehicleTelemetry& /*NewTelemetrySample*/);

// Where a command stands (SPEC.md §2.5). "Applied" only means the vehicle accepted it; telemetry DriveMode shows the effect.
UENUM(BlueprintType)
enum class EVehicleCommandOutcome : uint8
{
	// Nothing sent yet this session.
	None,
	// Couldn't go out at all (recorded trip, no connection); OutcomeReason says why.
	NotSent,
	WaitingForAck,
	Applied,
	Rejected,
	// No ack within the timeout; not retried.
	NoAck,
};

// One command and what happened to it, for the dashboard and the event log.
USTRUCT(BlueprintType)
struct FVehicleCommandRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	int32 CommandId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	FString CommandName;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	bool bEnabled = false;

	// Who asked: "operator", "console", "auto".
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	FString CommandSource;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	EVehicleCommandOutcome Outcome = EVehicleCommandOutcome::None;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	int64 AppliedAtSeq = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	float AckDelayMs = 0.f;

	// Rejection or not-sent reason; empty otherwise.
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	FString OutcomeReason;

	// Trip time of the latest sample when the command was sent, for the event log.
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Command")
	double SentAtTripSeconds = 0.0;

	bool operator==(const FVehicleCommandRecord& OtherRecord) const
	{
		return CommandId == OtherRecord.CommandId && Outcome == OtherRecord.Outcome && bEnabled == OtherRecord.bEnabled
			&& AppliedAtSeq == OtherRecord.AppliedAtSeq && OutcomeReason.Equals(OtherRecord.OutcomeReason, ESearchCase::CaseSensitive);
	}
};

// Broadcast whenever a command is sent or its outcome changes (ack, rejection, timeout), on the game thread.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVehicleCommandUpdated, const FVehicleCommandRecord& /*UpdatedCommandRecord*/);

// GameInstance subsystem (not World) so the receiver, and with it the WebSocket connection, survives level loads, matching
// UViewModelSubsystem (SPEC.md §5). Ticks through FTickableGameObject, only in game worlds and not while paused.
UCLASS()
class UTelemetrySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//~ USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//~ FTickableGameObject
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual UWorld* GetTickableGameObjectWorld() const override;

	// False until the first sample arrives; the getters below return default (all zero) samples until then.
	bool HasReceivedAnyTelemetrySample() const { return bHasReceivedAnyTelemetrySample; }

	const FVehicleTelemetry& GetLatestTelemetrySample() const { return LatestTelemetrySample; }

	// The sample before the latest one. Day 8 interpolates between the two; on a trip loop SampleTimeS goes backwards between them.
	const FVehicleTelemetry& GetPreviousTelemetrySample() const { return PreviousTelemetrySample; }

	// Status of the latest sample (SPEC.md §3). Already updated when OnTelemetryUpdated fires for that sample.
	const FVehicleStatusReport& GetCurrentVehicleStatusReport() const { return CurrentVehicleStatusReport; }

	// Connection state, dropped messages and latency of the active receiver (SPEC.md §4). Idle when there is none.
	FTelemetryConnectionStatus GetTelemetryConnectionStatus() const;

	FOnVehicleTelemetryUpdated OnTelemetryUpdated;

	// Asks the vehicle to switch engine protection derate on or off (SPEC.md §2.5). CommandSource says who asked ("operator",
	// "console", "auto"); CommandTrigger adds why, for the log. Returns false, and logs why, when the command couldn't go out; true
	// means sent, not obeyed: the ack is logged when it arrives, and the vehicle's DriveMode in telemetry shows the actual effect.
	bool SendEngineDerateCommand(bool bEnabled, const FString& CommandSource, const FString& CommandTrigger = FString());

	// The newest command and its outcome; Outcome None before the first one.
	const FVehicleCommandRecord& GetLatestVehicleCommandRecord() const { return LatestVehicleCommandRecord; }

	FOnVehicleCommandUpdated OnVehicleCommandUpdated;

	// "WebSocket ws://127.0.0.1:8765" or "File trip_sample.json"; empty without a receiver.
	FString GetActiveReceiverDisplayName() const;

private:
	// A sent command waiting for its ack.
	struct FPendingVehicleCommand
	{
		FVehicleCommand VehicleCommand;
		FString CommandTrigger;
		// FPlatformTime::Seconds when sent: real time, so the timeout doesn't depend on frame rate or time dilation.
		double SentTimeSeconds = 0.0;
		FVehicleCommandRecord CommandRecord;
	};

	// Stores the record as the latest if it is the newest command, and tells the listeners.
	void PublishVehicleCommandRecord(const FVehicleCommandRecord& UpdatedCommandRecord);

	FVehicleCommandRecord LatestVehicleCommandRecord;

	// Matches the acks the receiver got this tick to pending commands and logs each outcome.
	void ProcessVehicleCommandAcks();

	// Logs and forgets commands without an ack after CommandAckTimeoutSeconds. They are not retried (SPEC.md §2.5).
	void ExpireUnansweredVehicleCommands(double NowSeconds);

	// Sends derate when the coolant status has just become Critical, if the setting is on and the vehicle isn't derated already.
	void SendAutoEngineDerateIfCoolantJustBecameCritical(EVehicleStatus PreviousCoolantTemperatureStatus);

	// Logs one line when the vehicle reports a different drive mode: the visible proof that a command took effect.
	void LogDriveModeChange(EVehicleDriveMode PreviousDriveMode, const FVehicleTelemetry& TelemetrySample) const;

	static FString DescribeVehicleCommand(const FVehicleCommand& VehicleCommand);

	// Logs one line when the overall status changes, naming every signal that isn't Normal.
	void LogOverallStatusChange(EVehicleStatus PreviousOverallStatus, const FVehicleTelemetry& TelemetrySample) const;

	// Logs one line per second of game time: how many samples arrived and the latest values, instead of 10 lines a second.
	void LogTelemetrySummaryOncePerSecond(float DeltaSeconds, const TArray<FVehicleTelemetry>& NewTelemetrySamples);

	UPROPERTY()
	TScriptInterface<ITelemetryReceiver> ActiveTelemetryReceiver;

	FVehicleTelemetry LatestTelemetrySample;
	FVehicleTelemetry PreviousTelemetrySample;
	bool bHasReceivedAnyTelemetrySample = false;

	// Fed every received sample in order, never interpolated ones (see FVehicleStatusEvaluator).
	FVehicleStatusEvaluator VehicleStatusEvaluator;
	FVehicleStatusReport CurrentVehicleStatusReport;

	// Reused every tick so polling doesn't allocate.
	TArray<FVehicleTelemetry> NewTelemetrySamplesThisTick;
	TArray<FVehicleCommandAck> VehicleCommandAcksThisTick;

	TArray<FPendingVehicleCommand> PendingVehicleCommands;

	// Rises by one per command for the whole session, so it also rises within each connection as SPEC.md §2.5 asks.
	int32 NextVehicleCommandId = 1;

	// Copied from UTelemetrySettings in Initialize.
	bool bAutoEngineDerateOnCriticalCoolant = true;
	double CommandAckTimeoutSeconds = 2.0;

	// True between Initialize and Deinitialize. The class default object is also a tickable object, so ticking must be opt-in.
	bool bIsTelemetryTickingEnabled = false;

	double SecondsSinceLastTelemetrySummaryLog = 0.0;
	int32 SamplesSinceLastTelemetrySummaryLog = 0;
	int64 FirstSeqSinceLastTelemetrySummaryLog = 0;
};