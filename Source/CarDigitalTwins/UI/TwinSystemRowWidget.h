// One row of the Overview's Systems list: name, a short detail ("Coolant 108.9 °C · normal mode") and its status. Clicking it
// opens that system's view. The Overview panel fills it; C++ draws the hover state.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/DashboardViewModel.h"
#include "Telemetry/VehicleTelemetry.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinSystemRowWidget.generated.h"

class UButton;
class UTextBlock;
class UTwinStatusTextWidget;

UCLASS()
class UTwinSystemRowWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

public:
	void ShowSystemState(const FText& SystemDetail, EVehicleStatus SystemStatus);
	void ShowSystemStateWithLabel(const FText& SystemDetail, const FText& StatusLabel, EVehicleStatus ColorStatus);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RowButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SystemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SystemDetailText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinStatusTextWidget> SystemStatus;

	// The view the row opens.
	UPROPERTY(EditAnywhere, Category = "System Row")
	ETwinDashboardView TargetView = ETwinDashboardView::Powertrain;

	UPROPERTY(EditAnywhere, Category = "System Row")
	FText SystemName = NSLOCTEXT("TwinHmi", "DefaultSystemName", "Powertrain");

private:
	UFUNCTION()
	void HandleRowButtonClicked();
};