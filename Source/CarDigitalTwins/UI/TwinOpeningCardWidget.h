// One door, the hood or the tailgate in the Body view: its name and OPEN (amber edge and word) or CLOSED (grey). The Body panel
// feeds it from the Openings bitmask using this card's Opening.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Telemetry/VehicleTelemetry.h"
#include "TwinOpeningCardWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class UTwinOpeningCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowOpenState(bool bIsOpen);
	EVehicleOpening GetOpening() const { return Opening; }

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> CardBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OpeningNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OpeningStateText;

	// Which bit of the Openings bitmask this card shows.
	UPROPERTY(EditAnywhere, Category = "Opening Card")
	EVehicleOpening Opening = EVehicleOpening::DoorFL;

	// "Door FL", "Hood", ...
	UPROPERTY(EditAnywhere, Category = "Opening Card")
	FText OpeningLabel = NSLOCTEXT("TwinHmi", "DefaultOpeningLabel", "Door FL");
};