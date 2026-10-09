#include "MVVM/VehicleTelemetryViewModel.h"

#include "Engine/GameInstance.h"
#include "Telemetry/TelemetrySubsystem.h"

// Change tolerances: about half of the finest step a widget will show, so a value is marked changed when its displayed digits can
// change, and sensor noise below that doesn't redraw anything. Revisit when SPEC.md §8 fixes the displayed precision.
namespace VehicleTelemetryChangeTolerances
{
	static constexpr float SpeedKmh = 0.05f;           // shown to 0.1 km/h
	static constexpr float EngineRpm = 5.f;            // shown to 10 rpm
	static constexpr float PedalPct = 0.5f;            // throttle and brake, shown to 1 %
	static constexpr float SteerDeg = 0.5f;            // shown to 1°
	static constexpr double OdometerKm = 0.05;         // shown to 0.1 km
	static constexpr float CoolantTempC = 0.05f;       // shown to 0.1 °C: thresholds are 105 / 115, so tenths matter near them
	static constexpr float FuelPct = 0.5f;             // shown to 1 %
	static constexpr float BatteryV = 0.005f;          // shown to 0.01 V
	static constexpr float TyrePressureKpa = 0.5f;     // shown to 1 kPa
	static constexpr double SampleTimeS = 0.05;        // shown to 0.1 s
}

void UVehicleTelemetryViewModel::InitializeViewModel(UGameInstance& OwningGameInstance)
{
	UTelemetrySubsystem* TelemetrySubsystem = OwningGameInstance.GetSubsystem<UTelemetrySubsystem>();
	if (!TelemetrySubsystem)
	{
		return;
	}
	SubscribedTelemetrySubsystem = TelemetrySubsystem;
	TelemetryUpdatedDelegateHandle = TelemetrySubsystem->OnTelemetryUpdated.AddUObject(this, &UVehicleTelemetryViewModel::HandleTelemetryUpdated);

	// Created after data started arriving (a widget opened later): start from the latest sample instead of waiting for the next one.
	if (TelemetrySubsystem->HasReceivedAnyTelemetrySample())
	{
		ApplyTelemetrySample(TelemetrySubsystem->GetLatestTelemetrySample(), TelemetrySubsystem->GetCurrentVehicleStatusReport());
	}
}

void UVehicleTelemetryViewModel::DeinitializeViewModel()
{
	if (UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		TelemetrySubsystem->OnTelemetryUpdated.Remove(TelemetryUpdatedDelegateHandle);
	}
	SubscribedTelemetrySubsystem.Reset();
}

void UVehicleTelemetryViewModel::HandleTelemetryUpdated(const FVehicleTelemetry& NewTelemetrySample)
{
	// The subsystem updates its status report before broadcasting, so it belongs to this sample.
	if (const UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		ApplyTelemetrySample(NewTelemetrySample, TelemetrySubsystem->GetCurrentVehicleStatusReport());
	}
}

void UVehicleTelemetryViewModel::ApplyTelemetrySample(const FVehicleTelemetry& TelemetrySample, const FVehicleStatusReport& VehicleStatusReport)
{
	using TelemetryField = EVehicleTelemetryViewModelField;
	namespace ChangeTolerance = VehicleTelemetryChangeTolerances;

	SetField(TelemetryField::HasReceivedTelemetry, bHasReceivedTelemetry, true);
	SetField(TelemetryField::SampleTimeS, SampleTimeS, TelemetrySample.SampleTimeS, ChangeTolerance::SampleTimeS);

	SetField(TelemetryField::SpeedKmh, SpeedKmh, TelemetrySample.SpeedKmh, ChangeTolerance::SpeedKmh);
	SetField(TelemetryField::EngineRpm, EngineRpm, TelemetrySample.EngineRpm, ChangeTolerance::EngineRpm);
	SetField(TelemetryField::Gear, Gear, TelemetrySample.Gear);
	SetField(TelemetryField::ThrottlePct, ThrottlePct, TelemetrySample.ThrottlePct, ChangeTolerance::PedalPct);
	SetField(TelemetryField::BrakePct, BrakePct, TelemetrySample.BrakePct, ChangeTolerance::PedalPct);
	SetField(TelemetryField::SteerDeg, SteerDeg, TelemetrySample.SteerDeg, ChangeTolerance::SteerDeg);
	SetField(TelemetryField::OdometerKm, OdometerKm, TelemetrySample.OdometerKm, ChangeTolerance::OdometerKm);
	SetField(TelemetryField::DriveMode, DriveMode, TelemetrySample.DriveMode);

	SetField(TelemetryField::CoolantTempC, CoolantTempC, TelemetrySample.CoolantTempC, ChangeTolerance::CoolantTempC);
	SetField(TelemetryField::FuelPct, FuelPct, TelemetrySample.FuelPct, ChangeTolerance::FuelPct);
	SetField(TelemetryField::BatteryV, BatteryV, TelemetrySample.BatteryV, ChangeTolerance::BatteryV);
	SetField(TelemetryField::TyrePressureFrontLeftKpa, TyrePressureFrontLeftKpa, TelemetrySample.TyreKpa.FL, ChangeTolerance::TyrePressureKpa);
	SetField(TelemetryField::TyrePressureFrontRightKpa, TyrePressureFrontRightKpa, TelemetrySample.TyreKpa.FR, ChangeTolerance::TyrePressureKpa);
	SetField(TelemetryField::TyrePressureRearLeftKpa, TyrePressureRearLeftKpa, TelemetrySample.TyreKpa.RL, ChangeTolerance::TyrePressureKpa);
	SetField(TelemetryField::TyrePressureRearRightKpa, TyrePressureRearRightKpa, TelemetrySample.TyreKpa.RR, ChangeTolerance::TyrePressureKpa);

	SetField(TelemetryField::Openings, Openings, TelemetrySample.Openings);

	SetField(TelemetryField::OverallStatus, OverallStatus, VehicleStatusReport.OverallStatus);
	SetField(TelemetryField::CoolantTemperatureStatus, CoolantTemperatureStatus, VehicleStatusReport.CoolantTemperatureStatus);
	SetField(TelemetryField::TyrePressureStatus, TyrePressureStatus, VehicleStatusReport.TyrePressureStatus);
	SetField(TelemetryField::FuelLevelStatus, FuelLevelStatus, VehicleStatusReport.FuelLevelStatus);
	SetField(TelemetryField::BatteryVoltageStatus, BatteryVoltageStatus, VehicleStatusReport.BatteryVoltageStatus);
	SetField(TelemetryField::EngineRpmStatus, EngineRpmStatus, VehicleStatusReport.EngineRpmStatus);
}