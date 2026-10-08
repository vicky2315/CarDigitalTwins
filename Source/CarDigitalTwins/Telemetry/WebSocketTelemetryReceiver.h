// Live telemetry from Tools/Relay/relay.py over WebSocket (docs/SPEC.md §2.3 messages, §4 connection states). Day 10.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TelemetryReceiver.h"
#include "WebSocketTelemetryReceiver.generated.h"

class IWebSocket;

// Counts gaps and latency over one connection. Knows nothing about sockets, so tests can feed it Seq values directly.
struct FTelemetryStreamStatistics
{
	// Records one telemetry message. Returns how many messages were missing just before it: 0 when in order, and 0 when Seq went
	// backwards (trip looped, SPEC.md §2.3).
	int64 RecordTelemetryMessage(int64 Seq, double ReceiveLatencyMs);

	// New connection: forget the last Seq, so downtime between connections doesn't count as dropped messages.
	void Reset() { *this = FTelemetryStreamStatistics(); }

	double GetAverageReceiveLatencyMs() const { return ReceivedMessageCount > 0 ? SumOfReceiveLatenciesMs / ReceivedMessageCount : 0.0; }

	int64 ReceivedMessageCount = 0;
	int64 DroppedMessageCount = 0;
	double LatestReceiveLatencyMs = 0.0;
	double SumOfReceiveLatenciesMs = 0.0;
	double MaxReceiveLatencyMs = 0.0;
	TOptional<int64> LastSeq;
};

UCLASS()
class UWebSocketTelemetryReceiver : public UObject, public ITelemetryReceiver
{
	GENERATED_BODY()

public:
	// Set by UTelemetrySubsystem from UTelemetrySettings before StartReceiving, so this class doesn't depend on the settings.
	FString RelayUrl;
	float StaleAfterSeconds = 1.f;
	float ConnectTimeoutSeconds = 5.f;
	float ReconnectInitialDelaySeconds = 0.5f;
	float ReconnectMaxDelaySeconds = 10.f;

	//~ ITelemetryReceiver
	virtual bool StartReceiving() override;
	virtual void StopReceiving() override;
	virtual void PollNewTelemetrySamples(float DeltaSeconds, TArray<FVehicleTelemetry>& OutNewTelemetrySamples) override;
	virtual FString GetReceiverDisplayName() const override;
	virtual FTelemetryConnectionStatus GetConnectionStatus() const override;

	//~ UObject
	virtual void BeginDestroy() override;

	// Wait before reconnect attempt ReconnectAttemptNumber (1 = first): initial × 2^(attempt − 1), capped at the maximum, then spread by
	// ±20 % (JitterUnit in −1..1) so many clients that lost the relay together don't all retry in the same instant.
	static double ComputeReconnectDelaySeconds(int32 ReconnectAttemptNumber, double InitialDelaySeconds, double MaxDelaySeconds, double JitterUnit);

private:
	enum class ESocketEventType : uint8
	{
		Connected,
		ConnectionError,
		Closed,
		Message,
	};

	// What a socket callback reported, kept until the next poll. SocketGeneration tells events of an old, replaced socket apart.
	struct FQueuedSocketEvent
	{
		ESocketEventType SocketEventType;
		FString EventText;
		int32 SocketGeneration;
	};

	// Creates a new socket, binds its callbacks and starts connecting.
	void OpenSocket();

	// Unbinds the callbacks first (so a closing socket can't queue more events), then closes and releases the socket.
	void CloseSocket();

	// Socket callbacks: only queue the event, so all state changes happen on the game thread in PollNewTelemetrySamples.
	void HandleSocketConnected(int32 SocketGeneration);
	void HandleSocketConnectionError(const FString& ConnectionErrorText, int32 SocketGeneration);
	void HandleSocketClosed(int32 CloseStatusCode, const FString& CloseReason, bool bWasClean, int32 SocketGeneration);
	void HandleSocketMessage(const FString& RelayMessageText, int32 SocketGeneration);
	void QueueSocketEvent(ESocketEventType SocketEventType, FString EventText, int32 SocketGeneration);

	void ProcessSocketEvent(const FQueuedSocketEvent& SocketEvent, TArray<FVehicleTelemetry>& OutNewTelemetrySamples);

	// Parses one relay message: hello is remembered, telemetry becomes a sample, unknown types are ignored (SPEC.md §2.3).
	void ProcessRelayMessage(const FString& RelayMessageText, TArray<FVehicleTelemetry>& OutNewTelemetrySamples);

	// Closes the socket and schedules the next attempt with backoff.
	void HandleConnectionLost(const FString& LostReason);

	// Connect timeout, Live → Stale, and the reconnect timer.
	void UpdateConnectionTimers(double NowSeconds);

	void SetConnectionState(ETelemetryConnectionState NewConnectionState, const FString& StateChangeReason);

	// StaleAfterSeconds, but at least three message intervals from the relay's hello, so a slow --rate doesn't flicker to Stale.
	double GetEffectiveStaleAfterSeconds() const;

	static double GetCurrentUnixMs();

	TSharedPtr<IWebSocket> RelayWebSocket;
	int32 CurrentSocketGeneration = 0;

	FCriticalSection QueuedSocketEventsLock;
	TArray<FQueuedSocketEvent> QueuedSocketEvents;

	// Swapped with QueuedSocketEvents each poll, so the lock is held only for the swap.
	TArray<FQueuedSocketEvent> SocketEventsBeingProcessed;

	ETelemetryConnectionState ConnectionState = ETelemetryConnectionState::Idle;
	FTelemetryStreamStatistics StreamStatistics;

	// Failed attempts since the last successful connection (drives the backoff), and all attempts since StartReceiving.
	int32 ConsecutiveFailedConnectAttemptCount = 0;
	int32 TotalReconnectAttemptCount = 0;

	// FPlatformTime::Seconds values.
	double ConnectStartedTimeSeconds = 0.0;
	double LastTelemetryArrivalTimeSeconds = 0.0;
	double NextReconnectTimeSeconds = 0.0;

	// From the relay's hello; 0 until one arrives.
	double RelayMessagesPerSecond = 0.0;

	int64 RejectedMessageCount = 0;
	bool bIsReceiving = false;
};
