#include "Telemetry/VehicleTelemetryViewModel.h"

namespace VehicleTelemetryTolerance
{
	// Smallest change worth a widget update; well below what the dashboard displays.
	constexpr float SpeedKmh = 0.05f;
	constexpr float EngineRpm = 1.f;
	constexpr float Pct = 0.05f;
	constexpr float SteerDeg = 0.05f;
	constexpr double OdometerKm = 0.001;  // 1 m
	constexpr float TempC = 0.05f;
	constexpr float Volts = 0.005f;
	constexpr float Kpa = 0.1f;
}

void UVehicleTelemetryViewModel::ApplySample(const FVehicleTelemetry& Sample, EVehicleStatus InStatus)
{
	namespace Tol = VehicleTelemetryTolerance;
	using EField = EVehicleTelemetryField;

	SetField(EField::SpeedKmh, SpeedKmh, Sample.SpeedKmh, Tol::SpeedKmh);
	SetField(EField::EngineRpm, EngineRpm, Sample.EngineRpm, Tol::EngineRpm);
	SetField(EField::Gear, Gear, Sample.Gear);
	SetField(EField::ThrottlePct, ThrottlePct, Sample.ThrottlePct, Tol::Pct);
	SetField(EField::BrakePct, BrakePct, Sample.BrakePct, Tol::Pct);
	SetField(EField::SteerDeg, SteerDeg, Sample.SteerDeg, Tol::SteerDeg);
	SetField(EField::OdometerKm, OdometerKm, Sample.OdometerKm, Tol::OdometerKm);
	SetField(EField::CoolantTempC, CoolantTempC, Sample.CoolantTempC, Tol::TempC);
	SetField(EField::FuelPct, FuelPct, Sample.FuelPct, Tol::Pct);
	SetField(EField::BatteryV, BatteryV, Sample.BatteryV, Tol::Volts);
	SetField(EField::TyreKpaFL, TyreKpa.FL, Sample.TyreKpa.FL, Tol::Kpa);
	SetField(EField::TyreKpaFR, TyreKpa.FR, Sample.TyreKpa.FR, Tol::Kpa);
	SetField(EField::TyreKpaRL, TyreKpa.RL, Sample.TyreKpa.RL, Tol::Kpa);
	SetField(EField::TyreKpaRR, TyreKpa.RR, Sample.TyreKpa.RR, Tol::Kpa);
	SetField(EField::Openings, Openings, Sample.Openings);
	SetField(EField::Status, Status, InStatus);
}
