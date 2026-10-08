// Source of telemetry samples for UTelemetrySubsystem. Day 7: UFileTelemetryReceiver (recorded trip).
// Day 10: UWebSocketTelemetryReceiver (live relay). Data flow: docs/SPEC.md §5–6.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleTelemetry.h"
#include "TelemetryReceiver.generated.h"

// Where a receiver is in its connection life (SPEC.md §4). The file receiver is only ever Idle or Live.
UENUM(BlueprintType)
enum class ETelemetryConnectionState : uint8
{
	// Not started, or stopped.
	Idle,
	// Socket opening; no answer from the relay yet.
	Connecting,
	// Connected and samples arriving.
	Live,
	// Still connected, but no sample for longer than the stale threshold (relay paused, network stalled).
	Stale,
	// Closed or failed; waiting for the next reconnect attempt.
	Disconnected,
};

// Health of the telemetry link, for logs now and the dashboard later (Day 11).
USTRUCT(BlueprintType)
struct FTelemetryConnectionStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Connection")
	ETelemetryConnectionState ConnectionState = ETelemetryConnectionState::Idle;

	// Messages lost while connected: gaps in Seq. A trip loop (Seq going back) is not a gap, and the time between two connections
	// isn't counted either.
	UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Connection")
	int64 DroppedMessageCount = 0;

	// Relay send → UE receive in milliseconds (latency part (a), SPEC.md §2.3), over the current connection. 0 for the file receiver.
	UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Connection")
	float LatestReceiveLatencyMs = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Connection")
	float AverageReceiveLatencyMs = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Connection")
	float MaxReceiveLatencyMs = 0.f;

	// Reconnect attempts since StartReceiving.
	UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Connection")
	int32 ReconnectAttemptCount = 0;
};

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UTelemetryReceiver : public UInterface
{
	GENERATED_BODY()
};

// Implemented in C++ only. The subsystem pulls samples once per tick (PollNewTelemetrySamples) instead of the receiver pushing
// them, so both transports deliver on the game thread at the same point in the frame: the file receiver advances its playback
// clock there, the WebSocket receiver empties the queue its socket callbacks filled.
class ITelemetryReceiver
{
	GENERATED_BODY()

public:
	// Opens the source (loads the trip file, connects the socket). Returns false and logs the reason if it can't start,
	// e.g. a missing file or a schemaVersion mismatch (SPEC.md §2.4); the receiver then delivers nothing.
	virtual bool StartReceiving() = 0;

	// Closes the source. Safe to call when not started. StartReceiving may be called again afterwards.
	virtual void StopReceiving() = 0;

	// Appends every sample that became available since the last call, oldest first, to OutNewTelemetrySamples.
	// Appends nothing when no sample is due. Never skips samples on purpose: gaps in Seq mean dropped messages (Day 10).
	virtual void PollNewTelemetrySamples(float DeltaSeconds, TArray<FVehicleTelemetry>& OutNewTelemetrySamples) = 0;

	// Short name for logs, e.g. "File trip_sample.json" or "WebSocket <relay url>".
	virtual FString GetReceiverDisplayName() const = 0;

	// Connection state, dropped messages and latency (SPEC.md §4).
	virtual FTelemetryConnectionStatus GetConnectionStatus() const = 0;
};