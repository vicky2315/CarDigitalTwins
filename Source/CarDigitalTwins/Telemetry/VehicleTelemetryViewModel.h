// Vehicle data for the dashboard. Read-only for widgets; filled from telemetry samples. See docs/SPEC.md §5.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ViewModelBase.h"
#include "Telemetry/VehicleTelemetry.h"
#include "VehicleTelemetryViewModel.generated.h"

UENUM(BlueprintType)
enum class EVehicleTelemetryField : uint8
{
	SpeedKmh,
	EngineRpm,
	Gear,
	ThrottlePct,
	BrakePct,
	SteerDeg,
	OdometerKm,
	CoolantTempC,
	FuelPct,
	BatteryV,
	TyreKpaFL,
	TyreKpaFR,
	TyreKpaRL,
	TyreKpaRR,
	Openings,
	Status,

	Count UMETA(Hidden)
};

UCLASS(BlueprintType)
class CARDIGITALTWINS_API UVehicleTelemetryViewModel : public UViewModelBase
{
	GENERATED_BODY()

public:
	// Copies a sample into the fields; only values that changed (beyond their tolerance) are marked dirty.
	// Public so tests and debug commands can feed samples without a receiver.
	void ApplySample(const FVehicleTelemetry& Sample, EVehicleStatus Status);

	virtual int32 GetNumFields() const override { return static_cast<int32>(EVehicleTelemetryField::Count); }

	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetSpeedKmh() const { return SpeedKmh; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetEngineRpm() const { return EngineRpm; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") int32 GetGear() const { return Gear; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetThrottlePct() const { return ThrottlePct; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetBrakePct() const { return BrakePct; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetSteerDeg() const { return SteerDeg; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") double GetOdometerKm() const { return OdometerKm; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetCoolantTempC() const { return CoolantTempC; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetFuelPct() const { return FuelPct; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") float GetBatteryV() const { return BatteryV; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") FTyrePressuresKpa GetTyreKpa() const { return TyreKpa; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") int32 GetOpenings() const { return Openings; }
	UFUNCTION(BlueprintPure, Category = "Telemetry") EVehicleStatus GetStatus() const { return Status; }

	bool IsOpen(EVehicleOpening Opening) const { return (Openings & static_cast<int32>(Opening)) != 0; }

private:
	float SpeedKmh = 0.f;
	float EngineRpm = 0.f;
	int32 Gear = 0;
	float ThrottlePct = 0.f;
	float BrakePct = 0.f;
	float SteerDeg = 0.f;
	double OdometerKm = 0.0;
	float CoolantTempC = 0.f;
	float FuelPct = 0.f;
	float BatteryV = 0.f;
	FTyrePressuresKpa TyreKpa;
	int32 Openings = 0;
	EVehicleStatus Status = EVehicleStatus::Normal;
};
