// One view tab in the rail: icon, name and a status pip when something in that view is off. Clicking it selects the view
// (UDashboardViewModel::SelectView); UTwinViewRailWidget tells it whether it is selected and what pip to show. C++ draws the
// selected fill, edge and bar, the pip, and the hover state.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/DashboardViewModel.h"
#include "Telemetry/VehicleTelemetry.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinViewTabWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class UTwinViewTabWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

public:
	void SetTabSelected(bool bSelected);
	void SetTabStatus(EVehicleStatus ViewStatus);
	ETwinDashboardView GetTabView() const { return TabView; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SelectedBarImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TabIconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TabLabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> StatusPipImage;

	UPROPERTY(EditAnywhere, Category = "View Tab")
	ETwinDashboardView TabView = ETwinDashboardView::Overview;

	UPROPERTY(EditAnywhere, Category = "View Tab")
	FText TabLabel = NSLOCTEXT("TwinHmi", "DefaultTabLabel", "Overview");

	UPROPERTY(EditAnywhere, Category = "View Tab")
	TObjectPtr<UTexture2D> TabIcon;

private:
	UFUNCTION()
	void HandleTabButtonClicked();

	void ApplyTabStyle();

	bool bIsSelected = false;
};