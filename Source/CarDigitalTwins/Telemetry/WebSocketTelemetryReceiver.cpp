#include "Telemetry/WebSocketTelemetryReceiver.h"

#include "Dom/JsonObject.h"
#include "HAL/PlatformTime.h"
#include "IWebSocket.h"
#include "JsonObjectConverter.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "WebSocketsModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTelemetry, Log, All);

namespace WebSocketTelemetryReceiverConstants
{
	static constexpr double ReconnectJitterFraction = 0.2;
	static constexpr double MinStaleMessageIntervals = 3.0;
}

int64 FTelemetryStreamStatistics::RecordTelemetryMessage(int64 Seq, double ReceiveLatencyMs)
{
	int64 MissingMessageCount = 0;
	if (LastSeq.IsSet() && Seq > LastSeq.GetValue() + 1)
	{
		MissingMessageCount = Seq - LastSeq.GetValue() - 1;
	}
	DroppedMessageCount += MissingMessageCount;
	LastSeq = Seq;

	++ReceivedMessageCount;
	LatestReceiveLatencyMs = ReceiveLatencyMs;
	SumOfReceiveLatenciesMs += ReceiveLatencyMs;
	MaxReceiveLatencyMs = FMath::Max(MaxReceiveLatencyMs, ReceiveLatencyMs);
	return MissingMessageCount;
}

double UWebSocketTelemetryReceiver::ComputeReconnectDelaySeconds(int32 ReconnectAttemptNumber, double InitialDelaySeconds, double MaxDelaySeconds,
	double JitterUnit)
{
	// Clamped exponent: 2^30 × any sane initial delay is far past the maximum anyway, and it can't overflow.
	const int32 DoublingCount = FMath::Clamp(ReconnectAttemptNumber - 1, 0, 30);
	const double DelayBeforeJitterSeconds = FMath::Min(InitialDelaySeconds * FMath::Pow(2.0, DoublingCount), MaxDelaySeconds);
	return DelayBeforeJitterSeconds * (1.0 + WebSocketTelemetryReceiverConstants::ReconnectJitterFraction * FMath::Clamp(JitterUnit, -1.0, 1.0));
}

bool UWebSocketTelemetryReceiver::StartReceiving()
{
	StopReceiving();

	if (!RelayUrl.StartsWith(TEXT("ws://")) && !RelayUrl.StartsWith(TEXT("wss://")))
	{
		UE_LOG(LogVehicleTelemetry, Error, TEXT("%s: RelayUrl must start with ws:// or wss://. Check Project Settings > Vehicle Telemetry."),
			*GetReceiverDisplayName());
		return false;
	}

	// The WebSockets module isn't loaded by default.
	FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));

	StreamStatistics.Reset();
	ConsecutiveFailedConnectAttemptCount = 0;
	TotalReconnectAttemptCount = 0;
	RelayMessagesPerSecond = 0.0;
	RejectedMessageCount = 0;
	bIsReceiving = true;

	// A relay that isn't running yet is not a start failure: the receiver keeps retrying with backoff until it appears.
	OpenSocket();
	return true;
}

void UWebSocketTelemetryReceiver::StopReceiving()
{
	bIsReceiving = false;
	CloseSocket();
	{
		FScopeLock QueuedSocketEventsScopeLock(&QueuedSocketEventsLock);
		QueuedSocketEvents.Reset();
	}
	SetConnectionState(ETelemetryConnectionState::Idle, TEXT("stopped"));
}

void UWebSocketTelemetryReceiver::BeginDestroy()
{
	CloseSocket();
	Super::BeginDestroy();
}

void UWebSocketTelemetryReceiver::PollNewTelemetrySamples(float DeltaSeconds, TArray<FVehicleTelemetry>& OutNewTelemetrySamples)
{
	if (!bIsReceiving)
	{
		return;
	}

	{
		FScopeLock QueuedSocketEventsScopeLock(&QueuedSocketEventsLock);
		Swap(QueuedSocketEvents, SocketEventsBeingProcessed);
	}
	for (const FQueuedSocketEvent& SocketEvent : SocketEventsBeingProcessed)
	{
		// Events from a socket that was already replaced (e.g. a late "connected" after a connect timeout) are stale news.
		if (SocketEvent.SocketGeneration == CurrentSocketGeneration)
		{
			ProcessSocketEvent(SocketEvent, OutNewTelemetrySamples);
		}
	}
	SocketEventsBeingProcessed.Reset();

	UpdateConnectionTimers(FPlatformTime::Seconds());
}

