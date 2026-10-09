// The "Views" rail on the left: four tabs. Marks the open view as selected and puts a status pip on a tab when something in its view
// is off (Powertrain: coolant, rpm, fuel, battery; Tyres: tyre pressure).

#pragma once

#include "CoreMinimal.h"
#include "MVVM/DashboardViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinViewRailWidget.generated.h"

class UTwinViewTabWidget;

UCLASS()
class UTwinViewRailWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinViewTabWidget> ViewTabOverview;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinViewTabWidget> ViewTabPowertrain;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinViewTabWidget> ViewTabTyres;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinViewTabWidget> ViewTabBody;

private:
	void HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UDashboardViewModel> DashboardViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;
};