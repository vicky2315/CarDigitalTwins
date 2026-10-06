#include "Telemetry/VehicleStatusEvaluator.h"

namespace VehicleStatusRules
{
	// One threshold rule with hysteresis, written so that a higher "severity" value is worse. Low-side rules (tyre too low, fuel
	// too low, battery too low) pass the value and thresholds negated, so one function covers both directions.
	// "Enter" moves the status up; "Stay" is the clear threshold: the status holds while the value is still past it.
	struct FHysteresisRule
	{
		float WarningEnter = 0.f;
		float WarningStay = 0.f;
		bool bHasCritical = true;
		float CriticalEnter = 0.f;
		float CriticalStay = 0.f;

		// SPEC.md §3 mixes ≥ and >; these keep each row exactly as written there.
		bool bEnterAtThreshold = false;		// true: entering at exactly the threshold value (coolant "≥ 105")
		bool bStayAtThreshold = false;		// true: still held at exactly the clear value (coolant clears "< 102", so 102 holds)
	};

	bool IsPast(float SeverityValue, float Threshold, bool bIncludeThreshold)
	{
		return bIncludeThreshold ? SeverityValue >= Threshold : SeverityValue > Threshold;
	}

	EVehicleStatus EvaluateRule(float SeverityValue, EVehicleStatus PreviousStatus, const FHysteresisRule& Rule)
	{
		if (Rule.bHasCritical)
		{
			if (IsPast(SeverityValue, Rule.CriticalEnter, Rule.bEnterAtThreshold))
			{
				return EVehicleStatus::Critical;
			}
			if (PreviousStatus == EVehicleStatus::Critical && IsPast(SeverityValue, Rule.CriticalStay, Rule.bStayAtThreshold))
			{
				return EVehicleStatus::Critical;
			}
		}
		if (IsPast(SeverityValue, Rule.WarningEnter, Rule.bEnterAtThreshold))
		{
			return EVehicleStatus::Warning;
		}
		if (PreviousStatus != EVehicleStatus::Normal && IsPast(SeverityValue, Rule.WarningStay, Rule.bStayAtThreshold))
		{
			return EVehicleStatus::Warning;
		}
		return EVehicleStatus::Normal;
	}

	// SPEC.md §3. The Critical "stay" values use the same gap as the Warning clear column ("Critical → Warning uses the same gap").
	// Low-side rules are negated: e.g. tyre "warning < 180, clear ≥ 185" becomes severity -kPa > -180, held while -kPa > -185.

	// Coolant: warning ≥ 105, critical ≥ 115, warning clears < 102 (gap 3), so critical clears < 112.
	const FHysteresisRule CoolantTooHot{ 105.f, 102.f, true, 115.f, 112.f, /*bEnterAtThreshold*/ true, /*bStayAtThreshold*/ true };

	// Tyre (lowest wheel): warning < 180, critical < 140, warning clears ≥ 185 (gap 5), so critical clears ≥ 145.
	const FHysteresisRule TyreTooLow{ -180.f, -185.f, true, -140.f, -145.f, false, false };

	// Tyre (highest wheel): warning > 300, no critical, clears ≤ 295.
	const FHysteresisRule TyreTooHigh{ 300.f, 295.f, false, 0.f, 0.f, false, false };

	// Fuel: warning < 15, critical < 5, warning clears ≥ 17 (gap 2), so critical clears ≥ 7.
	const FHysteresisRule FuelTooLow{ -15.f, -17.f, true, -5.f, -7.f, false, false };

	// Battery low (engine running): warning < 13.0, critical < 12.0, warning clears ≥ 13.2 (gap 0.2), so critical clears ≥ 12.2.
	const FHysteresisRule BatteryTooLow{ -13.0f, -13.2f, true, -12.0f, -12.2f, false, false };

	// Battery high (engine running): warning > 14.8, critical > 15.5, warning clears ≤ 14.6 (gap 0.2), so critical clears ≤ 15.3.
	const FHysteresisRule BatteryTooHigh{ 14.8f, 14.6f, true, 15.5f, 15.3f, false, false };

