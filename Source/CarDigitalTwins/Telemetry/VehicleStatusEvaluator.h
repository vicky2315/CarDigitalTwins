// Derives vehicle health from telemetry samples using the thresholds and hysteresis in docs/SPEC.md §3. Plain C++ (no UObject),
// so it can be unit-tested without a world.

#pragma once

#include "CoreMinimal.h"
#include "VehicleTelemetry.h"
#include "VehicleStatusEvaluator.generated.h"

// Status of every SPEC.md §3 row for one sample, plus the overall status (the worst row).
USTRUCT(BlueprintType)
struct FVehicleStatusReport
{
	GENERATED_BODY()

	// Worst of the rows below. Drives the car's status colour.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Status")
	EVehicleStatus OverallStatus = EVehicleStatus::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Status")
	EVehicleStatus CoolantTemperatureStatus = EVehicleStatus::Normal;

	// Worst of the four tyres (too low or too high).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Status")
	EVehicleStatus TyrePressureStatus = EVehicleStatus::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Status")
	EVehicleStatus FuelLevelStatus = EVehicleStatus::Normal;

	// Always Normal while the engine is off (rpm 0): a resting battery reads ~12.6 V, which isn't a fault.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Status")
	EVehicleStatus BatteryVoltageStatus = EVehicleStatus::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle Status")
	EVehicleStatus EngineRpmStatus = EVehicleStatus::Normal;
};

// Keeps the previous status of each threshold rule between samples: hysteresis means a status only clears a few units past
// the threshold that set it (SPEC.md §3), so the same value can be Warning or Normal depending on where it came from.
class FVehicleStatusEvaluator
{
public:
	// Evaluates one received sample and remembers the result for the next call. Feed real samples in order, never interpolated
	// values: a blend between two samples could cross a threshold that no actual sample did.
	FVehicleStatusReport EvaluateTelemetrySample(const FVehicleTelemetry& TelemetrySample);

	// Forgets the hysteresis memory, e.g. when the telemetry source changes. Not needed on a trip loop: the memory still
	// reflects the last real values.
	void ResetStatusMemory();

	// The worse of two statuses (Normal < Warning < Critical).
	static EVehicleStatus WorseStatus(EVehicleStatus FirstStatus, EVehicleStatus SecondStatus);

private:
	// One status per rule (a signal can have a low-side and a high-side rule), from the previous sample.
	EVehicleStatus PreviousCoolantTooHotStatus = EVehicleStatus::Normal;
	EVehicleStatus PreviousTyreTooLowStatus = EVehicleStatus::Normal;
	EVehicleStatus PreviousTyreTooHighStatus = EVehicleStatus::Normal;
	EVehicleStatus PreviousFuelTooLowStatus = EVehicleStatus::Normal;
	EVehicleStatus PreviousBatteryTooLowStatus = EVehicleStatus::Normal;
	EVehicleStatus PreviousBatteryTooHighStatus = EVehicleStatus::Normal;
	EVehicleStatus PreviousEngineRpmTooHighStatus = EVehicleStatus::Normal;
};