FString UWebSocketTelemetryReceiver::GetReceiverDisplayName() const
{
	return FString::Printf(TEXT("WebSocket %s"), *RelayUrl);
}

FTelemetryConnectionStatus UWebSocketTelemetryReceiver::GetConnectionStatus() const
{
	FTelemetryConnectionStatus WebSocketConnectionStatus;
	WebSocketConnectionStatus.ConnectionState = ConnectionState;
	WebSocketConnectionStatus.DroppedMessageCount = StreamStatistics.DroppedMessageCount;
	WebSocketConnectionStatus.LatestReceiveLatencyMs = static_cast<float>(StreamStatistics.LatestReceiveLatencyMs);
	WebSocketConnectionStatus.AverageReceiveLatencyMs = static_cast<float>(StreamStatistics.GetAverageReceiveLatencyMs());
	WebSocketConnectionStatus.MaxReceiveLatencyMs = static_cast<float>(StreamStatistics.MaxReceiveLatencyMs);
	WebSocketConnectionStatus.ReconnectAttemptCount = TotalReconnectAttemptCount;
	return WebSocketConnectionStatus;
}

void UWebSocketTelemetryReceiver::OpenSocket()
{
	CloseSocket();

	++CurrentSocketGeneration;
	RelayWebSocket = FWebSocketsModule::Get().CreateWebSocket(RelayUrl);
	RelayWebSocket->OnConnected().AddUObject(this, &UWebSocketTelemetryReceiver::HandleSocketConnected, CurrentSocketGeneration);
	RelayWebSocket->OnConnectionError().AddUObject(this, &UWebSocketTelemetryReceiver::HandleSocketConnectionError, CurrentSocketGeneration);
	RelayWebSocket->OnClosed().AddUObject(this, &UWebSocketTelemetryReceiver::HandleSocketClosed, CurrentSocketGeneration);
	RelayWebSocket->OnMessage().AddUObject(this, &UWebSocketTelemetryReceiver::HandleSocketMessage, CurrentSocketGeneration);

	ConnectStartedTimeSeconds = FPlatformTime::Seconds();
	SetConnectionState(ETelemetryConnectionState::Connecting, RelayUrl);
	RelayWebSocket->Connect();
}

void UWebSocketTelemetryReceiver::CloseSocket()
{
	if (!RelayWebSocket.IsValid())
	{
		return;
	}

	RelayWebSocket->OnConnected().RemoveAll(this);
	RelayWebSocket->OnConnectionError().RemoveAll(this);
	RelayWebSocket->OnClosed().RemoveAll(this);
	RelayWebSocket->OnMessage().RemoveAll(this);
	if (RelayWebSocket->IsConnected())
	{
		RelayWebSocket->Close();
	}
	RelayWebSocket.Reset();
}

void UWebSocketTelemetryReceiver::HandleSocketConnected(int32 SocketGeneration)
{
	QueueSocketEvent(ESocketEventType::Connected, FString(), SocketGeneration);
}

void UWebSocketTelemetryReceiver::HandleSocketConnectionError(const FString& ConnectionErrorText, int32 SocketGeneration)
{
	QueueSocketEvent(ESocketEventType::ConnectionError, ConnectionErrorText, SocketGeneration);
}

void UWebSocketTelemetryReceiver::HandleSocketClosed(int32 CloseStatusCode, const FString& CloseReason, bool bWasClean, int32 SocketGeneration)
{
	QueueSocketEvent(ESocketEventType::Closed,
		FString::Printf(TEXT("code %d%s%s"), CloseStatusCode, CloseReason.IsEmpty() ? TEXT("") : *(TEXT(", ") + CloseReason),
			bWasClean ? TEXT("") : TEXT(", not clean")), SocketGeneration);
}

void UWebSocketTelemetryReceiver::HandleSocketMessage(const FString& RelayMessageText, int32 SocketGeneration)
{
	QueueSocketEvent(ESocketEventType::Message, RelayMessageText, SocketGeneration);
}

void UWebSocketTelemetryReceiver::QueueSocketEvent(ESocketEventType SocketEventType, FString EventText, int32 SocketGeneration)
{
	// Locked because the socket may call from a thread other than the game thread, depending on the platform's WebSocket backend.
	FScopeLock QueuedSocketEventsScopeLock(&QueuedSocketEventsLock);
	QueuedSocketEvents.Add({ SocketEventType, MoveTemp(EventText), SocketGeneration });
}

