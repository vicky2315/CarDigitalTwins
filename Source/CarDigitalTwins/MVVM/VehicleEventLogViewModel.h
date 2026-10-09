// What happened during the trip, for the events list and the alarm banner: status changes per signal, drive mode changes, doors
// and hood, commands and their outcomes, and the Critical coolant alarm with its acknowledge state.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ViewModelBase.h"
#include "Telemetry/TelemetrySubsystem.h"
#include "Telemetry/VehicleStatusEvaluator.h"
#include "Telemetry/VehicleTelemetry.h"
#include "VehicleEventLogViewModel.generated.h"

UENUM(BlueprintType)
enum class EVehicleEventSeverity : uint8
{
	// Something went back to Normal, a door opened, the trip restarted.
	Information,
	Warning,
	Critical,
	// A command was sent, applied, or the drive mode changed.
	Command,
};

USTRUCT(BlueprintType)
struct FVehicleEventLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	double TripSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	EVehicleEventSeverity Severity = EVehicleEventSeverity::Information;

	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	FString EventMessage;
};

// The one alarm the dashboard raises: Critical coolant. Active while the coolant stays Critical; flashing until acknowledged.
USTRUCT(BlueprintType)
struct FVehicleAlarm
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	bool bIsAcknowledged = false;

	// "CRITICAL · COOLANT 115.0 °C"
	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	FString AlarmTitle;

	// "Engine overheating" or "Engine derate requested automatically"
	UPROPERTY(BlueprintReadOnly, Category = "Event Log")
	FString AlarmDetail;

	bool operator==(const FVehicleAlarm& OtherAlarm) const
	{
		return bIsActive == OtherAlarm.bIsActive && bIsAcknowledged == OtherAlarm.bIsAcknowledged
			&& AlarmTitle.Equals(OtherAlarm.AlarmTitle, ESearchCase::CaseSensitive) && AlarmDetail.Equals(OtherAlarm.AlarmDetail, ESearchCase::CaseSensitive);
	}
};

UENUM(BlueprintType)
enum class EVehicleEventLogViewModelField : uint8
{
	RecentEvents,
	ActiveAlarm,
	Count UMETA(Hidden),
};

UCLASS()
class UVehicleEventLogViewModel : public UViewModelBase
{
	GENERATED_BODY()

public:
	//~ UViewModelBase
	virtual int32 GetFieldCount() const override { return static_cast<int32>(EVehicleEventLogViewModelField::Count); }
	virtual void InitializeViewModel(UGameInstance& OwningGameInstance) override;
	virtual void DeinitializeViewModel() override;

	// Newest first, at most MaxKeptEventCount.
	UFUNCTION(BlueprintPure, Category = "Event Log")
	const TArray<FVehicleEventLogEntry>& GetRecentEvents() const { return RecentEvents; }

	UFUNCTION(BlueprintPure, Category = "Event Log")
	const FVehicleAlarm& GetActiveAlarm() const { return ActiveAlarm; }

	// Command from the alarm banner: stops the flashing. The alarm stays until the coolant leaves Critical.
	UFUNCTION(BlueprintCallable, Category = "Event Log")
	void AcknowledgeAlarm();

	static constexpr int32 MaxKeptEventCount = 40;

private:
	void HandleTelemetryUpdated(const FVehicleTelemetry& NewTelemetrySample);
	void HandleVehicleCommandUpdated(const FVehicleCommandRecord& UpdatedCommandRecord);
	void AddEvent(double TripSeconds, EVehicleEventSeverity Severity, FString EventMessage);
	void LogSignalStatusChange(const TCHAR* SignalName, EVehicleStatus PreviousStatus, EVehicleStatus NewStatus, const FString& ValueText,
		double TripSeconds);

	TWeakObjectPtr<UTelemetrySubsystem> SubscribedTelemetrySubsystem;
	FDelegateHandle TelemetryUpdatedDelegateHandle;
	FDelegateHandle VehicleCommandUpdatedDelegateHandle;

	// The sample and status before the current one, to see what changed.
	bool bHasPreviousSample = false;
	FVehicleTelemetry PreviousTelemetrySample;
	FVehicleStatusReport PreviousVehicleStatusReport;

	TArray<FVehicleEventLogEntry> RecentEvents;
	FVehicleAlarm ActiveAlarm;
};