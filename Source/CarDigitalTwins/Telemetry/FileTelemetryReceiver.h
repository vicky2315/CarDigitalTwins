// Plays a recorded trip (trip.json, docs/SPEC.md §2.2) at its recorded rate, looping. Day 7.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RecordedTripFile.h"
#include "TelemetryReceiver.h"
#include "FileTelemetryReceiver.generated.h"

UCLASS()
class UFileTelemetryReceiver : public UObject, public ITelemetryReceiver
{
	GENERATED_BODY()

public:
	// Set by UTelemetrySubsystem from UTelemetrySettings before StartReceiving, so this class doesn't depend on the settings.
	// Relative to the project folder.
	FString TripFileRelativePath;

	// 1 = real time. Read on every poll, so it can change during playback.
	float PlaybackSpeedMultiplier = 1.f;

	//~ ITelemetryReceiver
	virtual bool StartReceiving() override;
	virtual void StopReceiving() override;
	virtual void PollNewTelemetrySamples(float DeltaSeconds, TArray<FVehicleTelemetry>& OutNewTelemetrySamples) override;
	virtual FString GetReceiverDisplayName() const override;
	virtual FTelemetryConnectionStatus GetConnectionStatus() const override;
	// A recorded trip can't change what already happened: every command is refused here, before it is sent anywhere.
	virtual bool SendVehicleCommand(const FVehicleCommand& VehicleCommand, FString& OutFailureReason) override;
	virtual void PollVehicleCommandAcks(TArray<FVehicleCommandAck>& OutVehicleCommandAcks) override {}

private:
	// Reads, checks and converts the trip file into LoadedRecordedTrip. Logs the reason and returns false on any problem.
	bool LoadRecordedTripFile();

	FRecordedTripFile LoadedRecordedTrip;

	// Trip time reached so far in the current loop, seconds. Compared against each frame's SampleTimeS.
	double PlaybackElapsedSeconds = 0.0;

	// First frame of LoadedRecordedTrip.Frames not handed out yet in the current loop.
	int32 NextFrameIndexToDeliver = 0;

	bool bIsReceiving = false;
};