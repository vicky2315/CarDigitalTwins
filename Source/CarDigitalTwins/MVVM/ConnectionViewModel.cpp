#include "MVVM/ConnectionViewModel.h"

#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"

namespace ConnectionChangeTolerances
{
	static constexpr float LatencyMs = 0.5f;            // shown to 1 ms
	static constexpr float CountdownSeconds = 0.05f;    // "no data 3.2 s", "retry in 1.6 s": shown to 0.1 s
}

void UConnectionViewModel::InitializeViewModel(UGameInstance& OwningGameInstance)
{
	UTelemetrySubsystem* TelemetrySubsystem = OwningGameInstance.GetSubsystem<UTelemetrySubsystem>();
	if (!TelemetrySubsystem)
	{
		return;
	}
	SubscribedTelemetrySubsystem = TelemetrySubsystem;
	TelemetryUpdatedDelegateHandle = TelemetrySubsystem->OnTelemetryUpdated.AddUObject(this, &UConnectionViewModel::HandleTelemetryUpdated);
	VehicleCommandUpdatedDelegateHandle = TelemetrySubsystem->OnVehicleCommandUpdated.AddUObject(this, &UConnectionViewModel::HandleVehicleCommandUpdated);
	if (TelemetrySubsystem->HasReceivedAnyTelemetrySample())
	{
		LastSampleArrivalTimeSeconds = FPlatformTime::Seconds();
	}
	HandleVehicleCommandUpdated(TelemetrySubsystem->GetLatestVehicleCommandRecord());
	UpdateViewModel(0.f);
}

void UConnectionViewModel::DeinitializeViewModel()
{
	if (UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		TelemetrySubsystem->OnTelemetryUpdated.Remove(TelemetryUpdatedDelegateHandle);
		TelemetrySubsystem->OnVehicleCommandUpdated.Remove(VehicleCommandUpdatedDelegateHandle);
	}
	SubscribedTelemetrySubsystem.Reset();
}

void UConnectionViewModel::UpdateViewModel(float DeltaSeconds)
{
	const UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get();
	if (!TelemetrySubsystem)
	{
		return;
	}

	using ConnectionField = EConnectionViewModelField;
	namespace ChangeTolerance = ConnectionChangeTolerances;
	const FTelemetryConnectionStatus ConnectionStatus = TelemetrySubsystem->GetTelemetryConnectionStatus();
	SetField(ConnectionField::ConnectionState, ConnectionState, ConnectionStatus.ConnectionState);
	SetField(ConnectionField::AverageReceiveLatencyMs, AverageReceiveLatencyMs, ConnectionStatus.AverageReceiveLatencyMs, ChangeTolerance::LatencyMs);
	SetField(ConnectionField::DroppedMessageCount, DroppedMessageCount, ConnectionStatus.DroppedMessageCount);
	SetField(ConnectionField::SecondsUntilReconnectAttempt, SecondsUntilReconnectAttempt, ConnectionStatus.SecondsUntilReconnectAttempt,
		ChangeTolerance::CountdownSeconds);
	SetField(ConnectionField::ReceiverDisplayName, ReceiverDisplayName, TelemetrySubsystem->GetActiveReceiverDisplayName());

	const float NewSecondsSinceLastSample = LastSampleArrivalTimeSeconds > 0.0
		? static_cast<float>(FPlatformTime::Seconds() - LastSampleArrivalTimeSeconds) : 0.f;
	SetField(ConnectionField::SecondsSinceLastSample, SecondsSinceLastSample, NewSecondsSinceLastSample, ChangeTolerance::CountdownSeconds);
}

void UConnectionViewModel::RequestEngineDerate(bool bEnabled)
{
	if (UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		// The outcome comes back through OnVehicleCommandUpdated; nothing here changes what the dashboard shows (SPEC.md §5).
		TelemetrySubsystem->SendEngineDerateCommand(bEnabled, TEXT("operator"), TEXT("dashboard"));
	}
}

void UConnectionViewModel::HandleTelemetryUpdated(const FVehicleTelemetry& NewTelemetrySample)
{
	LastSampleArrivalTimeSeconds = FPlatformTime::Seconds();
}

void UConnectionViewModel::HandleVehicleCommandUpdated(const FVehicleCommandRecord& UpdatedCommandRecord)
{
	if (const UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		SetField(EConnectionViewModelField::LatestVehicleCommand, LatestVehicleCommand, TelemetrySubsystem->GetLatestVehicleCommandRecord());
	}
}
