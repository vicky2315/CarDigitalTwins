// Automation tests for the recorded-trip receiver and its helpers (docs/TESTS.md §2, U1–U9). Run in the editor: Tools > Session
// Frontend > Automation, filter "CarDigitalTwins.Telemetry". Headless: UnrealEditor-Cmd.exe CarDigitalTwins.uproject
//   -ExecCmds="Automation RunTests CarDigitalTwins.Telemetry; Quit" -nullrhi -unattended -log
// U1 reads the committed Data/Trips/trip_sample.json; the others write small trips to Saved/Automation/TelemetryTests/ and delete them.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Telemetry/FileTelemetryReceiver.h"
#include "Telemetry/RecordedTripFile.h"
#include "Telemetry/TelemetrySettings.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace FileTelemetryReceiverTestHelpers
{
	// Relative to the project folder, like UTelemetrySettings::TripFileRelativePath.
	const TCHAR* const TestTripFolder = TEXT("Saved/Automation/TelemetryTests");

	// A trip of FrameCount frames at RateHz: frame N has seq N, sampleTimeS N / RateHz and speedKmh N + 1, so every frame is told
	// apart by its seq and nothing is left at 0. schemaVersion < 0 leaves the key out.
	FString MakeTripJson(int32 FrameCount, float RateHz, int32 SchemaVersion = VehicleTelemetrySchemaVersion)
	{
		TArray<FString> FrameJsonTexts;
		for (int32 FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
		{
			const double SampleTimeS = RateHz > 0.f ? FrameIndex / static_cast<double>(RateHz) : 0.0;
			FrameJsonTexts.Add(FString::Printf(TEXT("{\"seq\": %d, \"sampleTimeS\": %.4f, \"speedKmh\": %d.0, \"engineRpm\": 900, \"gear\": 1, ")
				TEXT("\"throttlePct\": 10.0, \"brakePct\": 0.0, \"steerDeg\": 0.0, \"odometerKm\": 100.0, \"coolantTempC\": 90.0, \"fuelPct\": 50.0, ")
				TEXT("\"batteryV\": 14.0, \"tyreKpa\": {\"fl\": 240, \"fr\": 240, \"rl\": 240, \"rr\": 240}, \"openings\": 0}"),
				FrameIndex, SampleTimeS, FrameIndex + 1));
		}
		const FString SchemaVersionJson = SchemaVersion >= 0 ? FString::Printf(TEXT("\"schemaVersion\": %d, "), SchemaVersion) : FString();
		return FString::Printf(TEXT("{%s\"vehicleId\": \"test-01\", \"rateHz\": %g, \"seed\": 1, \"frames\": [%s]}"), *SchemaVersionJson, RateHz,
			*FString::Join(FrameJsonTexts, TEXT(", ")));
	}

	// Writes JsonText to TestTripFolder/FileName and returns the path relative to the project folder.
	FString WriteTestTrip(const FString& FileName, const FString& JsonText)
	{
		const FString TripRelativePath = FString(TestTripFolder) / FileName;
		FFileHelper::SaveStringToFile(JsonText, *FPaths::Combine(FPaths::ProjectDir(), TripRelativePath));
		return TripRelativePath;
	}

	// Polls like the subsystem does at FramesPerSecond, StepCount times, and returns everything delivered, oldest first.
	TArray<FVehicleTelemetry> PollSteps(UFileTelemetryReceiver& Receiver, float FramesPerSecond, int32 StepCount)
	{
		TArray<FVehicleTelemetry> DeliveredSamples;
		for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
		{
			Receiver.PollNewTelemetrySamples(1.f / FramesPerSecond, DeliveredSamples);
		}
		return DeliveredSamples;
	}

	TArray<FVehicleTelemetry> PollOnce(UFileTelemetryReceiver& Receiver, float DeltaSeconds)
	{
		TArray<FVehicleTelemetry> DeliveredSamples;
		Receiver.PollNewTelemetrySamples(DeltaSeconds, DeliveredSamples);
		return DeliveredSamples;
	}
}

BEGIN_DEFINE_SPEC(FFileTelemetryReceiverSpec, "CarDigitalTwins.Telemetry.FileReceiver", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TStrongObjectPtr<UFileTelemetryReceiver> Receiver;

	// Starts the receiver on a test trip of FrameCount frames at RateHz.
	bool StartOnTestTrip(int32 FrameCount, float RateHz)
	{
		Receiver->TripFileRelativePath = FileTelemetryReceiverTestHelpers::WriteTestTrip(TEXT("trip_test.json"),
			FileTelemetryReceiverTestHelpers::MakeTripJson(FrameCount, RateHz));
		return Receiver->StartReceiving();
	}

	// Seq of each sample, in order: exactly ExpectedFirstSeq, +1, +2, ... wrapping to 0 after FramesPerLoop - 1.
	void TestSeqRun(const TCHAR* What, const TArray<FVehicleTelemetry>& Samples, int32 ExpectedCount, int32 ExpectedFirstSeq, int32 FramesPerLoop)
	{
		TestEqual(FString::Printf(TEXT("%s: frame count"), What), Samples.Num(), ExpectedCount);
		for (int32 SampleIndex = 0; SampleIndex < Samples.Num(); ++SampleIndex)
		{
			const int64 ExpectedSeq = (ExpectedFirstSeq + SampleIndex) % FramesPerLoop;
			if (Samples[SampleIndex].Seq != ExpectedSeq)
			{
				AddError(FString::Printf(TEXT("%s: sample %d has seq %lld, expected %lld"), What, SampleIndex, Samples[SampleIndex].Seq, ExpectedSeq));
				return;
			}
		}
	}

	// StartReceiving must fail with a log line matching ExpectedLogPattern, and the receiver must then deliver nothing.
	void TestTripRejected(const TCHAR* What, const FString& TripRelativePath, const FString& ExpectedLogPattern)
	{
		AddExpectedError(ExpectedLogPattern, EAutomationExpectedErrorFlags::Contains, 1, false);
		Receiver->TripFileRelativePath = TripRelativePath;
		TestFalse(FString::Printf(TEXT("%s: StartReceiving fails"), What), Receiver->StartReceiving());
		TestEqual(FString::Printf(TEXT("%s: delivers nothing"), What), FileTelemetryReceiverTestHelpers::PollOnce(*Receiver, 1.f).Num(), 0);
		TestTrue(FString::Printf(TEXT("%s: Idle"), What), Receiver->GetConnectionStatus().ConnectionState == ETelemetryConnectionState::Idle);
	}
END_DEFINE_SPEC(FFileTelemetryReceiverSpec)

void FFileTelemetryReceiverSpec::Define()
{
	using namespace FileTelemetryReceiverTestHelpers;

	BeforeEach([this]()
	{
		Receiver.Reset(NewObject<UFileTelemetryReceiver>());
	});

	AfterEach([this]()
	{
		Receiver->StopReceiving();
		Receiver.Reset();
		IFileManager::Get().DeleteDirectory(*FPaths::Combine(FPaths::ProjectDir(), TestTripFolder), false, true);
	});

	Describe("Loading", [this]()
	{
		It("U1: the committed sample trip loads: 1900 frames at 10 Hz, first frame filled in", [this]()
		{
			Receiver->TripFileRelativePath = TEXT("Data/Trips/trip_sample.json");
			if (!TestTrue(TEXT("StartReceiving"), Receiver->StartReceiving()))
			{
				return;
			}
			// Just under the 190 s loop: every frame once, no wrap yet.
			const TArray<FVehicleTelemetry> AllFrames = PollOnce(*Receiver, 189.95f);
			TestSeqRun(TEXT("whole trip"), AllFrames, 1900, 0, 1900);
			if (AllFrames.Num() < 2)
			{
				return;
			}
			TestTrue(TEXT("10 Hz: frame 1 at 0.1 s"), FMath::IsNearlyEqual(AllFrames[1].SampleTimeS - AllFrames[0].SampleTimeS, 0.1, 1e-6));

			// A misspelt JSON key converts silently to 0, so the fields that can't be 0 in a parked, running car must not be.
			const FVehicleTelemetry& FirstFrame = AllFrames[0];
			TestTrue(TEXT("engine idling"), FirstFrame.EngineRpm > 0.f);
			TestTrue(TEXT("odometer"), FirstFrame.OdometerKm > 0.0);
			TestTrue(TEXT("coolant"), FirstFrame.CoolantTempC > 0.f);
			TestTrue(TEXT("fuel"), FirstFrame.FuelPct > 0.f);
			TestTrue(TEXT("battery"), FirstFrame.BatteryV > 0.f);
			TestTrue(TEXT("all four tyres"), FirstFrame.TyreKpa.FL > 0.f && FirstFrame.TyreKpa.FR > 0.f && FirstFrame.TyreKpa.RL > 0.f
				&& FirstFrame.TyreKpa.RR > 0.f);
		});

		It("U2: every JSON field lands in its own FVehicleTelemetry member", [this]()
		{
			// Distinct values everywhere, so two fields swapped in the mapping can't pass.
			Receiver->TripFileRelativePath = WriteTestTrip(TEXT("trip_fields.json"), FString(TEXT("{\"schemaVersion\": 1, \"vehicleId\": \"test-01\", ")
				TEXT("\"rateHz\": 10, \"seed\": 1, \"frames\": [{\"seq\": 7, \"sampleTimeS\": 0.0, \"speedKmh\": 52.5, \"engineRpm\": 2150, \"gear\": 4, ")
				TEXT("\"throttlePct\": 33.0, \"brakePct\": 4.0, \"steerDeg\": -12.5, \"odometerKm\": 12450.2, \"coolantTempC\": 96.5, \"fuelPct\": 61.0, ")
				TEXT("\"batteryV\": 13.9, \"tyreKpa\": {\"fl\": 231, \"fr\": 232, \"rl\": 233, \"rr\": 234}, \"openings\": 17, \"driveMode\": \"EngineDerate\"}]}")));
			if (!TestTrue(TEXT("StartReceiving"), Receiver->StartReceiving()))
			{
				return;
			}
			const TArray<FVehicleTelemetry> Samples = PollOnce(*Receiver, 0.01f);
			if (!TestEqual(TEXT("one frame"), Samples.Num(), 1))
			{
				return;
			}
			const FVehicleTelemetry& Sample = Samples[0];
			TestEqual(TEXT("seq"), Sample.Seq, int64(7));
			TestEqual(TEXT("speedKmh"), Sample.SpeedKmh, 52.5f);
			TestEqual(TEXT("engineRpm"), Sample.EngineRpm, 2150.f);
			TestEqual(TEXT("gear"), Sample.Gear, 4);
			TestEqual(TEXT("throttlePct"), Sample.ThrottlePct, 33.f);
			TestEqual(TEXT("brakePct"), Sample.BrakePct, 4.f);
			TestEqual(TEXT("steerDeg"), Sample.SteerDeg, -12.5f);
			TestEqual(TEXT("odometerKm"), Sample.OdometerKm, 12450.2, 1e-6);
			TestEqual(TEXT("coolantTempC"), Sample.CoolantTempC, 96.5f);
			TestEqual(TEXT("fuelPct"), Sample.FuelPct, 61.f);
			TestEqual(TEXT("batteryV"), Sample.BatteryV, 13.9f, 1e-4f);
			TestEqual(TEXT("tyreKpa.fl"), Sample.TyreKpa.FL, 231.f);
			TestEqual(TEXT("tyreKpa.fr"), Sample.TyreKpa.FR, 232.f);
			TestEqual(TEXT("tyreKpa.rl"), Sample.TyreKpa.RL, 233.f);
			TestEqual(TEXT("tyreKpa.rr"), Sample.TyreKpa.RR, 234.f);
			TestEqual(TEXT("openings (DoorFL | Hood)"), Sample.Openings, 17);
			TestTrue(TEXT("driveMode"), Sample.DriveMode == EVehicleDriveMode::EngineDerate);
		});

		It("U3: broken trips are rejected with a reason, and the receiver then delivers nothing", [this]()
		{
			TestTripRejected(TEXT("missing file"), FString(TestTripFolder) / TEXT("does_not_exist.json"), TEXT("can't read"));
			TestTripRejected(TEXT("invalid JSON"), WriteTestTrip(TEXT("trip_invalid.json"), TEXT("{\"schemaVersion\": 1, \"frames\": [")),
				TEXT("is not valid JSON"));
			TestTripRejected(TEXT("schemaVersion 2"), WriteTestTrip(TEXT("trip_v2.json"), MakeTripJson(3, 10.f, 2)), TEXT("schemaVersion 2 in"));
			TestTripRejected(TEXT("no schemaVersion"), WriteTestTrip(TEXT("trip_noversion.json"), MakeTripJson(3, 10.f, -1)), TEXT("schemaVersion 0 in"));
			TestTripRejected(TEXT("no frames"), WriteTestTrip(TEXT("trip_empty.json"), MakeTripJson(0, 10.f)), TEXT("has 0 frames"));
			TestTripRejected(TEXT("rateHz 0"), WriteTestTrip(TEXT("trip_rate0.json"), MakeTripJson(3, 0.f)), TEXT("at 0.00 Hz"));

			// Frames 1 and 2 at the same time.
			FString RepeatedTimeJson = MakeTripJson(3, 10.f);
			RepeatedTimeJson.ReplaceInline(TEXT("\"sampleTimeS\": 0.2000"), TEXT("\"sampleTimeS\": 0.1000"));
			TestTripRejected(TEXT("sampleTimeS not increasing"), WriteTestTrip(TEXT("trip_time.json"), RepeatedTimeJson), TEXT("is not after frame"));
		});
	});

	Describe("Playback", [this]()
	{
		It("U4: 60 fps for just under 1 s delivers frames 0-9 in order, at most one per poll", [this]()
		{
			if (!TestTrue(TEXT("StartReceiving"), StartOnTestTrip(100, 10.f)))
			{
				return;
			}
			// 59 polls = 0.983 s: frames 0.0 to 0.9 s are due, 1.0 s isn't yet.
			int32 MostFramesInOnePoll = 0;
			TArray<FVehicleTelemetry> DeliveredSamples;
			for (int32 PollIndex = 0; PollIndex < 59; ++PollIndex)
			{
				const TArray<FVehicleTelemetry> NewSamples = PollOnce(*Receiver, 1.f / 60.f);
				MostFramesInOnePoll = FMath::Max(MostFramesInOnePoll, NewSamples.Num());
				DeliveredSamples.Append(NewSamples);
			}
			TestSeqRun(TEXT("first second"), DeliveredSamples, 10, 0, 100);
			TestEqual(TEXT("10 Hz at 60 fps: never two frames in one poll"), MostFramesInOnePoll, 1);
		});

		It("U5: one hitch of almost 5 s delivers all 50 frames due, none skipped", [this]()
		{
			if (!TestTrue(TEXT("StartReceiving"), StartOnTestTrip(100, 10.f)))
			{
				return;
			}
			TestSeqRun(TEXT("after the hitch"), PollOnce(*Receiver, 4.95f), 50, 0, 100);
			TestSeqRun(TEXT("next poll carries on"), PollOnce(*Receiver, 0.1f), 1, 50, 100);
		});

		It("U6: the last frame is held one period, then frame 0 again; loops don't drift", [this]()
		{
			// 5 frames at 10 Hz: frames at 0.0-0.4 s, 0.5 s loop.
			if (!TestTrue(TEXT("StartReceiving"), StartOnTestTrip(5, 10.f)))
			{
				return;
			}
			TestSeqRun(TEXT("first loop"), PollOnce(*Receiver, 0.45f), 5, 0, 5);
			TestEqual(TEXT("last frame still shown at 0.49 s"), PollOnce(*Receiver, 0.04f).Num(), 0);
			TestSeqRun(TEXT("wrap at 0.5 s"), PollOnce(*Receiver, 0.02f), 1, 0, 5);

			// From the start: 3 loops in 60 fps steps (just under 1.5 s) = 15 frames; 100 loops (just under 50 s) = 500.
			StartOnTestTrip(5, 10.f);
			TestSeqRun(TEXT("3 loops"), PollSteps(*Receiver, 60.f, 89), 15, 0, 5);
			StartOnTestTrip(5, 10.f);
			TestSeqRun(TEXT("100 loops at 60 fps"), PollSteps(*Receiver, 60.f, 2998), 500, 0, 5);
		});

		It("U7: the speed multiplier scales delivery, <= 0 pauses, and a change applies from the next poll", [this]()
		{
			if (!TestTrue(TEXT("StartReceiving"), StartOnTestTrip(300, 10.f)))
			{
				return;
			}
			Receiver->PlaybackSpeedMultiplier = 10.f;
			TestSeqRun(TEXT("x10 for 0.995 s"), PollOnce(*Receiver, 0.995f), 100, 0, 300);

			Receiver->PlaybackSpeedMultiplier = 0.f;
			TestEqual(TEXT("x0 delivers nothing"), PollOnce(*Receiver, 1.f).Num(), 0);
			Receiver->PlaybackSpeedMultiplier = -1.f;
			TestEqual(TEXT("negative delivers nothing (no rewind)"), PollOnce(*Receiver, 1.f).Num(), 0);

			// Restart: 0.45 s at x1 = frames 0-4, then 0.5 s at x2 = trip time 1.45 s, frames 5-14.
			StartOnTestTrip(300, 10.f);
			Receiver->PlaybackSpeedMultiplier = 1.f;
			TestSeqRun(TEXT("x1"), PollOnce(*Receiver, 0.45f), 5, 0, 300);
			Receiver->PlaybackSpeedMultiplier = 2.f;
			TestSeqRun(TEXT("then x2"), PollOnce(*Receiver, 0.5f), 10, 5, 300);
		});

		It("U8: nothing before start or after stop; double stop is safe; a restart begins at frame 0", [this]()
		{
			Receiver->TripFileRelativePath = WriteTestTrip(TEXT("trip_test.json"), MakeTripJson(100, 10.f));
			TestEqual(TEXT("poll before start"), PollOnce(*Receiver, 1.f).Num(), 0);
			TestTrue(TEXT("Idle before start"), Receiver->GetConnectionStatus().ConnectionState == ETelemetryConnectionState::Idle);

			if (!TestTrue(TEXT("StartReceiving"), Receiver->StartReceiving()))
			{
				return;
			}
			TestTrue(TEXT("Live while playing"), Receiver->GetConnectionStatus().ConnectionState == ETelemetryConnectionState::Live);
			TestSeqRun(TEXT("playing"), PollOnce(*Receiver, 0.25f), 3, 0, 100);

			Receiver->StopReceiving();
			Receiver->StopReceiving();
			TestTrue(TEXT("Idle after stop"), Receiver->GetConnectionStatus().ConnectionState == ETelemetryConnectionState::Idle);
			TestEqual(TEXT("poll after stop"), PollOnce(*Receiver, 1.f).Num(), 0);

			TestTrue(TEXT("restart"), Receiver->StartReceiving());
			TestSeqRun(TEXT("restart begins at frame 0"), PollOnce(*Receiver, 0.05f), 1, 0, 100);
		});
	});

	Describe("Helpers", [this]()
	{
		It("U9: loop length, opening bits, display name, command refusal and the configured settings", [this]()
		{
			FRecordedTripFile RecordedTrip;
			RecordedTrip.Frames.SetNum(1900);
			RecordedTrip.RateHz = 10.f;
			TestEqual(TEXT("1900 frames at 10 Hz = 190 s"), RecordedTrip.GetLoopDurationSeconds(), 190.0, 1e-9);
			RecordedTrip.RateHz = 0.f;
			TestEqual(TEXT("rate 0 = 0 s, no division by zero"), RecordedTrip.GetLoopDurationSeconds(), 0.0);

			const EVehicleOpening Openings[] = { EVehicleOpening::DoorFL, EVehicleOpening::DoorFR, EVehicleOpening::DoorRL,
				EVehicleOpening::DoorRR, EVehicleOpening::Hood, EVehicleOpening::Tailgate };
			for (const EVehicleOpening Opening : Openings)
			{
				FVehicleTelemetry Sample;
				Sample.Openings = static_cast<int32>(Opening);
				int32 OpenCount = 0;
				for (const EVehicleOpening OtherOpening : Openings)
				{
					OpenCount += Sample.IsOpen(OtherOpening) ? 1 : 0;
				}
				TestTrue(FString::Printf(TEXT("bit %d: its own opening is open"), static_cast<int32>(Opening)), Sample.IsOpen(Opening));
				TestEqual(FString::Printf(TEXT("bit %d: only that one"), static_cast<int32>(Opening)), OpenCount, 1);
			}

			Receiver->TripFileRelativePath = TEXT("Data/Trips/trip_sample.json");
			TestEqual(TEXT("display name"), Receiver->GetReceiverDisplayName(), FString(TEXT("File trip_sample.json")));
			FVehicleCommand Command;
			FString FailureReason;
			TestFalse(TEXT("a recorded trip refuses commands"), Receiver->SendVehicleCommand(Command, FailureReason));
			TestFalse(TEXT("with a reason"), FailureReason.IsEmpty());

			// The settings as configured on this machine must be usable, whatever the source is set to.
			const UTelemetrySettings* TelemetrySettings = GetDefault<UTelemetrySettings>();
			TestTrue(TEXT("trip file exists"), FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), TelemetrySettings->TripFileRelativePath)));
			TestTrue(TEXT("playback speed in 0.1-50"), TelemetrySettings->PlaybackSpeedMultiplier >= 0.1f && TelemetrySettings->PlaybackSpeedMultiplier <= 50.f);
			TestTrue(TEXT("relay URL is ws://"), TelemetrySettings->RelayUrl.StartsWith(TEXT("ws://")));
			TestTrue(TEXT("stale threshold > 0"), TelemetrySettings->StaleAfterSeconds > 0.f);
			TestTrue(TEXT("reconnect delay grows up to its maximum"),
				TelemetrySettings->ReconnectInitialDelaySeconds <= TelemetrySettings->ReconnectMaxDelaySeconds);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
