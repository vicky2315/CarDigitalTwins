// Vehicle data for the dashboards: every telemetry value (docs/SPEC.md §2.1) and its derived status (§3), as plain numbers and enums.
// Read-only for widgets; formatting (units, decimals, colours) happens in the widgets (SPEC.md §5). Holds no layout assumptions, so
// the remote ops dashboard and the in-car HMI view (Phase H) can both use it.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ViewModelBase.h"
#include "Telemetry/VehicleStatusEvaluator.h"
#include "Telemetry/VehicleTelemetry.h"
#include "VehicleTelemetryViewModel.generated.h"

// One bit per value in the change mask. Order is free, but values must run 0, 1, 2, ... with Count last.
UENUM(BlueprintType)
enum class EVehicleTelemetryViewModelField : uint8
{
	HasReceivedTelemetry,
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
	TyrePressureFrontLeftKpa,
	TyrePressureFrontRightKpa,
	TyrePressureRearLeftKpa,
	TyrePressureRearRightKpa,
	Openings,
	DriveMode,
	OverallStatus,
	CoolantTemperatureStatus,
	TyrePressureStatus,
	FuelLevelStatus,
	BatteryVoltageStatus,
	EngineRpmStatus,
	Count UMETA(Hidden),
};
static_assert(static_cast<int32>(EVehicleTelemetryViewModelField::Count) <= FViewModelFieldMask::MaxFieldCount,
	"EVehicleTelemetryViewModelField has more values than the field mask has bits");

UCLASS()
class UVehicleTelemetryViewModel : public UViewModelBase
{
	GENERATED_BODY()

public:
	// Takes one received sample and its status. Only values that really changed (past their tolerance) are marked for the next flush.
	// Called by the ViewModel subsystem for every sample from UTelemetrySubsystem (step 8); by hand in tests.
	void ApplyTelemetrySample(const FVehicleTelemetry& TelemetrySample, const FVehicleStatusReport& VehicleStatusReport);

	//~ UViewModelBase
	virtual int32 GetFieldCount() const override { return static_cast<int32>(EVehicleTelemetryViewModelField::Count); }

	// False until the first sample: widgets show "no data yet" instead of zeros.
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry")
	bool HasReceivedTelemetry() const { return bHasReceivedTelemetry; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	float GetSpeedKmh() const { return SpeedKmh; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	float GetEngineRpm() const { return EngineRpm; }

	// -1 = reverse, 0 = neutral, 1..6 = forward gears.
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	int32 GetGear() const { return Gear; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	float GetThrottlePct() const { return ThrottlePct; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	float GetBrakePct() const { return BrakePct; }

	// Positive = turning right.
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	float GetSteerDeg() const { return SteerDeg; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	double GetOdometerKm() const { return OdometerKm; }

	// What the vehicle reports it is doing, e.g. EngineDerate after a command (SPEC.md §2.5).
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Motion")
	EVehicleDriveMode GetDriveMode() const { return DriveMode; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetCoolantTempC() const { return CoolantTempC; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetFuelPct() const { return FuelPct; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetBatteryV() const { return BatteryV; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetTyrePressureFrontLeftKpa() const { return TyrePressureFrontLeftKpa; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetTyrePressureFrontRightKpa() const { return TyrePressureFrontRightKpa; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetTyrePressureRearLeftKpa() const { return TyrePressureRearLeftKpa; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Health")
	float GetTyrePressureRearRightKpa() const { return TyrePressureRearRightKpa; }

	// EVehicleOpening bitmask; a set bit = open.
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Body")
	int32 GetOpenings() const { return Openings; }

	bool IsOpen(EVehicleOpening Opening) const { return (Openings & static_cast<int32>(Opening)) != 0; }

	// Worst of the per-signal statuses below (SPEC.md §3).
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Status")
	EVehicleStatus GetOverallStatus() const { return OverallStatus; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Status")
	EVehicleStatus GetCoolantTemperatureStatus() const { return CoolantTemperatureStatus; }

	// Worst of the four tyres.
	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Status")
	EVehicleStatus GetTyrePressureStatus() const { return TyrePressureStatus; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Status")
	EVehicleStatus GetFuelLevelStatus() const { return FuelLevelStatus; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Status")
	EVehicleStatus GetBatteryVoltageStatus() const { return BatteryVoltageStatus; }

	UFUNCTION(BlueprintPure, Category = "Vehicle Telemetry|Status")
	EVehicleStatus GetEngineRpmStatus() const { return EngineRpmStatus; }

private:
	bool bHasReceivedTelemetry = false;

	float SpeedKmh = 0.f;
	float EngineRpm = 0.f;
	int32 Gear = 0;
	float ThrottlePct = 0.f;
	float BrakePct = 0.f;
	float SteerDeg = 0.f;
	double OdometerKm = 0.0;
	EVehicleDriveMode DriveMode = EVehicleDriveMode::Normal;

	float CoolantTempC = 0.f;
	float FuelPct = 0.f;
	float BatteryV = 0.f;
	float TyrePressureFrontLeftKpa = 0.f;
	float TyrePressureFrontRightKpa = 0.f;
	float TyrePressureRearLeftKpa = 0.f;
	float TyrePressureRearRightKpa = 0.f;

	int32 Openings = 0;

	EVehicleStatus OverallStatus = EVehicleStatus::Normal;
	EVehicleStatus CoolantTemperatureStatus = EVehicleStatus::Normal;
	EVehicleStatus TyrePressureStatus = EVehicleStatus::Normal;
	EVehicleStatus FuelLevelStatus = EVehicleStatus::Normal;
	EVehicleStatus BatteryVoltageStatus = EVehicleStatus::Normal;
	EVehicleStatus EngineRpmStatus = EVehicleStatus::Normal;
};