	// Engine rpm: warning > 5500, critical > 6200, warning clears < 5300 (gap 200), so critical clears < 6000.
	const FHysteresisRule EngineRpmTooHigh{ 5500.f, 5300.f, true, 6200.f, 6000.f, /*bEnterAtThreshold*/ false, /*bStayAtThreshold*/ true };
}

FVehicleStatusReport FVehicleStatusEvaluator::EvaluateTelemetrySample(const FVehicleTelemetry& TelemetrySample)
{
	using namespace VehicleStatusRules;

	const FTyrePressuresKpa& TyreKpa = TelemetrySample.TyreKpa;
	const float LowestTyreKpa = FMath::Min(FMath::Min(TyreKpa.FL, TyreKpa.FR), FMath::Min(TyreKpa.RL, TyreKpa.RR));
	const float HighestTyreKpa = FMath::Max(FMath::Max(TyreKpa.FL, TyreKpa.FR), FMath::Max(TyreKpa.RL, TyreKpa.RR));

	PreviousCoolantTooHotStatus = EvaluateRule(TelemetrySample.CoolantTempC, PreviousCoolantTooHotStatus, CoolantTooHot);
	PreviousTyreTooLowStatus = EvaluateRule(-LowestTyreKpa, PreviousTyreTooLowStatus, TyreTooLow);
	PreviousTyreTooHighStatus = EvaluateRule(HighestTyreKpa, PreviousTyreTooHighStatus, TyreTooHigh);
	PreviousFuelTooLowStatus = EvaluateRule(-TelemetrySample.FuelPct, PreviousFuelTooLowStatus, FuelTooLow);
	PreviousEngineRpmTooHighStatus = EvaluateRule(TelemetrySample.EngineRpm, PreviousEngineRpmTooHighStatus, EngineRpmTooHigh);

	// Battery only counts while the engine runs; with the engine off its memory is cleared so a restart is judged fresh.
	const bool bEngineRunning = TelemetrySample.EngineRpm > 0.f;
	PreviousBatteryTooLowStatus = bEngineRunning ? EvaluateRule(-TelemetrySample.BatteryV, PreviousBatteryTooLowStatus, BatteryTooLow) : EVehicleStatus::Normal;
	PreviousBatteryTooHighStatus = bEngineRunning ? EvaluateRule(TelemetrySample.BatteryV, PreviousBatteryTooHighStatus, BatteryTooHigh) : EVehicleStatus::Normal;

	FVehicleStatusReport StatusReport;
	StatusReport.CoolantTemperatureStatus = PreviousCoolantTooHotStatus;
	StatusReport.TyrePressureStatus = WorseStatus(PreviousTyreTooLowStatus, PreviousTyreTooHighStatus);
	StatusReport.FuelLevelStatus = PreviousFuelTooLowStatus;
	StatusReport.BatteryVoltageStatus = WorseStatus(PreviousBatteryTooLowStatus, PreviousBatteryTooHighStatus);
	StatusReport.EngineRpmStatus = PreviousEngineRpmTooHighStatus;
	StatusReport.OverallStatus = WorseStatus(
		WorseStatus(StatusReport.CoolantTemperatureStatus, StatusReport.TyrePressureStatus),
		WorseStatus(WorseStatus(StatusReport.FuelLevelStatus, StatusReport.BatteryVoltageStatus), StatusReport.EngineRpmStatus));
	return StatusReport;
}

void FVehicleStatusEvaluator::ResetStatusMemory()
{
	*this = FVehicleStatusEvaluator();
}

EVehicleStatus FVehicleStatusEvaluator::WorseStatus(EVehicleStatus FirstStatus, EVehicleStatus SecondStatus)
{
	return static_cast<uint8>(FirstStatus) >= static_cast<uint8>(SecondStatus) ? FirstStatus : SecondStatus;
}
