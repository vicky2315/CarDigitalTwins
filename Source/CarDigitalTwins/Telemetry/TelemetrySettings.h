// Project Settings > Game > Vehicle Telemetry. Saved to Config/DefaultGame.ini so every machine plays the same trip.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TelemetrySettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Vehicle Telemetry"))
class UTelemetrySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Recorded trip the file receiver plays, relative to the project folder (Data/ is outside Content/, SPEC.md §2.2).
	UPROPERTY(config, EditAnywhere, Category = "File Receiver")
	FString TripFileRelativePath = TEXT("Data/Trips/trip_sample.json");

	// 1 = real time. Higher plays the trip faster, e.g. 10 to check the 190 s loop in 19 s. Doesn't change the samples themselves.
	UPROPERTY(config, EditAnywhere, Category = "File Receiver", meta = (ClampMin = "0.1", ClampMax = "50"))
	float PlaybackSpeedMultiplier = 1.f;
};