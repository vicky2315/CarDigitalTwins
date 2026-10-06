#include "Telemetry/FileTelemetryReceiver.h"

#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTelemetry, Log, All);

bool UFileTelemetryReceiver::StartReceiving()
{
	StopReceiving();

	if (!LoadRecordedTripFile())
	{
		return false;
	}

	PlaybackElapsedSeconds = 0.0;
	NextFrameIndexToDeliver = 0;
	bIsReceiving = true;
	return true;
}

void UFileTelemetryReceiver::StopReceiving()
{
	bIsReceiving = false;
}

void UFileTelemetryReceiver::PollNewTelemetrySamples(float DeltaSeconds, TArray<FVehicleTelemetry>& OutNewTelemetrySamples)
{
	if (!bIsReceiving)
	{
		return;
	}

	const TArray<FVehicleTelemetry>& RecordedFrames = LoadedRecordedTrip.Frames;
	const double LoopDurationSeconds = LoadedRecordedTrip.GetLoopDurationSeconds();

	PlaybackElapsedSeconds += DeltaSeconds * FMath::Max(PlaybackSpeedMultiplier, 0.f);

	// Hand out every frame that is due, in order. After a hitch (or at a high speed multiplier) that is several frames, possibly
	// across one or more loop wraps; none are skipped, so Seq has no false gaps. LoopDurationSeconds > 0 is checked at load.
	while (true)
	{
		while (NextFrameIndexToDeliver < RecordedFrames.Num()
			&& RecordedFrames[NextFrameIndexToDeliver].SampleTimeS <= PlaybackElapsedSeconds)
		{
			OutNewTelemetrySamples.Add(RecordedFrames[NextFrameIndexToDeliver]);
			++NextFrameIndexToDeliver;
		}

		const bool bAllFramesOfThisLoopDelivered = NextFrameIndexToDeliver >= RecordedFrames.Num();
		if (!bAllFramesOfThisLoopDelivered || PlaybackElapsedSeconds < LoopDurationSeconds)
		{
			break;
		}

		// Wrap: Seq and SampleTimeS start again from frame 0, which receivers downstream treat as a loop, not an error (SPEC.md §2.3).
		PlaybackElapsedSeconds -= LoopDurationSeconds;
		NextFrameIndexToDeliver = 0;
		UE_LOG(LogVehicleTelemetry, Log, TEXT("%s: trip looped, starting again from frame 0."), *GetReceiverDisplayName());
	}
}

FString UFileTelemetryReceiver::GetReceiverDisplayName() const
{
	return FString::Printf(TEXT("File %s"), *FPaths::GetCleanFilename(TripFileRelativePath));
}

bool UFileTelemetryReceiver::LoadRecordedTripFile()
{
	const FString TripFileFullPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TripFileRelativePath));

	FString TripFileJsonText;
	if (!FFileHelper::LoadFileToString(TripFileJsonText, *TripFileFullPath))
	{
		UE_LOG(LogVehicleTelemetry, Error, TEXT("%s: can't read %s. Check TripFileRelativePath in Project Settings > Vehicle Telemetry."),
			*GetReceiverDisplayName(), *TripFileFullPath);
		return false;
	}

	TSharedPtr<FJsonObject> TripJsonObject;
	const TSharedRef<TJsonReader<>> TripJsonReader = TJsonReaderFactory<>::Create(TripFileJsonText);
	if (!FJsonSerializer::Deserialize(TripJsonReader, TripJsonObject) || !TripJsonObject.IsValid())
	{
		UE_LOG(LogVehicleTelemetry, Error, TEXT("%s: %s is not valid JSON: %s"), *GetReceiverDisplayName(), *TripFileFullPath,
			*TripJsonReader->GetErrorMessage());
		return false;
	}

	// Check the version before converting the frames: a different schema could convert without errors into wrong values (SPEC.md §2.4).
	int32 FileSchemaVersion = 0;
	if (!TripJsonObject->TryGetNumberField(TEXT("schemaVersion"), FileSchemaVersion) || FileSchemaVersion != VehicleTelemetrySchemaVersion)
	{
		UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s: schemaVersion %d in %s, expected %d. Trip rejected; regenerate it with Tools/TripGenerator."),
			*GetReceiverDisplayName(), FileSchemaVersion, *TripFileFullPath, VehicleTelemetrySchemaVersion);
		return false;
	}

	LoadedRecordedTrip = FRecordedTripFile();
	if (!FJsonObjectConverter::JsonObjectToUStruct(TripJsonObject.ToSharedRef(), &LoadedRecordedTrip))
	{
		UE_LOG(LogVehicleTelemetry, Error, TEXT("%s: %s doesn't match the trip format (SPEC.md §2.2)."), *GetReceiverDisplayName(), *TripFileFullPath);
		return false;
	}

	const TArray<FVehicleTelemetry>& RecordedFrames = LoadedRecordedTrip.Frames;
	if (RecordedFrames.IsEmpty() || LoadedRecordedTrip.RateHz <= 0.f)
	{
		UE_LOG(LogVehicleTelemetry, Error, TEXT("%s: %s has %d frames at %.2f Hz; needs at least one frame and a rateHz above 0."),
			*GetReceiverDisplayName(), *TripFileFullPath, RecordedFrames.Num(), LoadedRecordedTrip.RateHz);
		return false;
	}

	// Playback walks the frames in order, so a frame that goes back in time would hold up every frame after it until the loop wraps.
	for (int32 FrameIndex = 1; FrameIndex < RecordedFrames.Num(); ++FrameIndex)
	{
		if (RecordedFrames[FrameIndex].SampleTimeS <= RecordedFrames[FrameIndex - 1].SampleTimeS)
		{
			UE_LOG(LogVehicleTelemetry, Error, TEXT("%s: frame %d (sampleTimeS %.3f) is not after frame %d (%.3f) in %s."),
				*GetReceiverDisplayName(), FrameIndex, RecordedFrames[FrameIndex].SampleTimeS, FrameIndex - 1,
				RecordedFrames[FrameIndex - 1].SampleTimeS, *TripFileFullPath);
			return false;
		}
	}

	UE_LOG(LogVehicleTelemetry, Log, TEXT("%s: loaded trip %s, %d frames at %.1f Hz (%.1f s loop) from %s."), *GetReceiverDisplayName(),
		*LoadedRecordedTrip.VehicleId, RecordedFrames.Num(), LoadedRecordedTrip.RateHz, LoadedRecordedTrip.GetLoopDurationSeconds(),
		*TripFileFullPath);

	// Missing or misspelt JSON keys don't fail the conversion, they leave the field at 0. Showing the first frame makes that visible.
	const FVehicleTelemetry& FirstFrame = RecordedFrames[0];
	UE_LOG(LogVehicleTelemetry, Log, TEXT("%s: first frame: %.1f km/h, %.0f rpm, gear %d, coolant %.1f C, fuel %.0f %%, battery %.2f V, tyres %.0f/%.0f/%.0f/%.0f kPa, openings %d."),
		*GetReceiverDisplayName(), FirstFrame.SpeedKmh, FirstFrame.EngineRpm, FirstFrame.Gear, FirstFrame.CoolantTempC, FirstFrame.FuelPct,
		FirstFrame.BatteryV, FirstFrame.TyreKpa.FL, FirstFrame.TyreKpa.FR, FirstFrame.TyreKpa.RL, FirstFrame.TyreKpa.RR, FirstFrame.Openings);

	return true;
}