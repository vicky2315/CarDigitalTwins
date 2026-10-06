#include "Telemetry/TelemetrySubsystem.h"

#include "Engine/GameInstance.h"
#include "Telemetry/FileTelemetryReceiver.h"
#include "Telemetry/TelemetrySettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTelemetry, Log, All);

void UTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Day 7: always the file receiver. Day 10 adds a setting that picks the WebSocket receiver instead.
	const UTelemetrySettings* TelemetrySettings = GetDefault<UTelemetrySettings>();
	UFileTelemetryReceiver* FileTelemetryReceiver = NewObject<UFileTelemetryReceiver>(this);
	FileTelemetryReceiver->TripFileRelativePath = TelemetrySettings->TripFileRelativePath;
	FileTelemetryReceiver->PlaybackSpeedMultiplier = TelemetrySettings->PlaybackSpeedMultiplier;
	ActiveTelemetryReceiver = FileTelemetryReceiver;

	if (ActiveTelemetryReceiver->StartReceiving())
	{
		UE_LOG(LogVehicleTelemetry, Log, TEXT("Telemetry: receiving from %s at %.1fx speed."),
			*ActiveTelemetryReceiver->GetReceiverDisplayName(), TelemetrySettings->PlaybackSpeedMultiplier);
	}
	else
	{
		// The receiver logged why. Keep it so the reason stays visible in one place; it delivers nothing.
		UE_LOG(LogVehicleTelemetry, Error, TEXT("Telemetry: %s failed to start, the car gets no telemetry this session."),
			*ActiveTelemetryReceiver->GetReceiverDisplayName());
	}

	bIsTelemetryTickingEnabled = true;
}

void UTelemetrySubsystem::Deinitialize()
{
	bIsTelemetryTickingEnabled = false;

	if (ActiveTelemetryReceiver)
	{
		ActiveTelemetryReceiver->StopReceiving();
		ActiveTelemetryReceiver = nullptr;
	}
	OnTelemetryUpdated.Clear();

	Super::Deinitialize();
}

void UTelemetrySubsystem::Tick(float DeltaSeconds)
{
	if (!ActiveTelemetryReceiver)
	{
		return;
	}

	NewTelemetrySamplesThisTick.Reset();
	ActiveTelemetryReceiver->PollNewTelemetrySamples(DeltaSeconds, NewTelemetrySamplesThisTick);

	for (const FVehicleTelemetry& NewTelemetrySample : NewTelemetrySamplesThisTick)
	{
		PreviousTelemetrySample = bHasReceivedAnyTelemetrySample ? LatestTelemetrySample : NewTelemetrySample;
		LatestTelemetrySample = NewTelemetrySample;
		bHasReceivedAnyTelemetrySample = true;
		OnTelemetryUpdated.Broadcast(LatestTelemetrySample);
	}

	LogTelemetrySummaryOncePerSecond(DeltaSeconds, NewTelemetrySamplesThisTick);
}

TStatId UTelemetrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTelemetrySubsystem, STATGROUP_Tickables);
}

ETickableTickType UTelemetrySubsystem::GetTickableTickType() const
{
	// The class default object registers as a tickable too (FTickableGameObject's constructor does that); it must never tick.
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool UTelemetrySubsystem::IsTickable() const
{
	return bIsTelemetryTickingEnabled;
}

UWorld* UTelemetrySubsystem::GetTickableGameObjectWorld() const
{
	// Ties ticking to this game instance's world, so with two PIE windows each subsystem ticks once per frame of its own world.
	const UGameInstance* OwningGameInstance = GetGameInstance();
	return OwningGameInstance ? OwningGameInstance->GetWorld() : nullptr;
}

void UTelemetrySubsystem::LogTelemetrySummaryOncePerSecond(float DeltaSeconds, const TArray<FVehicleTelemetry>& NewTelemetrySamples)
{
	if (!NewTelemetrySamples.IsEmpty())
	{
		if (SamplesSinceLastTelemetrySummaryLog == 0)
		{
			FirstSeqSinceLastTelemetrySummaryLog = NewTelemetrySamples[0].Seq;
		}
		SamplesSinceLastTelemetrySummaryLog += NewTelemetrySamples.Num();
	}

	SecondsSinceLastTelemetrySummaryLog += DeltaSeconds;
	if (SecondsSinceLastTelemetrySummaryLog < 1.0)
	{
		return;
	}

	// Seq range reads "1895..4" across a loop: expected, not dropped samples.
	UE_LOG(LogVehicleTelemetry, Log, TEXT("Telemetry: %d samples in %.2f s, seq %lld..%lld, t=%.1f s, %.1f km/h, %.0f rpm, coolant %.1f C, tyre RR %.0f kPa, openings %d."),
		SamplesSinceLastTelemetrySummaryLog, SecondsSinceLastTelemetrySummaryLog, FirstSeqSinceLastTelemetrySummaryLog, LatestTelemetrySample.Seq,
		LatestTelemetrySample.SampleTimeS, LatestTelemetrySample.SpeedKmh, LatestTelemetrySample.EngineRpm, LatestTelemetrySample.CoolantTempC,
		LatestTelemetrySample.TyreKpa.RR, LatestTelemetrySample.Openings);

	SecondsSinceLastTelemetrySummaryLog = 0.0;
	SamplesSinceLastTelemetrySummaryLog = 0;
}