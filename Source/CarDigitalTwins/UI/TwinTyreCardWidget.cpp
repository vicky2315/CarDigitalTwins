#include "UI/TwinTyreCardWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"
#include "UI/TwinRangeBarWidget.h"
#include "UI/TwinStatusTextWidget.h"

void UTwinTyreCardWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (WheelLabelText)
	{
		WheelLabelText->SetText(WheelLabel);
	}
	ShowPressure(240.f);
}

void UTwinTyreCardWidget::ShowPressure(float PressureKpa)
{
	const EVehicleStatus TyreStatus = TwinHmiFormat::SingleTyreStatus(PressureKpa);
	if (CardBorder)
	{
		CardBorder->SetBrush(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(), TwinHmiColors::ForStatus(TyreStatus, TwinHmiColors::PanelEdge()),
			1.f, 5.f));
	}
	if (PressureValueText)
	{
		PressureValueText->SetText(TwinHmiFormat::FormatFixed(PressureKpa, 0));
		PressureValueText->SetColorAndOpacity(FSlateColor(TwinHmiColors::ForStatus(TyreStatus, TwinHmiColors::TextPrimary())));
	}
	if (WheelStatus)
	{
		WheelStatus->SetStatus(TyreStatus);
	}
	if (PressureBar)
	{
		PressureBar->SetBarValue(PressureKpa, TyreStatus);
	}
}