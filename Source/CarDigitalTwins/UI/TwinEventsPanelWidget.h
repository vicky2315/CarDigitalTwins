// The Events list, bottom left: the five newest events from the event log, newest on top.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/VehicleEventLogViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinEventsPanelWidget.generated.h"

class UTextBlock;
class UTwinEventRowWidget;

UCLASS()
class UTwinEventsPanelWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinEventRowWidget> EventRow0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinEventRowWidget> EventRow1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinEventRowWidget> EventRow2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinEventRowWidget> EventRow3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinEventRowWidget> EventRow4;

	// "Nothing yet this trip", shown while the log is empty.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EmptyEventsText;

private:
	void HandleEventLogFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UVehicleEventLogViewModel> EventLogViewModel;
};