// Automation tests for UViewModelSubsystem (docs/TESTS.md §7, M10–M11). Same run as the core tests: filter "CarDigitalTwins.MVVM".
// The subsystem is made by hand on a bare game instance (no world, no other subsystems, so no telemetry source), and the tests call
// Tick themselves in place of the engine's once-per-frame tick.

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "MVVM/DashboardViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "MVVM/ViewModelSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FViewModelSubsystemSpec, "CarDigitalTwins.MVVM.Subsystem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UGameInstance> BareGameInstance;
	TStrongObjectPtr<UViewModelSubsystem> ViewModelSubsystem;

	// Counts the broadcasts of one ViewModel after Subscribe's immediate all-fields call, and keeps the last mask.
	struct FBroadcastCounter
	{
		FViewModelSubscription Subscription;
		int32 BroadcastCount = 0;
		FViewModelFieldMask LastBroadcastFieldMask;

		void Listen(UViewModelBase& ViewModel)
		{
			Subscription = ViewModel.SubscribeLambda([this](FViewModelFieldMask ChangedFieldMask)
			{
				++BroadcastCount;
				LastBroadcastFieldMask = ChangedFieldMask;
			});
			BroadcastCount = 0;
			LastBroadcastFieldMask.Reset();
		}
	};

	// Not initialized through a subsystem collection: Initialize would turn on the engine tick, and the tests tick by hand instead.
	// The game instance has no subsystems, so the telemetry ViewModel finds no telemetry source and only changes when a test says so.
	void CreateSubsystem()
	{
		BareGameInstance.Reset(NewObject<UGameInstance>(GetTransientPackage()));
		ViewModelSubsystem.Reset(NewObject<UViewModelSubsystem>(BareGameInstance.Get()));
	}

	static FVehicleTelemetry MakeTelemetrySampleAtSpeed(float SpeedKmh)
	{
		FVehicleTelemetry TelemetrySample;
		TelemetrySample.SpeedKmh = SpeedKmh;
		return TelemetrySample;
	}
END_DEFINE_SPEC(FViewModelSubsystemSpec)

void FViewModelSubsystemSpec::Define()
{
	using TelemetryField = EVehicleTelemetryViewModelField;

	BeforeEach([this]()
	{
		CreateSubsystem();
	});

	AfterEach([this]()
	{
		ViewModelSubsystem->Deinitialize();
		ViewModelSubsystem.Reset();
		BareGameInstance.Reset();
	});

	Describe("UViewModelSubsystem", [this]()
	{
		It("M10: one Tick flushes every ViewModel with changes once, with all of its changes; an idle Tick sends nothing", [this]()
		{
			UVehicleTelemetryViewModel* TelemetryViewModel = ViewModelSubsystem->GetViewModel<UVehicleTelemetryViewModel>();
			UDashboardViewModel* DashboardViewModel = ViewModelSubsystem->GetViewModel<UDashboardViewModel>();
			FBroadcastCounter TelemetryCounter;
			FBroadcastCounter DashboardCounter;
			TelemetryCounter.Listen(*TelemetryViewModel);
			DashboardCounter.Listen(*DashboardViewModel);

			// Several changes in one frame, on two ViewModels.
			TelemetryViewModel->ApplyTelemetrySample(MakeTelemetrySampleAtSpeed(40.f), FVehicleStatusReport());
			TelemetryViewModel->ApplyTelemetrySample(MakeTelemetrySampleAtSpeed(45.f), FVehicleStatusReport());
			DashboardViewModel->SelectView(ETwinDashboardView::Tyres);
			TestEqual(TEXT("nothing broadcast before the Tick"), TelemetryCounter.BroadcastCount + DashboardCounter.BroadcastCount, 0);

			ViewModelSubsystem->Tick(1.f / 60.f);
			TestEqual(TEXT("telemetry: one broadcast"), TelemetryCounter.BroadcastCount, 1);
			TestEqual(TEXT("dashboard: one broadcast"), DashboardCounter.BroadcastCount, 1);
			FViewModelFieldMask ExpectedTelemetryFieldMask;
			ExpectedTelemetryFieldMask.AddField(TelemetryField::HasReceivedTelemetry);
			ExpectedTelemetryFieldMask.AddField(TelemetryField::SpeedKmh);
			TestTrue(TEXT("telemetry: first sample + speed, once"), TelemetryCounter.LastBroadcastFieldMask == ExpectedTelemetryFieldMask);
			TestTrue(TEXT("dashboard: active view"), DashboardCounter.LastBroadcastFieldMask.HasField(EDashboardViewModelField::ActiveView));
			TestFalse(TEXT("telemetry: nothing left waiting"), TelemetryViewModel->HasChangedFieldsWaitingForFlush());
			TestFalse(TEXT("dashboard: nothing left waiting"), DashboardViewModel->HasChangedFieldsWaitingForFlush());

			ViewModelSubsystem->Tick(1.f / 60.f);
			TestEqual(TEXT("idle Tick: telemetry still 1"), TelemetryCounter.BroadcastCount, 1);
			TestEqual(TEXT("idle Tick: dashboard still 1"), DashboardCounter.BroadcastCount, 1);

			// Only the ViewModel that changed broadcasts.
			DashboardViewModel->SelectView(ETwinDashboardView::Body);
			ViewModelSubsystem->Tick(1.f / 60.f);
			TestEqual(TEXT("telemetry unchanged: still 1"), TelemetryCounter.BroadcastCount, 1);
			TestEqual(TEXT("dashboard changed: 2"), DashboardCounter.BroadcastCount, 2);
		});

		It("M11: GetViewModel keeps one ViewModel per class until Deinitialize", [this]()
		{
			UVehicleTelemetryViewModel* TelemetryViewModel = ViewModelSubsystem->GetViewModel<UVehicleTelemetryViewModel>();
			UDashboardViewModel* DashboardViewModel = ViewModelSubsystem->GetViewModel<UDashboardViewModel>();
			TestTrue(TEXT("telemetry ViewModel created"), TelemetryViewModel != nullptr);
			TestTrue(TEXT("same telemetry ViewModel on the second call"), ViewModelSubsystem->GetViewModel<UVehicleTelemetryViewModel>() == TelemetryViewModel);
			TestTrue(TEXT("same dashboard ViewModel on the second call"), ViewModelSubsystem->GetViewModel<UDashboardViewModel>() == DashboardViewModel);
			TestTrue(TEXT("by class too"), ViewModelSubsystem->GetViewModelOfClass(UDashboardViewModel::StaticClass()) == DashboardViewModel);
			TestTrue(TEXT("owned by the subsystem"), TelemetryViewModel->GetOuter() == ViewModelSubsystem.Get());

			// After Deinitialize the subsystem no longer holds or flushes the old ViewModel.
			FBroadcastCounter TelemetryCounter;
			TelemetryCounter.Listen(*TelemetryViewModel);
			ViewModelSubsystem->Deinitialize();
			TelemetryViewModel->ApplyTelemetrySample(MakeTelemetrySampleAtSpeed(30.f), FVehicleStatusReport());
			ViewModelSubsystem->Tick(1.f / 60.f);
			TestEqual(TEXT("released ViewModel isn't flushed"), TelemetryCounter.BroadcastCount, 0);
			TestTrue(TEXT("a new request makes a new ViewModel"), ViewModelSubsystem->GetViewModel<UVehicleTelemetryViewModel>() != TelemetryViewModel);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
