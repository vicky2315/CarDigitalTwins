// Automation tests for the custom MVVM core (docs/TESTS.md §7, M1–M9). Run in the editor: Tools > Session Frontend > Automation,
// filter "CarDigitalTwins.MVVM". Headless: UnrealEditor-Cmd.exe CarDigitalTwins.uproject
//   -ExecCmds="Automation RunTests CarDigitalTwins.MVVM; Quit" -nullrhi -unattended -log

#include "Misc/AutomationTest.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ViewModelCoreTestHelpers
{
	// A driving car with nothing unusual: every value the first sample sets is non-zero except pedals, steer, odometer and openings.
	FVehicleTelemetry MakeCruisingTelemetrySample()
	{
		FVehicleTelemetry CruisingTelemetrySample;
		CruisingTelemetrySample.SpeedKmh = 50.f;
		CruisingTelemetrySample.EngineRpm = 2000.f;
		CruisingTelemetrySample.Gear = 3;
		CruisingTelemetrySample.CoolantTempC = 90.f;
		CruisingTelemetrySample.FuelPct = 70.f;
		CruisingTelemetrySample.BatteryV = 14.1f;
		CruisingTelemetrySample.TyreKpa.FL = 240.f;
		CruisingTelemetrySample.TyreKpa.FR = 241.f;
		CruisingTelemetrySample.TyreKpa.RL = 238.f;
		CruisingTelemetrySample.TyreKpa.RR = 240.f;
		return CruisingTelemetrySample;
	}

	FViewModelFieldMask MakeFieldMask(std::initializer_list<EVehicleTelemetryViewModelField> Fields)
	{
		FViewModelFieldMask FieldMask;
		for (const EVehicleTelemetryViewModelField Field : Fields)
		{
			FieldMask.AddField(Field);
		}
		return FieldMask;
	}
}

