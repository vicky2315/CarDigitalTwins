// A recorded trip as stored in trip.json (written by Tools/TripGenerator, read by UFileTelemetryReceiver). Format: docs/SPEC.md §2.2.

#pragma once

#include "CoreMinimal.h"
#include "VehicleTelemetry.h"
#include "RecordedTripFile.generated.h"

// Whole trip file: header plus fixed-rate frames. Property names match the JSON keys (first letter lowered), so one
// FJsonObjectConverter::JsonObjectStringToUStruct call fills it. Keep them in sync with the generator, not renamed for style.
USTRUCT()
struct FRecordedTripFile
{
	GENERATED_BODY()

	// Must equal VehicleTelemetrySchemaVersion, otherwise the trip is rejected (SPEC.md §2.4).
	UPROPERTY()
	int32 SchemaVersion = 0;

	UPROPERTY()
	FString VehicleId;

	// Frames per second the trip was recorded at. Frame N is at SampleTimeS = N / RateHz.
	UPROPERTY()
	float RateHz = 0.f;

	// Random seed the generator used; the same seed gives a byte-identical file. Informational only.
	UPROPERTY()
	int32 Seed = 0;

	// One telemetry sample per frame, in SampleTimeS order.
	UPROPERTY()
	TArray<FVehicleTelemetry> Frames;

	// Length of one playback loop: the last frame is shown for one frame period before wrapping to frame 0.
	double GetLoopDurationSeconds() const
	{
		return RateHz > 0.f ? Frames.Num() / static_cast<double>(RateHz) : 0.0;
	}
};