// One tyre in the Tyres view: wheel name, pressure, its own status and a range bar. The Tyres panel feeds it; C++ colours the card's
// edge and the value by the tyre's status.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TwinTyreCardWidget.generated.h"

class UBorder;
class UTextBlock;
class UTwinRangeBarWidget;
class UTwinStatusTextWidget;

UCLASS()
class UTwinTyreCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowPressure(float PressureKpa);

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> CardBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WheelLabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinStatusTextWidget> WheelStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PressureValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinRangeBarWidget> PressureBar;

	// "FL", "FR", "RL", "RR".
	UPROPERTY(EditAnywhere, Category = "Tyre Card")
	FText WheelLabel = NSLOCTEXT("TwinHmi", "DefaultWheelLabel", "FL");
};