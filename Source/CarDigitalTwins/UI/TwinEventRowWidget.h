// One line of the Events list: trip time, a severity dot (round amber/red/grey, square cyan for commands) and the message.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MVVM/VehicleEventLogViewModel.h"
#include "TwinEventRowWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class UTwinEventRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowEvent(const FVehicleEventLogEntry& EventEntry);

	// Fewer events than rows: the row disappears.
	void ShowNoEvent();

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EventTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> EventSeverityImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EventMessageText;
};