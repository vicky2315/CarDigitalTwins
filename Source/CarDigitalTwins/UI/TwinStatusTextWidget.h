// A status word with its shape: "◆ WARNING" in amber, "◆ CRITICAL" in red, a grey "NORMAL" without the diamond. Never colour alone
// (HMI rule 2). C++ sizes, rotates and colours the diamond and the text; WBP_StatusText holds the two named widgets.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Telemetry/VehicleTelemetry.h"
#include "TwinStatusTextWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class UTwinStatusTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// "Normal" (NormalStatusLabel), "Warning" or "Critical".
	UFUNCTION(BlueprintCallable, Category = "Status Text")
	void SetStatus(EVehicleStatus NewStatus);

	// Own word with a status colour, e.g. "Open" in amber for a door.
	UFUNCTION(BlueprintCallable, Category = "Status Text")
	void SetStatusWithLabel(const FText& NewLabel, EVehicleStatus ColorStatus);

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> StatusDiamondImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusLabelText;

	// What Normal reads as on this copy, e.g. "Closed" on the Body row.
	UPROPERTY(EditAnywhere, Category = "Status Text")
	FText NormalStatusLabel = NSLOCTEXT("TwinHmi", "NormalStatusLabel", "Normal");
};