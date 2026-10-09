#include "UI/TwinStatusTextWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"

void UTwinStatusTextWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (StatusDiamondImage)
	{
		// An 11 × 11 rounded square turned 45°: the mock's diamond.
		FSlateBrush DiamondBrush = TwinHmiBrushes::MakeRoundedBox(FLinearColor::White, TwinHmiColors::Transparent(), 0.f, 2.f);
		DiamondBrush.ImageSize = FVector2D(11.f, 11.f);
		StatusDiamondImage->SetBrush(DiamondBrush);
		StatusDiamondImage->SetRenderTransformAngle(45.f);
	}
	SetStatus(EVehicleStatus::Normal);
}

void UTwinStatusTextWidget::SetStatus(EVehicleStatus NewStatus)
{
	SetStatusWithLabel(NewStatus == EVehicleStatus::Normal ? NormalStatusLabel : TwinHmiFormat::StatusText(NewStatus), NewStatus);
}

void UTwinStatusTextWidget::SetStatusWithLabel(const FText& NewLabel, EVehicleStatus ColorStatus)
{
	if (!StatusDiamondImage || !StatusLabelText)
	{
		return;
	}
	const FLinearColor StatusColor = TwinHmiColors::ForStatus(ColorStatus, TwinHmiColors::TextTertiary());
	StatusLabelText->SetText(NewLabel);
	StatusLabelText->SetColorAndOpacity(FSlateColor(StatusColor));
	StatusDiamondImage->SetColorAndOpacity(StatusColor);
	StatusDiamondImage->SetVisibility(ColorStatus == EVehicleStatus::Normal ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}