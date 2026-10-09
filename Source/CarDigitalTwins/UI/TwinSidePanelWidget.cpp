#include "UI/TwinSidePanelWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

namespace SidePanelText
{
	FText Subtitle(ETwinDashboardView View)
	{
		switch (View)
		{
		case ETwinDashboardView::Powertrain: return NSLOCTEXT("TwinHmi", "SubtitlePowertrain", "Engine and electrics");
		case ETwinDashboardView::Tyres: return NSLOCTEXT("TwinHmi", "SubtitleTyres", "Pressures, nominal 240 kPa");
		case ETwinDashboardView::Body: return NSLOCTEXT("TwinHmi", "SubtitleBody", "Openings");
		default: return NSLOCTEXT("TwinHmi", "SubtitleOverview", "All systems");
		}
	}
}

void UTwinSidePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(DashboardViewModel, this, &UTwinSidePanelWidget::HandleDashboardFieldsChanged);
}

void UTwinSidePanelWidget::HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!DashboardViewModel || !ChangedFieldMask.HasField(EDashboardViewModelField::ActiveView))
	{
		return;
	}
	const ETwinDashboardView ActiveView = DashboardViewModel->GetActiveView();
	if (PanelTitleText)
	{
		PanelTitleText->SetText(FText::FromString(GetTwinDashboardViewName(ActiveView)));
	}
	if (PanelSubtitleText)
	{
		PanelSubtitleText->SetText(SidePanelText::Subtitle(ActiveView));
	}
	if (ViewPanelSwitcher)
	{
		ViewPanelSwitcher->SetActiveWidgetIndex(static_cast<int32>(ActiveView));
	}
	// No slide-in for the first view at Play, only when the operator switches.
	if (PanelInAnimation && bHasShownFirstView)
	{
		PlayAnimation(PanelInAnimation);
	}
	bHasShownFirstView = true;
}