BEGIN_DEFINE_SPEC(FViewModelCoreSpec, "CarDigitalTwins.MVVM.Core", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;
	FViewModelSubscription CountingSubscription;
	int32 BroadcastCount = 0;
	FViewModelFieldMask LastBroadcastFieldMask;

	// Counts broadcasts and keeps the last mask. Created ViewModels aren't in UViewModelSubsystem, so tests call Flush themselves.
	// Subscribe calls once right away with all fields (M9 checks that); the counters start after it.
	void CreateViewModelAndListen()
	{
		TelemetryViewModel.Reset(NewObject<UVehicleTelemetryViewModel>());
		CountingSubscription = TelemetryViewModel->SubscribeLambda([this](FViewModelFieldMask ChangedFieldMask)
		{
			++BroadcastCount;
			LastBroadcastFieldMask = ChangedFieldMask;
		});
		BroadcastCount = 0;
		LastBroadcastFieldMask.Reset();
	}

	// Compares two masks and shows both in hex on failure, so a wrong bit is easy to spot.
	void TestBitsEqual(const TCHAR* What, uint64 ActualBits, uint64 ExpectedBits)
	{
		TestTrue(FString::Printf(TEXT("%s: got 0x%llx, expected 0x%llx"), What, ActualBits, ExpectedBits), ActualBits == ExpectedBits);
	}
END_DEFINE_SPEC(FViewModelCoreSpec)

void FViewModelCoreSpec::Define()
{
	using namespace ViewModelCoreTestHelpers;
	using TelemetryField = EVehicleTelemetryViewModelField;

	BeforeEach([this]()
	{
		CreateViewModelAndListen();
	});

	AfterEach([this]()
	{
		CountingSubscription.Reset();
		TelemetryViewModel.Reset();
	});

	Describe("FViewModelFieldMask", [this]()
	{
		It("M1: AllFields sets exactly the first N bits, including the full 64", [this]()
		{
			TestBitsEqual(TEXT("0 fields"), FViewModelFieldMask::AllFields(0).ChangedFieldBits, uint64(0));
			TestBitsEqual(TEXT("3 fields"), FViewModelFieldMask::AllFields(3).ChangedFieldBits, uint64(0b111));
			TestBitsEqual(TEXT("64 fields"), FViewModelFieldMask::AllFields(64).ChangedFieldBits, ~uint64(0));
		});

		It("M2: AddField, HasField, HasFieldIndex and AddFields agree", [this]()
		{
			FViewModelFieldMask FirstFieldMask = MakeFieldMask({ TelemetryField::SpeedKmh });
			const FViewModelFieldMask SecondFieldMask = MakeFieldMask({ TelemetryField::CoolantTempC });

			TestTrue(TEXT("has speed"), FirstFieldMask.HasField(TelemetryField::SpeedKmh));
			TestFalse(TEXT("no coolant yet"), FirstFieldMask.HasField(TelemetryField::CoolantTempC));
			TestTrue(TEXT("speed by index"), FirstFieldMask.HasFieldIndex(static_cast<int32>(TelemetryField::SpeedKmh)));
			TestFalse(TEXT("index -1 is false"), FirstFieldMask.HasFieldIndex(-1));
			TestFalse(TEXT("index 64 is false"), FirstFieldMask.HasFieldIndex(64));

			FirstFieldMask.AddFields(SecondFieldMask);
			TestTrue(TEXT("merged mask has both"), FirstFieldMask == MakeFieldMask({ TelemetryField::SpeedKmh, TelemetryField::CoolantTempC }));
		});
	});

	Describe("UVehicleTelemetryViewModel", [this]()
	{
		It("M3: reports its field count and an all-fields mask that matches it", [this]()
		{
			TestEqual(TEXT("field count"), TelemetryViewModel->GetFieldCount(), static_cast<int32>(TelemetryField::Count));
			TestBitsEqual(TEXT("all fields"), TelemetryViewModel->GetAllFields().ChangedFieldBits,
				(uint64(1) << static_cast<int32>(TelemetryField::Count)) - 1);
		});

		It("M4: the first sample marks exactly the fields that left their default, in one broadcast at Flush", [this]()
		{
			TelemetryViewModel->ApplyTelemetrySample(MakeCruisingTelemetrySample(), FVehicleStatusReport());
			TestEqual(TEXT("nothing broadcast before Flush"), BroadcastCount, 0);
			TestTrue(TEXT("waiting for flush"), TelemetryViewModel->HasChangedFieldsWaitingForFlush());

			TelemetryViewModel->Flush();
			TestEqual(TEXT("one broadcast"), BroadcastCount, 1);
			const FViewModelFieldMask ExpectedFieldMask = MakeFieldMask({ TelemetryField::HasReceivedTelemetry, TelemetryField::SpeedKmh,
				TelemetryField::EngineRpm, TelemetryField::Gear, TelemetryField::CoolantTempC, TelemetryField::FuelPct, TelemetryField::BatteryV,
				TelemetryField::TyrePressureFrontLeftKpa, TelemetryField::TyrePressureFrontRightKpa, TelemetryField::TyrePressureRearLeftKpa,
				TelemetryField::TyrePressureRearRightKpa });
			TestBitsEqual(TEXT("exact mask (pedals, steer, odometer, openings, drive mode, statuses unchanged)"),
				LastBroadcastFieldMask.ChangedFieldBits, ExpectedFieldMask.ChangedFieldBits);
			TestTrue(TEXT("getter has the new value"), FMath::IsNearlyEqual(TelemetryViewModel->GetSpeedKmh(), 50.f));
			TestFalse(TEXT("nothing waiting after Flush"), TelemetryViewModel->HasChangedFieldsWaitingForFlush());

			TelemetryViewModel->Flush();
			TestEqual(TEXT("an empty flush broadcasts nothing"), BroadcastCount, 1);
		});

		It("M5: many samples before one Flush give one broadcast with the union of their changes", [this]()
		{
			FVehicleTelemetry TelemetrySample = MakeCruisingTelemetrySample();
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetryViewModel->Flush();

			TelemetrySample.SpeedKmh = 55.f;
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetrySample.Gear = 4;
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetrySample.SpeedKmh = 60.f;
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetryViewModel->Flush();

			TestEqual(TEXT("two broadcasts in total"), BroadcastCount, 2);
			TestBitsEqual(TEXT("speed and gear, once each"), LastBroadcastFieldMask.ChangedFieldBits,
				MakeFieldMask({ TelemetryField::SpeedKmh, TelemetryField::Gear }).ChangedFieldBits);
			TestTrue(TEXT("latest speed"), FMath::IsNearlyEqual(TelemetryViewModel->GetSpeedKmh(), 60.f));
		});

		It("M6: a float change within its tolerance is ignored, but slow drift gets through once it adds up", [this]()
		{
			FVehicleTelemetry TelemetrySample = MakeCruisingTelemetrySample();
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetryViewModel->Flush();

			// Coolant tolerance is 0.05 °C. +0.03 is noise: not marked, and the stored (reported) value stays 90.00.
			TelemetrySample.CoolantTempC = 90.03f;
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TestFalse(TEXT("+0.03 not marked"), TelemetryViewModel->HasChangedFieldsWaitingForFlush());
			TestTrue(TEXT("getter keeps the reported 90.00"), FMath::IsNearlyEqual(TelemetryViewModel->GetCoolantTempC(), 90.f));

			// Another +0.03: 0.06 away from the reported value, so it gets through.
			TelemetrySample.CoolantTempC = 90.06f;
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetryViewModel->Flush();
			TestBitsEqual(TEXT("drift reported"), LastBroadcastFieldMask.ChangedFieldBits, MakeFieldMask({ TelemetryField::CoolantTempC }).ChangedFieldBits);
			TestTrue(TEXT("getter has 90.06"), FMath::IsNearlyEqual(TelemetryViewModel->GetCoolantTempC(), 90.06f));
		});

		It("M7: status and drive mode changes are marked like values", [this]()
		{
			FVehicleTelemetry TelemetrySample = MakeCruisingTelemetrySample();
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());
			TelemetryViewModel->Flush();

			FVehicleStatusReport CoolantWarningReport;
			CoolantWarningReport.CoolantTemperatureStatus = EVehicleStatus::Warning;
			CoolantWarningReport.OverallStatus = EVehicleStatus::Warning;
			TelemetrySample.DriveMode = EVehicleDriveMode::EngineDerate;
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, CoolantWarningReport);
			TelemetryViewModel->Flush();

			TestBitsEqual(TEXT("overall + coolant status + drive mode"), LastBroadcastFieldMask.ChangedFieldBits, MakeFieldMask({
				TelemetryField::OverallStatus, TelemetryField::CoolantTemperatureStatus, TelemetryField::DriveMode }).ChangedFieldBits);
			TestTrue(TEXT("getter says Warning"), TelemetryViewModel->GetCoolantTemperatureStatus() == EVehicleStatus::Warning);
		});

		It("M8: a field changed by a listener during Flush goes out with the next Flush, not lost", [this]()
		{
			FVehicleTelemetry TelemetrySample = MakeCruisingTelemetrySample();
			TelemetryViewModel->ApplyTelemetrySample(TelemetrySample, FVehicleStatusReport());

			// On the first broadcast only, this listener applies a faster sample, as if new data arrived during the flush. Armed after
			// subscribing, so the immediate all-fields call from Subscribe doesn't count as that broadcast.
			bool bIsArmed = false;
			bool bHasChangedSpeedDuringFlush = false;
			FViewModelSubscription ChangingSubscription = TelemetryViewModel->SubscribeLambda(
				[this, &bIsArmed, &bHasChangedSpeedDuringFlush, TelemetrySample](FViewModelFieldMask)
			{
				if (bIsArmed && !bHasChangedSpeedDuringFlush)
				{
					bHasChangedSpeedDuringFlush = true;
					FVehicleTelemetry FasterTelemetrySample = TelemetrySample;
					FasterTelemetrySample.SpeedKmh = 80.f;
					TelemetryViewModel->ApplyTelemetrySample(FasterTelemetrySample, FVehicleStatusReport());
				}
			});
			bIsArmed = true;

			TelemetryViewModel->Flush();
			TestEqual(TEXT("first flush broadcast once"), BroadcastCount, 1);
			TestTrue(TEXT("the listener's change waits for the next flush"), TelemetryViewModel->HasChangedFieldsWaitingForFlush());

			TelemetryViewModel->Flush();
			TestEqual(TEXT("second flush broadcast"), BroadcastCount, 2);
			TestBitsEqual(TEXT("only speed"), LastBroadcastFieldMask.ChangedFieldBits, MakeFieldMask({ TelemetryField::SpeedKmh }).ChangedFieldBits);
		});

		It("M9: Subscribe calls once at once with all fields, and resetting the handle stops the calls", [this]()
		{
			int32 SubscriberCallCount = 0;
			FViewModelFieldMask FirstFieldMask;
			FViewModelSubscription TestSubscription = TelemetryViewModel->SubscribeLambda([&SubscriberCallCount, &FirstFieldMask](FViewModelFieldMask ChangedFieldMask)
			{
				if (SubscriberCallCount++ == 0)
				{
					FirstFieldMask = ChangedFieldMask;
				}
			});
			TestEqual(TEXT("called once while subscribing"), SubscriberCallCount, 1);
			TestBitsEqual(TEXT("with every field"), FirstFieldMask.ChangedFieldBits, TelemetryViewModel->GetAllFields().ChangedFieldBits);
			TestTrue(TEXT("handle active"), TestSubscription.IsActive());

			TelemetryViewModel->ApplyTelemetrySample(MakeCruisingTelemetrySample(), FVehicleStatusReport());
			TelemetryViewModel->Flush();
			TestEqual(TEXT("called again on the flush"), SubscriberCallCount, 2);

			TestSubscription.Reset();
			FVehicleTelemetry FasterTelemetrySample = MakeCruisingTelemetrySample();
			FasterTelemetrySample.SpeedKmh = 70.f;
			TelemetryViewModel->ApplyTelemetrySample(FasterTelemetrySample, FVehicleStatusReport());
			TelemetryViewModel->Flush();
			TestEqual(TEXT("no call after Reset"), SubscriberCallCount, 2);
			TestEqual(TEXT("the counting listener still hears it"), BroadcastCount, 2);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
