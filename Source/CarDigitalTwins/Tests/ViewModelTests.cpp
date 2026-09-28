// Automation tests for the custom MVVM core. Run: Session Frontend → Automation → filter "CarDigitalTwins.MVVM",
// or headless: UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests CarDigitalTwins.MVVM;Quit" -unattended -nullrhi

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "Telemetry/VehicleTelemetryViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FVehicleTelemetry MakeSample(float SpeedKmh = 50.f, float CoolantTempC = 90.f)
	{
		FVehicleTelemetry Sample;
		Sample.SpeedKmh = SpeedKmh;
		Sample.EngineRpm = 2000.f;
		Sample.Gear = 3;
		Sample.CoolantTempC = CoolantTempC;
		Sample.FuelPct = 70.f;
		Sample.BatteryV = 14.1f;
		Sample.TyreKpa = { 240.f, 240.f, 238.f, 238.f };
		return Sample;
	}
}

BEGIN_DEFINE_SPEC(FViewModelCoreSpec, "CarDigitalTwins.MVVM.Core",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UVehicleTelemetryViewModel> VM;
	using EField = EVehicleTelemetryField;
END_DEFINE_SPEC(FViewModelCoreSpec)

void FViewModelCoreSpec::Define()
{
	BeforeEach([this]()
	{
		// Created outside UViewModelSubsystem, so the tests call Flush() themselves.
		VM.Reset(NewObject<UVehicleTelemetryViewModel>());
	});

	AfterEach([this]()
	{
		VM.Reset();
	});

	It("fires once with all fields on subscribe", [this]()
	{
		int32 Calls = 0;
		FViewModelFieldMask Received;
		FViewModelSubscription Sub = VM->Subscribe([&](FViewModelFieldMask Mask) { ++Calls; Received = Mask; });

		TestEqual("calls", Calls, 1);
		TestTrue("all fields", Received == VM->AllFields());
		TestTrue("status included", Received.Has(EField::Status));
	});

	It("batches many changes into one broadcast", [this]()
	{
		int32 Calls = 0;
		FViewModelFieldMask Received;
		FViewModelSubscription Sub = VM->Subscribe([&](FViewModelFieldMask Mask) { ++Calls; Received = Mask; });
		Calls = 0;

		VM->ApplySample(MakeSample(), EVehicleStatus::Normal);
		TestEqual("no broadcast before flush", Calls, 0);
		TestTrue("dirty", VM->IsDirty());

		VM->Flush();
		TestEqual("one broadcast", Calls, 1);
		TestTrue("speed changed", Received.Has(EField::SpeedKmh));
		TestTrue("coolant changed", Received.Has(EField::CoolantTempC));
		TestFalse("status unchanged (Normal -> Normal)", Received.Has(EField::Status));
		TestFalse("clean after flush", VM->IsDirty());
	});

	It("does not mark dirty when the sample repeats", [this]()
	{
		VM->ApplySample(MakeSample(), EVehicleStatus::Normal);
		VM->Flush();

		VM->ApplySample(MakeSample(), EVehicleStatus::Normal);
		TestFalse("not dirty", VM->IsDirty());
	});

	It("ignores float changes within tolerance and keeps the old value", [this]()
	{
		VM->ApplySample(MakeSample(50.f), EVehicleStatus::Normal);
		VM->Flush();

		VM->ApplySample(MakeSample(50.04f), EVehicleStatus::Normal);
		TestFalse("0.04 km/h ignored", VM->GetDirtyFields().Has(EField::SpeedKmh));
		TestEqual("old value kept", VM->GetSpeedKmh(), 50.f);

		VM->ApplySample(MakeSample(50.1f), EVehicleStatus::Normal);
		TestTrue("0.1 km/h marks dirty", VM->GetDirtyFields().Has(EField::SpeedKmh));
		TestEqual("new value stored", VM->GetSpeedKmh(), 50.1f);
	});

	It("reports only the fields that changed", [this]()
	{
		VM->ApplySample(MakeSample(50.f, 90.f), EVehicleStatus::Normal);
		VM->Flush();

		VM->ApplySample(MakeSample(50.f, 106.f), EVehicleStatus::Warning);
		const FViewModelFieldMask Dirty = VM->GetDirtyFields();
		TestTrue("coolant", Dirty.Has(EField::CoolantTempC));
		TestTrue("status", Dirty.Has(EField::Status));
		TestFalse("speed", Dirty.Has(EField::SpeedKmh));
		TestFalse("fuel", Dirty.Has(EField::FuelPct));
	});

	It("does not broadcast when nothing changed", [this]()
	{
		int32 Calls = 0;
		FViewModelSubscription Sub = VM->Subscribe([&](FViewModelFieldMask) { ++Calls; });
		Calls = 0;

		VM->Flush();
		TestEqual("no broadcast", Calls, 0);
	});

	It("stops calling after the subscription is destroyed", [this]()
	{
		int32 Calls = 0;
		{
			FViewModelSubscription Sub = VM->Subscribe([&](FViewModelFieldMask) { ++Calls; });
		}
		Calls = 0;

		VM->ApplySample(MakeSample(), EVehicleStatus::Normal);
		VM->Flush();
		TestEqual("no calls after unsubscribe", Calls, 0);
	});

	It("keeps the subscription alive when moved", [this]()
	{
		int32 Calls = 0;
		FViewModelSubscription Outer;
		{
			FViewModelSubscription Inner = VM->Subscribe([&](FViewModelFieldMask) { ++Calls; });
			Outer = MoveTemp(Inner);
		}
		Calls = 0;

		VM->ApplySample(MakeSample(), EVehicleStatus::Normal);
		VM->Flush();
		TestEqual("still subscribed", Calls, 1);
		TestTrue("active", Outer.IsActive());
	});

	It("delivers a change made during a flush on the next flush", [this]()
	{
		int32 Calls = 0;
		FViewModelSubscription Sub = VM->Subscribe([&](FViewModelFieldMask Mask)
		{
			++Calls;
			if (Calls == 2)
			{
				VM->ApplySample(MakeSample(80.f), EVehicleStatus::Normal);  // listener causes another change
			}
		});

		VM->ApplySample(MakeSample(50.f), EVehicleStatus::Normal);
		VM->Flush();
		TestEqual("first flush delivered", Calls, 2);
		TestTrue("re-dirtied by listener", VM->IsDirty());

		VM->Flush();
		TestEqual("second flush delivered", Calls, 3);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
