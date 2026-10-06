// Owns the active telemetry receiver, polls it every frame and hands each new sample to the rest of the game.
// Data flow and who subscribes: docs/SPEC.md §5–6. Knows no UI class.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TelemetryReceiver.h"
#include "VehicleTelemetry.h"
#include "TelemetrySubsystem.generated.h"

// Broadcast once per new sample, oldest first, on the game thread.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVehicleTelemetryUpdated, const FVehicleTelemetry& /*NewTelemetrySample*/);

// GameInstance subsystem (not World) so the receiver, and on Day 10 the WebSocket connection, survives level loads, matching
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

	FOnVehicleTelemetryUpdated OnTelemetryUpdated;

private:
	// Logs one line per second of game time: how many samples arrived and the latest values, instead of 10 lines a second.
	void LogTelemetrySummaryOncePerSecond(float DeltaSeconds, const TArray<FVehicleTelemetry>& NewTelemetrySamples);

	UPROPERTY()
	TScriptInterface<ITelemetryReceiver> ActiveTelemetryReceiver;

	FVehicleTelemetry LatestTelemetrySample;
	FVehicleTelemetry PreviousTelemetrySample;
	bool bHasReceivedAnyTelemetrySample = false;

	// Reused every tick so polling doesn't allocate.
	TArray<FVehicleTelemetry> NewTelemetrySamplesThisTick;

	// True between Initialize and Deinitialize. The class default object is also a tickable object, so ticking must be opt-in.
	bool bIsTelemetryTickingEnabled = false;

	double SecondsSinceLastTelemetrySummaryLog = 0.0;
	int32 SamplesSinceLastTelemetrySummaryLog = 0;
	int64 FirstSeqSinceLastTelemetrySummaryLog = 0;
};