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

	// Asks the vehicle to switch engine protection derate on or off (SPEC.md §2.5). CommandTrigger says why, for the log ("console",
	// "auto: coolant Critical"). Returns false, and logs why, when the command couldn't go out; true means sent, not obeyed: the
	// ack is logged when it arrives, and the vehicle's DriveMode in telemetry shows the actual effect.
	bool SendEngineDerateCommand(bool bEnabled, const FString& CommandTrigger);

private:
	// A sent command waiting for its ack.
	struct FPendingVehicleCommand
	{
		FVehicleCommand VehicleCommand;
		FString CommandTrigger;
		// FPlatformTime::Seconds when sent: real time, so the timeout doesn't depend on frame rate or time dilation.
		double SentTimeSeconds = 0.0;
	};

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