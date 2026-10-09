#include "UI/TwinViewTabWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TwinHmiStyle.h"

void UTwinViewTabWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (TabLabelText)
	{
		TabLabelText->SetText(TabLabel);
		TabLabelText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextPrimary()));
	}
	if (TabIconImage)
	{
		if (TabIcon)
		{
			TabIconImage->SetBrushFromTexture(TabIcon);
		}
		TabIconImage->SetDesiredSizeOverride(FVector2D(30.f, 30.f));
		TabIconImage->SetColorAndOpacity(TwinHmiColors::TextSecondary());
	}
	if (SelectedBarImage)
	{
		FSlateBrush SelectedBarBrush = TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Accent(), TwinHmiColors::Transparent(), 0.f, 2.f);
		SelectedBarBrush.ImageSize = FVector2D(4.f, 34.f);
		SelectedBarImage->SetBrush(SelectedBarBrush);
	}
	if (StatusPipImage)
	{
		FSlateBrush PipBrush = TwinHmiBrushes::MakeRoundedBox(FLinearColor::White, TwinHmiColors::Transparent(), 0.f, 3.f);
		PipBrush.ImageSize = FVector2D(14.f, 14.f);
		StatusPipImage->SetBrush(PipBrush);
		StatusPipImage->SetRenderTransformAngle(45.f);
	}
	// In the designer, show the Overview tab selected so its look can be checked.
	bIsSelected = IsDesignTime() && TabView == ETwinDashboardView::Overview;
	ApplyTabStyle();
	SetTabStatus(EVehicleStatus::Normal);
}

void UTwinViewTabWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (TabButton)
	{
		TabButton->OnClicked.AddDynamic(this, &UTwinViewTabWidget::HandleTabButtonClicked);
	}
}

void UTwinViewTabWidget::SetTabSelected(bool bSelected)
{
	if (bSelected != bIsSelected)
	{
		bIsSelected = bSelected;
		ApplyTabStyle();
	}
}

void UTwinViewTabWidget::SetTabStatus(EVehicleStatus ViewStatus)
{
	if (StatusPipImage)
	{
		StatusPipImage->SetColorAndOpacity(TwinHmiColors::ForStatus(ViewStatus, TwinHmiColors::Transparent()));
		StatusPipImage->SetVisibility(ViewStatus == EVehicleStatus::Normal ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}
}

void UTwinViewTabWidget::ApplyTabStyle()
{
	if (!TabButton)
	{
		return;
	}
	// Unselected: no fill until hovered. Selected: soft cyan fill with a cyan edge, plus the bar on the left.
	const FSlateBrush IdleBrush = bIsSelected
		? TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::AccentSoft(), TwinHmiColors::Accent(), 1.f, 4.f)
		: TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(), TwinHmiColors::Transparent(), 0.f, 4.f);
	const FSlateBrush HoveredBrush = bIsSelected
		? IdleBrush
		: TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::HoverTint(), TwinHmiColors::Transparent(), 0.f, 4.f);
	FButtonStyle TabButtonStyle;
	TabButtonStyle.SetNormal(IdleBrush);
	TabButtonStyle.SetHovered(HoveredBrush);
	TabButtonStyle.SetPressed(HoveredBrush);
	TabButtonStyle.SetNormalPadding(FMargin(0.f));
	TabButtonStyle.SetPressedPadding(FMargin(0.f));
	TabButton->SetStyle(TabButtonStyle);

	if (SelectedBarImage)
	{
		SelectedBarImage->SetVisibility(bIsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UTwinViewTabWidget::HandleTabButtonClicked()
{
	if (UDashboardViewModel* DashboardViewModel = FindViewModel<UDashboardViewModel>())
	{
		DashboardViewModel->SelectView(TabView);
	}
}