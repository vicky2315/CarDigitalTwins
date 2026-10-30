// Automation tests for the derived vehicle status (docs/TESTS.md §8, S1–S11): every SPEC.md §3 threshold on both sides of its
// value, enter/clear hysteresis, worst-of across signals, and the committed sample trip. Run in the editor: Tools > Session Frontend
// > Automation, filter "CarDigitalTwins.Telemetry.StatusEvaluator". Headless: UnrealEditor-Cmd.exe CarDigitalTwins.uproject
//   -ExecCmds="Automation RunTests CarDigitalTwins.Telemetry.StatusEvaluator; Quit" -nullrhi -unattended -log
// S11 reads the committed Data/Trips/trip_sample.json; the rest need no files, world or assets.

#include "Misc/AutomationTest.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Telemetry/RecordedTripFile.h"
#include "Telemetry/VehicleStatusEvaluator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace VehicleStatusEvaluatorTestHelpers
{
	using EStatus = EVehicleStatus;

	// Engine running, every signal well inside its Normal band.
	FVehicleTelemetry MakeHealthyTelemetrySample()
	{
		FVehicleTelemetry HealthyTelemetrySample;
		HealthyTelemetrySample.SpeedKmh = 50.f;
		HealthyTelemetrySample.EngineRpm = 2000.f;
		HealthyTelemetrySample.Gear = 3;
		HealthyTelemetrySample.CoolantTempC = 90.f;
		HealthyTelemetrySample.FuelPct = 70.f;
		HealthyTelemetrySample.BatteryV = 14.1f;
		HealthyTelemetrySample.TyreKpa.FL = 240.f;
		HealthyTelemetrySample.TyreKpa.FR = 241.f;
		HealthyTelemetrySample.TyreKpa.RL = 238.f;
		HealthyTelemetrySample.TyreKpa.RR = 240.f;
		return HealthyTelemetrySample;
	}

	// One step of a walk along a signal: the value fed in and the status that sample must give, given the steps before it.
	struct FStatusStep
	{
		float Value;
		EStatus ExpectedStatus;
	};

	const TCHAR* GetStatusName(EStatus Status)
	{
		switch (Status)
		{
		case EStatus::Normal: return TEXT("Normal");
		case EStatus::Warning: return TEXT("Warning");
		case EStatus::Critical: return TEXT("Critical");
		}
		return TEXT("?");
	}

	// How a walk applies its value to the healthy sample and which row of the report it checks.
	using FApplyValue = TFunction<void(FVehicleTelemetry&, float)>;
	using FReadStatus = TFunction<EStatus(const FVehicleStatusReport&)>;

	const FReadStatus ReadCoolant = [](const FVehicleStatusReport& Report) { return Report.CoolantTemperatureStatus; };
	const FReadStatus ReadTyres = [](const FVehicleStatusReport& Report) { return Report.TyrePressureStatus; };
	const FReadStatus ReadFuel = [](const FVehicleStatusReport& Report) { return Report.FuelLevelStatus; };
	const FReadStatus ReadBattery = [](const FVehicleStatusReport& Report) { return Report.BatteryVoltageStatus; };
	const FReadStatus ReadEngineRpm = [](const FVehicleStatusReport& Report) { return Report.EngineRpmStatus; };
}

