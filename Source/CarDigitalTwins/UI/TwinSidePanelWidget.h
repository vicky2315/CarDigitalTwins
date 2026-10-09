// The detail panel on the right: title and subtitle of the open view and a Widget Switcher holding the four view panels in tab order
// (Overview, Powertrain, Tyres, Body). Switches with the view and plays PanelInAnimation (fade and slide in, 0.32 s).

#pragma once

#include "CoreMinimal.h"
#include "MVVM/DashboardViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinSidePanelWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;
class UWidgetSwitcher;

UCLASS()
class UTwinSidePanelWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PanelTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PanelSubtitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ViewPanelSwitcher;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PanelInAnimation;

private:
	void HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask);

	UPROPERTY(Transient)
	TObjectPtr<UDashboardViewModel> DashboardViewModel;

	bool bHasShownFirstView = false;
};