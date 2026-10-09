// Commands going down to the vehicle and the relay's answers (docs/SPEC.md §2.5, Twin completion T1).

#pragma once

#include "CoreMinimal.h"
#include "VehicleCommand.generated.h"

// Command names as they go over the wire. One for now.
namespace VehicleCommandNames
{
	// Engine protection derate: rpm capped at 2500, speed at 50 km/h, until disabled or the trip restarts.
	inline const TCHAR* EngineDerate = TEXT("engineDerate");
}

// One command for the vehicle. Written to JSON by hand in the WebSocket receiver (not FJsonObjectConverter), because the converter
// would send bEnabled as "bEnabled" instead of "enabled".
struct FVehicleCommand
{
	// Rises by one per command; the relay's ack repeats it.
	int32 CommandId = 0;

	// One of VehicleCommandNames.
	FString CommandName;

	bool bEnabled = false;
};

// The relay's answer to one command. Property names match the JSON keys (first letter lowered), so FJsonObjectConverter fills it.
// "applied" only means accepted: the effect shows up later in telemetry as FVehicleTelemetry::DriveMode (SPEC.md §2.5).
USTRUCT()
struct FVehicleCommandAck
{
	GENERATED_BODY()

	// -1 when the relay couldn't read the command's id ("bad command message").
	UPROPERTY()
	int32 CommandId = -1;

	// "applied" or "rejected".
	UPROPERTY()
	FString Status;

	// Why it was rejected; empty when applied.
	UPROPERTY()
	FString Reason;

	// First frame produced under the new setting; -1 when rejected.
	UPROPERTY()
	int64 AppliedAtSeq = -1;

	bool WasApplied() const { return Status == TEXT("applied"); }
};