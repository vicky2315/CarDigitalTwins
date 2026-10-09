// A value against its range: a track with the Warning and Critical zones drawn faintly, and a marker coloured by status (ISA-101
// style). C++ draws every part inside BarCanvas from the properties below, so WBP_RangeBar only holds the canvas. Each placed copy
// sets its own range, zones and ticks in the Details panel.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "Telemetry/VehicleTelemetry.h"
#include "TwinRangeBarWidget.generated.h"

class UCanvasPanel;
class UImage;
class USizeBox;

UCLASS()
class UTwinRangeBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Range Bar")
	void SetBarValue(float NewValue, EVehicleStatus NewStatus);

	// A dashed-style amber line at LimitValue, e.g. the 2500 rpm cap while the engine is derated.
	UFUNCTION(BlueprintCallable, Category = "Range Bar")
	void SetLimitLine(bool bShowLimitLine, float NewLimitValue);

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> BarCanvas;

	// The root Size Box; C++ sets its width to BarWidth and its height to 44.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> BarSizeBox;

	// Px at 1080p, like every size on the build page.
	UPROPERTY(EditAnywhere, Category = "Range Bar", meta = (ClampMin = "40"))
	float BarWidth = 496.f;

	UPROPERTY(EditAnywhere, Category = "Range Bar")
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, Category = "Range Bar")
	float MaxValue = 100.f;

	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (InlineEditConditionToggle))
	bool bHasCriticalBelow = false;

	// Critical zone from MinValue up to here.
	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (EditCondition = "bHasCriticalBelow"))
	float CriticalBelow = 0.f;

	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (InlineEditConditionToggle))
	bool bHasWarningBelow = false;

	// Warning zone from the critical zone (or MinValue) up to here.
	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (EditCondition = "bHasWarningBelow"))
	float WarningBelow = 0.f;

	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (InlineEditConditionToggle))
	bool bHasWarningAbove = false;

	// Warning zone from here up to the critical zone (or MaxValue).
	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (EditCondition = "bHasWarningAbove"))
	float WarningAbove = 0.f;

	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (InlineEditConditionToggle))
	bool bHasCriticalAbove = false;

	// Critical zone from here up to MaxValue.
	UPROPERTY(EditAnywhere, Category = "Range Bar|Zones", meta = (EditCondition = "bHasCriticalAbove"))
	float CriticalAbove = 0.f;

	// Scale labels under the bar, e.g. 60, 90, 105, 115, 130.
	UPROPERTY(EditAnywhere, Category = "Range Bar")
	TArray<float> TickValues;

	// Set once in WBP_RangeBar's Class Defaults: Barlow Regular, size 10.
	UPROPERTY(EditAnywhere, Category = "Range Bar")
	FSlateFontInfo TickFont;

	// Where the marker sits in the designer, so the bar can be checked without playing.
	UPROPERTY(EditAnywhere, Category = "Range Bar")
	float PreviewValue = 50.f;

private:
	// Recreates the track, zones, marker and ticks inside BarCanvas. Runs in the designer on every property change and once at Play.
	void BuildBarParts();
	void PlaceMarkerAndLimitLine();
	float ValueToBarX(float Value) const;

	UPROPERTY(Transient)
	TObjectPtr<UImage> MarkerImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> MarkerOutlineImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> LimitLineImage;

	float CurrentValue = 0.f;
	EVehicleStatus CurrentStatus = EVehicleStatus::Normal;
	bool bIsLimitLineShown = false;
	float LimitValue = 0.f;
	bool bHasRuntimeValue = false;
};