void UWebSocketTelemetryReceiver::ProcessSocketEvent(const FQueuedSocketEvent& SocketEvent, TArray<FVehicleTelemetry>& OutNewTelemetrySamples)
{
	switch (SocketEvent.SocketEventType)
	{
	case ESocketEventType::Connected:
		// Downtime between connections isn't counted as dropped messages, and latency starts fresh for the new connection.
		StreamStatistics.Reset();
		ConsecutiveFailedConnectAttemptCount = 0;
		LastTelemetryArrivalTimeSeconds = FPlatformTime::Seconds();
		SetConnectionState(ETelemetryConnectionState::Live, TEXT("connected"));
		break;

	case ESocketEventType::ConnectionError:
		// The WebSocket backend sometimes reports a failed connect with no text at all.
		HandleConnectionLost(FString::Printf(TEXT("can't connect: %s"), SocketEvent.EventText.IsEmpty() ? TEXT("no details") : *SocketEvent.EventText));
		break;

	case ESocketEventType::Closed:
		HandleConnectionLost(FString::Printf(TEXT("closed by the relay (%s)"), *SocketEvent.EventText));
		break;

	case ESocketEventType::Message:
		ProcessRelayMessage(SocketEvent.EventText, OutNewTelemetrySamples);
		break;
	}
}

void UWebSocketTelemetryReceiver::ProcessRelayMessage(const FString& RelayMessageText, TArray<FVehicleTelemetry>& OutNewTelemetrySamples)
{
	TSharedPtr<FJsonObject> RelayMessageObject;
	const TSharedRef<TJsonReader<>> RelayMessageReader = TJsonReaderFactory<>::Create(RelayMessageText);
	if (!FJsonSerializer::Deserialize(RelayMessageReader, RelayMessageObject) || !RelayMessageObject.IsValid())
	{
		// Logged for the first one only: a broken relay would otherwise write ten lines a second.
		if (RejectedMessageCount++ == 0)
		{
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s: message is not a JSON object, ignored (further ones counted silently): %s"),
				*GetReceiverDisplayName(), *RelayMessageText.Left(120));
		}
		return;
	}

	FString RelayMessageType;
	RelayMessageObject->TryGetStringField(TEXT("type"), RelayMessageType);

	if (RelayMessageType == TEXT("hello"))
	{
		FString RelayVehicleId;
		RelayMessageObject->TryGetStringField(TEXT("vehicleId"), RelayVehicleId);
		RelayMessageObject->TryGetNumberField(TEXT("messagesPerSecond"), RelayMessagesPerSecond);
		UE_LOG(LogVehicleTelemetry, Log, TEXT("%s: relay hello: vehicle %s, %.1f messages/s, stale after %.2f s without data."),
			*GetReceiverDisplayName(), *RelayVehicleId, RelayMessagesPerSecond, GetEffectiveStaleAfterSeconds());
		return;
	}

	if (RelayMessageType != TEXT("telemetry"))
	{
		// Unknown types are for newer relays; ignoring them keeps old receivers working (SPEC.md §2.3).
		UE_LOG(LogVehicleTelemetry, Verbose, TEXT("%s: ignored message type '%s'."), *GetReceiverDisplayName(), *RelayMessageType);
		return;
	}

	// Check the version before converting: a different schema could convert without errors into wrong values (SPEC.md §2.4).
	int32 MessageSchemaVersion = 0;
	const TSharedPtr<FJsonObject>* FrameObject = nullptr;
	FVehicleTelemetry ReceivedTelemetrySample;
	if (!RelayMessageObject->TryGetNumberField(TEXT("schemaVersion"), MessageSchemaVersion) || MessageSchemaVersion != VehicleTelemetrySchemaVersion
		|| !RelayMessageObject->TryGetObjectField(TEXT("frame"), FrameObject)
		|| !FJsonObjectConverter::JsonObjectToUStruct(FrameObject->ToSharedRef(), &ReceivedTelemetrySample))
	{
		if (RejectedMessageCount++ == 0)
		{
			UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s: telemetry rejected (schemaVersion %d, expected %d, or no valid frame); further ones counted silently."),
				*GetReceiverDisplayName(), MessageSchemaVersion, VehicleTelemetrySchemaVersion);
		}
		return;
	}

	double SentUnixMs = 0.0;
	const double ReceiveLatencyMs = RelayMessageObject->TryGetNumberField(TEXT("sentUnixMs"), SentUnixMs) ? GetCurrentUnixMs() - SentUnixMs : 0.0;
	const int64 MissingMessageCount = StreamStatistics.RecordTelemetryMessage(ReceivedTelemetrySample.Seq, ReceiveLatencyMs);
	if (MissingMessageCount > 0)
	{
		UE_LOG(LogVehicleTelemetry, Verbose, TEXT("%s: %lld messages missing before seq %lld."), *GetReceiverDisplayName(), MissingMessageCount,
			ReceivedTelemetrySample.Seq);
	}

	LastTelemetryArrivalTimeSeconds = FPlatformTime::Seconds();
	if (ConnectionState == ETelemetryConnectionState::Stale)
	{
		SetConnectionState(ETelemetryConnectionState::Live, TEXT("telemetry again"));
	}
	OutNewTelemetrySamples.Add(ReceivedTelemetrySample);
}

