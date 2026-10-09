// A pulsing ring on the part of the car that isn't Normal: amber or red, hidden when Normal (decision D4). The pulse is the WBP's
// PulseAnimation (scale 0.55 → 1.35, fade 1 → 0, 1.6 s, looping), started by C++ when the ring appears.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Telemetry/VehicleTelemetry.h"
#include "TwinPartRingWidget.generated.h"

class UImage;
class UWidgetAnimation;

// Which signal the ring watches.
UENUM(BlueprintType)
enum class ETwinPartRingSignal : uint8
{
	// Coolant or engine rpm, on the hood.
	Engine,
	TyreFrontLeft,
	TyreFrontRight,
	TyreRearLeft,
	TyreRearRight,
};

UCLASS()
class UTwinPartRingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowPartStatus(EVehicleStatus PartStatus);

	FName GetAnchorTag() const { return AnchorTag; }
	ETwinPartRingSignal GetRingSignal() const { return RingSignal; }

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RingImage;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PulseAnimation;

	UPROPERTY(EditAnywhere, Category = "Part Ring")
	FName AnchorTag = TEXT("CalloutAnchor.Hood");

	UPROPERTY(EditAnywhere, Category = "Part Ring")
	ETwinPartRingSignal RingSignal = ETwinPartRingSignal::Engine;

private:
	EVehicleStatus ShownStatus = EVehicleStatus::Normal;
};