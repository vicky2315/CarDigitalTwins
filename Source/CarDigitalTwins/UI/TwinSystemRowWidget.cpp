#include "UI/TwinSystemRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"
#include "UI/TwinStatusTextWidget.h"

void UTwinSystemRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (SystemNameText)
	{
		SystemNameText->SetText(SystemName);
		SystemNameText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextPrimary()));
	}
	if (SystemDetailText)
	{
		SystemDetailText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}
	if (RowButton)
	{
		FButtonStyle RowButtonStyle;
		RowButtonStyle.SetNormal(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(), TwinHmiColors::Transparent(), 0.f, 4.f));
		RowButtonStyle.SetHovered(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::HoverTint(), TwinHmiColors::Transparent(), 0.f, 4.f));
		RowButtonStyle.SetPressed(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::HoverTint(), TwinHmiColors::Transparent(), 0.f, 4.f));
		RowButtonStyle.SetNormalPadding(FMargin(10.f, 0.f));
		RowButtonStyle.SetPressedPadding(FMargin(10.f, 0.f));
		RowButton->SetStyle(RowButtonStyle);
	}
}

void UTwinSystemRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (RowButton)
	{
		RowButton->OnClicked.AddDynamic(this, &UTwinSystemRowWidget::HandleRowButtonClicked);
	}
}

void UTwinSystemRowWidget::ShowSystemState(const FText& SystemDetail, EVehicleStatus SystemStatusValue)
{
	if (SystemDetailText)
	{
		SystemDetailText->SetText(SystemDetail);
	}
	if (SystemStatus)
	{
		SystemStatus->SetStatus(SystemStatusValue);
	}
}

void UTwinSystemRowWidget::ShowSystemStateWithLabel(const FText& SystemDetail, const FText& StatusLabel, EVehicleStatus ColorStatus)
{
	if (SystemDetailText)
	{
		SystemDetailText->SetText(SystemDetail);
	}
	if (SystemStatus)
	{
		SystemStatus->SetStatusWithLabel(StatusLabel, ColorStatus);
	}
}

void UTwinSystemRowWidget::HandleRowButtonClicked()
{
	if (UDashboardViewModel* DashboardViewModel = FindViewModel<UDashboardViewModel>())
	{
		DashboardViewModel->SelectView(TargetView);
	}
}