// UI state shared by the dashboard: which view is open. The view tabs send SelectView; the side panel, the callouts on the car
// and the camera (ATwinOpsPlayerController) all follow ActiveView, so they can never disagree (docs/SPEC.md §8).

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ViewModelBase.h"
#include "DashboardViewModel.generated.h"

// Each view = a camera shot of the car + a detail panel. The order is the tab order and the side panel's widget switcher order.
UENUM(BlueprintType)
enum class ETwinDashboardView : uint8
{
	Overview,
	Powertrain,
	Tyres,
	Body,
	Count UMETA(Hidden),
};

// "Overview", "Powertrain", ...: the part after the camera tag prefix (TwinView.Overview) and the panel title.
const TCHAR* GetTwinDashboardViewName(ETwinDashboardView View);

UENUM(BlueprintType)
enum class EDashboardViewModelField : uint8
{
	ActiveView,
	Count UMETA(Hidden),
};

UCLASS()
class UDashboardViewModel : public UViewModelBase
{
	GENERATED_BODY()

public:
	//~ UViewModelBase
	virtual int32 GetFieldCount() const override { return static_cast<int32>(EDashboardViewModelField::Count); }

	UFUNCTION(BlueprintPure, Category = "Dashboard")
	ETwinDashboardView GetActiveView() const { return ActiveView; }

	// Command from a view tab, a system row, the alarm's Show button or the 1-4 keys.
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void SelectView(ETwinDashboardView NewActiveView);

private:
	ETwinDashboardView ActiveView = ETwinDashboardView::Overview;
};