void UWebSocketTelemetryReceiver::HandleConnectionLost(const FString& LostReason)
{
	CloseSocket();
	++ConsecutiveFailedConnectAttemptCount;
	++TotalReconnectAttemptCount;
	const double ReconnectDelaySeconds = ComputeReconnectDelaySeconds(ConsecutiveFailedConnectAttemptCount, ReconnectInitialDelaySeconds,
		ReconnectMaxDelaySeconds, FMath::FRandRange(-1.0, 1.0));
	NextReconnectTimeSeconds = FPlatformTime::Seconds() + ReconnectDelaySeconds;
	SetConnectionState(ETelemetryConnectionState::Disconnected,
		FString::Printf(TEXT("%s; retry %d in %.1f s"), *LostReason, ConsecutiveFailedConnectAttemptCount, ReconnectDelaySeconds));
}

void UWebSocketTelemetryReceiver::UpdateConnectionTimers(double NowSeconds)
{
	switch (ConnectionState)
	{
	case ETelemetryConnectionState::Connecting:
		if (NowSeconds - ConnectStartedTimeSeconds > ConnectTimeoutSeconds)
		{
			HandleConnectionLost(FString::Printf(TEXT("no answer within %.1f s"), ConnectTimeoutSeconds));
		}
		break;

	case ETelemetryConnectionState::Live:
		if (NowSeconds - LastTelemetryArrivalTimeSeconds > GetEffectiveStaleAfterSeconds())
		{
			SetConnectionState(ETelemetryConnectionState::Stale,
				FString::Printf(TEXT("no telemetry for %.1f s"), NowSeconds - LastTelemetryArrivalTimeSeconds));
		}
		break;

	case ETelemetryConnectionState::Disconnected:
		if (NowSeconds >= NextReconnectTimeSeconds)
		{
			OpenSocket();
		}
		break;

	default:
		break;
	}
}

void UWebSocketTelemetryReceiver::SetConnectionState(ETelemetryConnectionState NewConnectionState, const FString& StateChangeReason)
{
	if (NewConnectionState == ConnectionState)
	{
		return;
	}

	const UEnum* ConnectionStateEnum = StaticEnum<ETelemetryConnectionState>();
	const FString PreviousStateName = ConnectionStateEnum->GetNameStringByValue(static_cast<int64>(ConnectionState));
	const FString NewStateName = ConnectionStateEnum->GetNameStringByValue(static_cast<int64>(NewConnectionState));
	ConnectionState = NewConnectionState;

	// Warning (yellow) when the data stops, so it stands out between the once-a-second summary lines.
	if (NewConnectionState == ETelemetryConnectionState::Stale || NewConnectionState == ETelemetryConnectionState::Disconnected)
	{
		UE_LOG(LogVehicleTelemetry, Warning, TEXT("%s: %s -> %s (%s)."), *GetReceiverDisplayName(), *PreviousStateName, *NewStateName, *StateChangeReason);
	}
	else
	{
		UE_LOG(LogVehicleTelemetry, Log, TEXT("%s: %s -> %s (%s)."), *GetReceiverDisplayName(), *PreviousStateName, *NewStateName, *StateChangeReason);
	}
}

double UWebSocketTelemetryReceiver::GetEffectiveStaleAfterSeconds() const
{
	const double RelayMessageIntervalsSeconds = RelayMessagesPerSecond > 0.0
		? WebSocketTelemetryReceiverConstants::MinStaleMessageIntervals / RelayMessagesPerSecond : 0.0;
	return FMath::Max(static_cast<double>(StaleAfterSeconds), RelayMessageIntervalsSeconds);
}

double UWebSocketTelemetryReceiver::GetCurrentUnixMs()
{
	// Same clock as the relay's time.time(): both run on this machine, so the difference is the transport latency (SPEC.md §2.3).
	return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalMilliseconds();
}
