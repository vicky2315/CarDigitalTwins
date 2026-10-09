// Vehicle telemetry schema v1. Field meanings, units and JSON format: docs/SPEC.md §2.

#pragma once

#include "CoreMinimal.h"
#include "VehicleTelemetry.generated.h"

// Bump only for breaking changes (rename, removal, unit change). Adding optional fields does not bump it.
inline constexpr int32 VehicleTelemetrySchemaVersion = 1;

// Bit flags for FVehicleTelemetry::Openings (JSON: "openings", an int bitmask).
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EVehicleOpening : uint8
{
	None       = 0 UMETA(Hidden),
	DoorFL     = 1 << 0,
	DoorFR     = 1 << 1,
	DoorRL     = 1 << 2,
	DoorRR     = 1 << 3,
	Hood       = 1 << 4,
	Tailgate   = 1 << 5,
};
ENUM_CLASS_FLAGS(EVehicleOpening);

// What the vehicle reports it is doing (JSON: "driveMode", a string matching the value name). Changes when the vehicle obeys a
// command (SPEC.md §2.5); UE shows this, never the command it sent.
UENUM(BlueprintType)
enum class EVehicleDriveMode : uint8
{
	Normal,
	// Engine protection derate: rpm capped at 2500, speed at 50 km/h.
	EngineDerate,
};

// Derived in UE from thresholds (SPEC.md §3); never sent over the wire.
UENUM(BlueprintType)
enum class EVehicleStatus : uint8
{
	Normal,
	Warning,
	Critical,
};

USTRUCT(BlueprintType)
struct FTyrePressuresKpa
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	float FL = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	float FR = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	float RL = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	float RR = 0.f;
};

// One telemetry sample. Property names match the camelCase JSON keys (first letter lowered) so
// FJsonObjectConverter can fill the struct without a hand-written parser.
USTRUCT(BlueprintType)
struct FVehicleTelemetry
{
	GENERATED_BODY()

	// Sequence number, +1 per sample. Gaps = dropped messages.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Meta")
	int64 Seq = 0;

	// Sample time since trip start, seconds. Used for interpolation, not wall clock.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Meta")
	double SampleTimeS = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	float SpeedKmh = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	float EngineRpm = 0.f;

	// -1 = reverse, 0 = neutral, 1..6 = forward gears.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	int32 Gear = 0;

	// 0..100
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	float ThrottlePct = 0.f;

	// 0..100
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	float BrakePct = 0.f;

	// Front road-wheel angle, degrees. Positive = turning right (matches UE yaw).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	float SteerDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	double OdometerKm = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Health")
	float CoolantTempC = 0.f;

	// 0..100
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Health")
	float FuelPct = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Health")
	float BatteryV = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Health")
	FTyrePressuresKpa TyreKpa;

	// EVehicleOpening bitmask; a set bit = open.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Body", meta = (Bitmask, BitmaskEnum = "/Script/CarDigitalTwins.EVehicleOpening"))
	int32 Openings = 0;

	// Optional in the JSON: trips recorded before T1 have no "driveMode" and keep Normal. An unknown value fails the conversion, so
	// a new mode needs this enum extended first.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry|Motion")
	EVehicleDriveMode DriveMode = EVehicleDriveMode::Normal;

	bool IsOpen(EVehicleOpening Opening) const
	{
		return (Openings & static_cast<int32>(Opening)) != 0;
	}
};