BEGIN_DEFINE_SPEC(FVehicleStatusEvaluatorSpec, "CarDigitalTwins.Telemetry.StatusEvaluator", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	FVehicleStatusEvaluator StatusEvaluator;

	// Feeds the steps in order to one evaluator and checks each sample's row, and that the overall status is that row (every other
	// signal stays healthy). A failure names the step, its value and both statuses, so the broken edge of the table is easy to find.
	void CheckStatusWalk(const TCHAR* What, const VehicleStatusEvaluatorTestHelpers::FApplyValue& ApplyValue,
		const VehicleStatusEvaluatorTestHelpers::FReadStatus& ReadStatus, std::initializer_list<VehicleStatusEvaluatorTestHelpers::FStatusStep> Steps)
	{
		using namespace VehicleStatusEvaluatorTestHelpers;
		int32 StepIndex = 0;
		for (const FStatusStep& Step : Steps)
		{
			FVehicleTelemetry TelemetrySample = MakeHealthyTelemetrySample();
			ApplyValue(TelemetrySample, Step.Value);
			const FVehicleStatusReport StatusReport = StatusEvaluator.EvaluateTelemetrySample(TelemetrySample);
			const EStatus ActualStatus = ReadStatus(StatusReport);
			TestTrue(FString::Printf(TEXT("%s, step %d (%g): got %s, expected %s"), What, StepIndex, Step.Value, GetStatusName(ActualStatus),
				GetStatusName(Step.ExpectedStatus)), ActualStatus == Step.ExpectedStatus);
			TestTrue(FString::Printf(TEXT("%s, step %d (%g): overall is the only off row"), What, StepIndex, Step.Value),
				StatusReport.OverallStatus == Step.ExpectedStatus);
			++StepIndex;
		}
	}
END_DEFINE_SPEC(FVehicleStatusEvaluatorSpec)

void FVehicleStatusEvaluatorSpec::Define()
{
	using namespace VehicleStatusEvaluatorTestHelpers;

	BeforeEach([this]()
	{
		StatusEvaluator = FVehicleStatusEvaluator();
	});

	Describe("Thresholds", [this]()
	{
		It("S1: a healthy running car is Normal on every row", [this]()
		{
			const FVehicleStatusReport StatusReport = StatusEvaluator.EvaluateTelemetrySample(MakeHealthyTelemetrySample());
			TestTrue(TEXT("overall"), StatusReport.OverallStatus == EStatus::Normal);
			TestTrue(TEXT("coolant"), StatusReport.CoolantTemperatureStatus == EStatus::Normal);
			TestTrue(TEXT("tyres"), StatusReport.TyrePressureStatus == EStatus::Normal);
			TestTrue(TEXT("fuel"), StatusReport.FuelLevelStatus == EStatus::Normal);
			TestTrue(TEXT("battery"), StatusReport.BatteryVoltageStatus == EStatus::Normal);
			TestTrue(TEXT("rpm"), StatusReport.EngineRpmStatus == EStatus::Normal);
		});

		// Rises through both thresholds, then falls back through both clear values. ">=" rows enter at exactly the threshold,
		// ">" rows just above it; "<" clear values still hold at exactly the clear value.
		It("S2: coolant enters at >= 105 / >= 115 and clears below 112 / 102", [this]()
		{
			CheckStatusWalk(TEXT("coolant"), [](FVehicleTelemetry& Sample, float Value) { Sample.CoolantTempC = Value; }, ReadCoolant, {
				{ 104.9f, EStatus::Normal }, { 105.f, EStatus::Warning }, { 114.9f, EStatus::Warning }, { 115.f, EStatus::Critical },
				{ 112.f, EStatus::Critical }, { 111.9f, EStatus::Warning }, { 102.f, EStatus::Warning }, { 101.9f, EStatus::Normal } });
		});

		It("S3: the lowest tyre enters below 180 / 140 and clears at >= 145 / 185", [this]()
		{
			// Only RR leaks, as in the sample trip; the other three stay near 240 kPa.
			CheckStatusWalk(TEXT("tyre low"), [](FVehicleTelemetry& Sample, float Value) { Sample.TyreKpa.RR = Value; }, ReadTyres, {
				{ 180.f, EStatus::Normal }, { 179.9f, EStatus::Warning }, { 140.f, EStatus::Warning }, { 139.9f, EStatus::Critical },
				{ 144.9f, EStatus::Critical }, { 145.f, EStatus::Warning }, { 184.9f, EStatus::Warning }, { 185.f, EStatus::Normal } });
		});

		It("S4: the highest tyre enters above 300 and clears at <= 295, with no Critical", [this]()
		{
			CheckStatusWalk(TEXT("tyre high"), [](FVehicleTelemetry& Sample, float Value) { Sample.TyreKpa.FL = Value; }, ReadTyres, {
				{ 300.f, EStatus::Normal }, { 300.1f, EStatus::Warning }, { 400.f, EStatus::Warning }, { 295.1f, EStatus::Warning },
				{ 295.f, EStatus::Normal } });
		});

		It("S5: fuel enters below 15 / 5 and clears at >= 7 / 17", [this]()
		{
			CheckStatusWalk(TEXT("fuel"), [](FVehicleTelemetry& Sample, float Value) { Sample.FuelPct = Value; }, ReadFuel, {
				{ 15.f, EStatus::Normal }, { 14.9f, EStatus::Warning }, { 5.f, EStatus::Warning }, { 4.9f, EStatus::Critical },
				{ 6.9f, EStatus::Critical }, { 7.f, EStatus::Warning }, { 16.9f, EStatus::Warning }, { 17.f, EStatus::Normal } });
		});

		It("S6: battery (engine running) enters below 13.0 / 12.0 or above 14.8 / 15.5, and clears inside 13.2-14.6", [this]()
		{
			const FApplyValue SetBattery = [](FVehicleTelemetry& Sample, float Value) { Sample.BatteryV = Value; };
			CheckStatusWalk(TEXT("battery low"), SetBattery, ReadBattery, {
				{ 13.f, EStatus::Normal }, { 12.9f, EStatus::Warning }, { 12.f, EStatus::Warning }, { 11.9f, EStatus::Critical },
				{ 12.1f, EStatus::Critical }, { 12.2f, EStatus::Warning }, { 13.1f, EStatus::Warning }, { 13.2f, EStatus::Normal } });
			CheckStatusWalk(TEXT("battery high"), SetBattery, ReadBattery, {
				{ 14.8f, EStatus::Normal }, { 14.9f, EStatus::Warning }, { 15.5f, EStatus::Warning }, { 15.6f, EStatus::Critical },
				{ 15.4f, EStatus::Critical }, { 15.3f, EStatus::Warning }, { 14.7f, EStatus::Warning }, { 14.6f, EStatus::Normal } });
		});

		It("S7: engine rpm enters above 5500 / 6200 and clears below 6000 / 5300", [this]()
		{
			CheckStatusWalk(TEXT("rpm"), [](FVehicleTelemetry& Sample, float Value) { Sample.EngineRpm = Value; }, ReadEngineRpm, {
				{ 5500.f, EStatus::Normal }, { 5500.1f, EStatus::Warning }, { 6200.f, EStatus::Warning }, { 6200.1f, EStatus::Critical },
				{ 6000.f, EStatus::Critical }, { 5999.9f, EStatus::Warning }, { 5300.f, EStatus::Warning }, { 5299.9f, EStatus::Normal } });
		});
	});

	Describe("Hysteresis", [this]()
	{
		It("S8: the same value is Warning or Normal depending on where it came from; jumps skip levels both ways", [this]()
		{
			const FApplyValue SetCoolant = [](FVehicleTelemetry& Sample, float Value) { Sample.CoolantTempC = Value; };
			// 103 °C sits in the gap between clear (102) and enter (105).
			CheckStatusWalk(TEXT("rising to 103"), SetCoolant, ReadCoolant, { { 95.f, EStatus::Normal }, { 103.f, EStatus::Normal } });
			CheckStatusWalk(TEXT("falling to 103"), SetCoolant, ReadCoolant, { { 108.f, EStatus::Warning }, { 103.f, EStatus::Warning } });
			// 113 °C: Warning on the way up, still Critical on the way down.
			CheckStatusWalk(TEXT("rising to 113"), SetCoolant, ReadCoolant, { { 90.f, EStatus::Normal }, { 113.f, EStatus::Warning } });
			CheckStatusWalk(TEXT("falling to 113"), SetCoolant, ReadCoolant, { { 120.f, EStatus::Critical }, { 113.f, EStatus::Critical } });
			// A big jump goes straight from Normal to Critical and back to Normal, no Warning sample in between.
			CheckStatusWalk(TEXT("jumps"), SetCoolant, ReadCoolant, { { 90.f, EStatus::Normal }, { 120.f, EStatus::Critical },
				{ 90.f, EStatus::Normal } });
		});

		It("S9: ResetStatusMemory forgets the hysteresis; engine off ignores and clears the battery", [this]()
		{
			CheckStatusWalk(TEXT("before reset"), [](FVehicleTelemetry& Sample, float Value) { Sample.CoolantTempC = Value; }, ReadCoolant,
				{ { 108.f, EStatus::Warning } });
			StatusEvaluator.ResetStatusMemory();
			CheckStatusWalk(TEXT("after reset"), [](FVehicleTelemetry& Sample, float Value) { Sample.CoolantTempC = Value; }, ReadCoolant,
				{ { 103.f, EStatus::Normal } });

			// 11 V with the engine off is a resting battery, not a fault. Value = battery V; the walk sets rpm per step.
			const FApplyValue SetBatteryEngineOff = [](FVehicleTelemetry& Sample, float Value) { Sample.EngineRpm = 0.f; Sample.BatteryV = Value; };
			const FApplyValue SetBatteryEngineOn = [](FVehicleTelemetry& Sample, float Value) { Sample.BatteryV = Value; };
			CheckStatusWalk(TEXT("engine on, low"), SetBatteryEngineOn, ReadBattery, { { 12.9f, EStatus::Warning } });
			CheckStatusWalk(TEXT("engine off"), SetBatteryEngineOff, ReadBattery, { { 11.f, EStatus::Normal } });
			// 13.1 V would still be Warning had the memory survived; after engine off it is judged fresh.
			CheckStatusWalk(TEXT("engine on again"), SetBatteryEngineOn, ReadBattery, { { 13.1f, EStatus::Normal } });
		});
	});

	Describe("Worst of", [this]()
	{
		It("S10: overall is the worst row, each row keeps its own status, tyres take the worse side", [this]()
		{
			FVehicleTelemetry TelemetrySample = MakeHealthyTelemetrySample();
			TelemetrySample.CoolantTempC = 108.f;
			TelemetrySample.FuelPct = 3.f;
			FVehicleStatusReport StatusReport = StatusEvaluator.EvaluateTelemetrySample(TelemetrySample);
			TestTrue(TEXT("coolant Warning + fuel Critical: overall Critical"), StatusReport.OverallStatus == EStatus::Critical);
			TestTrue(TEXT("coolant row Warning"), StatusReport.CoolantTemperatureStatus == EStatus::Warning);
			TestTrue(TEXT("fuel row Critical"), StatusReport.FuelLevelStatus == EStatus::Critical);
			TestTrue(TEXT("other rows Normal"), StatusReport.TyrePressureStatus == EStatus::Normal
				&& StatusReport.BatteryVoltageStatus == EStatus::Normal && StatusReport.EngineRpmStatus == EStatus::Normal);

			StatusEvaluator.ResetStatusMemory();
			TelemetrySample = MakeHealthyTelemetrySample();
			TelemetrySample.EngineRpm = 5600.f;
			TelemetrySample.TyreKpa.RL = 170.f;
			StatusReport = StatusEvaluator.EvaluateTelemetrySample(TelemetrySample);
			TestTrue(TEXT("two Warnings: overall Warning"), StatusReport.OverallStatus == EStatus::Warning);

			StatusEvaluator.ResetStatusMemory();
			TelemetrySample = MakeHealthyTelemetrySample();
			TelemetrySample.TyreKpa.FR = 310.f;
			TelemetrySample.TyreKpa.RR = 130.f;
			StatusReport = StatusEvaluator.EvaluateTelemetrySample(TelemetrySample);
			TestTrue(TEXT("one tyre high (Warning) and one low (Critical): tyre row Critical"), StatusReport.TyrePressureStatus == EStatus::Critical);

			TestTrue(TEXT("WorseStatus Normal/Critical"), FVehicleStatusEvaluator::WorseStatus(EStatus::Normal, EStatus::Critical) == EStatus::Critical);
			TestTrue(TEXT("WorseStatus Critical/Warning"), FVehicleStatusEvaluator::WorseStatus(EStatus::Critical, EStatus::Warning) == EStatus::Critical);
			TestTrue(TEXT("WorseStatus Warning/Normal"), FVehicleStatusEvaluator::WorseStatus(EStatus::Warning, EStatus::Normal) == EStatus::Warning);
			TestTrue(TEXT("WorseStatus Normal/Normal"), FVehicleStatusEvaluator::WorseStatus(EStatus::Normal, EStatus::Normal) == EStatus::Normal);
		});
	});

	Describe("Sample trip", [this]()
	{
		// The same timings SPEC.md §2.2 promises and P6 checks on the Python side (±3 s), now through the C++ evaluator.
		It("S11: the committed trip turns coolant Warning ~91 s and Critical ~119 s, the RR tyre Warning ~148 s, nothing else", [this]()
		{
			FString TripJsonText;
			const FString TripPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/Trips/trip_sample.json"));
			if (!TestTrue(TEXT("trip file read"), FFileHelper::LoadFileToString(TripJsonText, *TripPath)))
			{
				return;
			}
			FRecordedTripFile RecordedTrip;
			if (!TestTrue(TEXT("trip parsed"), FJsonObjectConverter::JsonObjectStringToUStruct(TripJsonText, &RecordedTrip))
				|| !TestEqual(TEXT("frame count"), RecordedTrip.Frames.Num(), 1900))
			{
				return;
			}

			double CoolantWarningTimeS = -1.0;
			double CoolantCriticalTimeS = -1.0;
			double TyreWarningTimeS = -1.0;
			EStatus WorstTyreStatus = EStatus::Normal;
			EStatus WorstOtherStatus = EStatus::Normal;
			for (const FVehicleTelemetry& TelemetryFrame : RecordedTrip.Frames)
			{
				const FVehicleStatusReport StatusReport = StatusEvaluator.EvaluateTelemetrySample(TelemetryFrame);
				if (CoolantWarningTimeS < 0.0 && StatusReport.CoolantTemperatureStatus != EStatus::Normal)
				{
					CoolantWarningTimeS = TelemetryFrame.SampleTimeS;
				}
				if (CoolantCriticalTimeS < 0.0 && StatusReport.CoolantTemperatureStatus == EStatus::Critical)
				{
					CoolantCriticalTimeS = TelemetryFrame.SampleTimeS;
				}
				if (TyreWarningTimeS < 0.0 && StatusReport.TyrePressureStatus != EStatus::Normal)
				{
					TyreWarningTimeS = TelemetryFrame.SampleTimeS;
				}
				WorstTyreStatus = FVehicleStatusEvaluator::WorseStatus(WorstTyreStatus, StatusReport.TyrePressureStatus);
				WorstOtherStatus = FVehicleStatusEvaluator::WorseStatus(WorstOtherStatus, FVehicleStatusEvaluator::WorseStatus(
					StatusReport.FuelLevelStatus, FVehicleStatusEvaluator::WorseStatus(StatusReport.BatteryVoltageStatus, StatusReport.EngineRpmStatus)));
			}

			TestTrue(FString::Printf(TEXT("coolant Warning at %.1f s, expected 88-94 s"), CoolantWarningTimeS),
				CoolantWarningTimeS >= 88.0 && CoolantWarningTimeS <= 94.0);
			TestTrue(FString::Printf(TEXT("coolant Critical at %.1f s, expected 116-122 s"), CoolantCriticalTimeS),
				CoolantCriticalTimeS >= 116.0 && CoolantCriticalTimeS <= 122.0);
			TestTrue(FString::Printf(TEXT("RR tyre Warning at %.1f s, expected 145-151 s"), TyreWarningTimeS),
				TyreWarningTimeS >= 145.0 && TyreWarningTimeS <= 151.0);
			TestTrue(TEXT("tyre never Critical (ends ~150 kPa)"), WorstTyreStatus != EStatus::Critical);
			TestTrue(TEXT("fuel, battery and rpm stay Normal all trip"), WorstOtherStatus == EStatus::Normal);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
