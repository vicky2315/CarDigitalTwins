// The telemetry link and the command path for the dashboards: connection state, latency, lost messages, time since the last
// sample, and the latest command with its outcome (docs/SPEC.md §4, §2.5). Commands go down through RequestEngineDerate; the
// result comes back up as LatestVehicleCommand and, for the effect, as DriveMode in the telemetry ViewModel (SPEC.md §5).

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ViewModelBase.h"
#include "Telemetry/TelemetryReceiver.h"
#include "Telemetry/TelemetrySubsystem.h"
#include "ConnectionViewModel.generated.h"

UENUM(BlueprintType)
enum class EConnectionViewModelField : uint8
{
	ConnectionState,
	AverageReceiveLatencyMs,
	DroppedMessageCount,
	SecondsSinceLastSample,
	SecondsUntilReconnectAttempt,
	ReceiverDisplayName,
	LatestVehicleCommand,
	Count UMETA(Hidden),
};
static_assert(static_cast<int32>(EConnectionViewModelField::Count) <= FViewModelFieldMask::MaxFieldCount,
	"EConnectionViewModelField has more values than the field mask has bits");

UCLASS()
class UConnectionViewModel : public UViewModelBase
{
	GENERATED_BODY()

public:
	//~ UViewModelBase
	virtual int32 GetFieldCount() const override { return static_cast<int32>(EConnectionViewModelField::Count); }
	virtual void InitializeViewModel(UGameInstance& OwningGameInstance) override;
	virtual void DeinitializeViewModel() override;
	// Polls the receiver's connection status: it changes without a sample arriving (Stale, Disconnected, the reconnect countdown).
	virtual void UpdateViewModel(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Connection")
	ETelemetryConnectionState GetConnectionState() const { return ConnectionState; }

	UFUNCTION(BlueprintPure, Category = "Connection")
	float GetAverageReceiveLatencyMs() const { return AverageReceiveLatencyMs; }

	UFUNCTION(BlueprintPure, Category = "Connection")
	int64 GetDroppedMessageCount() const { return DroppedMessageCount; }

	// Seconds since the last sample arrived; 0 before the first one.
	UFUNCTION(BlueprintPure, Category = "Connection")
	float GetSecondsSinceLastSample() const { return SecondsSinceLastSample; }

	UFUNCTION(BlueprintPure, Category = "Connection")
	float GetSecondsUntilReconnectAttempt() const { return SecondsUntilReconnectAttempt; }

	// "WebSocket ws://127.0.0.1:8765" or "File trip_sample.json".
	UFUNCTION(BlueprintPure, Category = "Connection")
	FString GetReceiverDisplayName() const { return ReceiverDisplayName; }

	UFUNCTION(BlueprintPure, Category = "Connection")
	const FVehicleCommandRecord& GetLatestVehicleCommand() const { return LatestVehicleCommand; }

	// Command from the dashboard: asks the vehicle to switch engine protection derate on or off.
	UFUNCTION(BlueprintCallable, Category = "Connection")
	void RequestEngineDerate(bool bEnabled);

private:
	void HandleTelemetryUpdated(const FVehicleTelemetry& NewTelemetrySample);
	void HandleVehicleCommandUpdated(const FVehicleCommandRecord& UpdatedCommandRecord);

	TWeakObjectPtr<UTelemetrySubsystem> SubscribedTelemetrySubsystem;
	FDelegateHandle TelemetryUpdatedDelegateHandle;
	FDelegateHandle VehicleCommandUpdatedDelegateHandle;

	// FPlatformTime::Seconds of the last sample; 0 = none yet.
	double LastSampleArrivalTimeSeconds = 0.0;

	ETelemetryConnectionState ConnectionState = ETelemetryConnectionState::Idle;
	float AverageReceiveLatencyMs = 0.f;
	int64 DroppedMessageCount = 0;
	float SecondsSinceLastSample = 0.f;
	float SecondsUntilReconnectAttempt = 0.f;
	FString ReceiverDisplayName;
	FVehicleCommandRecord LatestVehicleCommand;
};
