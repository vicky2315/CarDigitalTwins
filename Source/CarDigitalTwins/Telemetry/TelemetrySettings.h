// Project Settings > Game > Vehicle Telemetry. Saved to Config/DefaultGame.ini so every machine plays the same trip.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TelemetrySettings.generated.h"

// Which receiver UTelemetrySubsystem creates. Read once when the game starts (next PIE after a change).
UENUM()
enum class ETelemetrySource : uint8
{
	File UMETA(DisplayName = "Recorded trip file"),
	WebSocket UMETA(DisplayName = "WebSocket relay (Tools/Relay/relay.py)"),
};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Vehicle Telemetry"))
class UTelemetrySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// File = the recorded trip below, no setup needed. WebSocket = live stream from the relay, which must be running (SPEC.md §2.3).
	UPROPERTY(config, EditAnywhere, Category = "Source")
	ETelemetrySource TelemetrySource = ETelemetrySource::File;

	// Recorded trip the file receiver plays, relative to the project folder (Data/ is outside Content/, SPEC.md §2.2).
	UPROPERTY(config, EditAnywhere, Category = "File Receiver")
	FString TripFileRelativePath = TEXT("Data/Trips/trip_sample.json");

	// 1 = real time. Higher plays the trip faster, e.g. 10 to check the 190 s loop in 19 s. Doesn't change the samples themselves.
	UPROPERTY(config, EditAnywhere, Category = "File Receiver", meta = (ClampMin = "0.1", ClampMax = "50"))
	float PlaybackSpeedMultiplier = 1.f;

	// Relay address. The relay's own --rate sets the playback speed for this source.
	UPROPERTY(config, EditAnywhere, Category = "WebSocket Receiver")
	FString RelayUrl = TEXT("ws://127.0.0.1:8765");

	// Connected but no sample for this long → Stale (SPEC.md §4). Never shorter than 3 message intervals from the relay's hello.
	UPROPERTY(config, EditAnywhere, Category = "WebSocket Receiver", meta = (ClampMin = "0.1", Units = "s"))
	float StaleAfterSeconds = 1.f;

	// No answer from the relay within this time → treat the attempt as failed and back off.
	UPROPERTY(config, EditAnywhere, Category = "WebSocket Receiver", meta = (ClampMin = "0.5", Units = "s"))
	float ConnectTimeoutSeconds = 5.f;

	// Wait before the first reconnect; doubles after every failed attempt up to the maximum (SPEC.md §4).
	UPROPERTY(config, EditAnywhere, Category = "WebSocket Receiver", meta = (ClampMin = "0.1", Units = "s"))
	float ReconnectInitialDelaySeconds = 0.5f;

	UPROPERTY(config, EditAnywhere, Category = "WebSocket Receiver", meta = (ClampMin = "0.1", Units = "s"))
	float ReconnectMaxDelaySeconds = 10.f;
};
