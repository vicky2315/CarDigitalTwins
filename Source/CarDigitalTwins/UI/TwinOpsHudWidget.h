// The dashboard's root (WBP_TwinOpsHUD): a full-screen canvas holding the other parts. Shows the "Waiting for the vehicle" card
// until the first sample, hiding the side panel and the callouts until then, and dims them while the link is Stale or Disconnected,
// because they show the last data received rather than live values.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ConnectionViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinOpsHudWidget.generated.h"

UCLASS()
class UTwinOpsHudWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> WaitingPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> SidePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> CalloutLayer;

private:
	void HandleViewModelFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UConnectionViewModel> ConnectionViewModel;
};