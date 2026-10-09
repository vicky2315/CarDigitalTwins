#include "UI/TwinCarCalloutWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"

void UTwinCarCalloutWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (CalloutKeyText)
	{
		CalloutKeyText->SetText(CalloutKey);
		CalloutKeyText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}
	if (CalloutUnitText)
	{
		CalloutUnitText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}
	if (CalloutStemImage)
	{
		FSlateBrush StemBrush = TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::PanelEdgeHighlight(), TwinHmiColors::Transparent(), 0.f, 0.f);
		StemBrush.ImageSize = FVector2D(2.f, 46.f);
		CalloutStemImage->SetBrush(StemBrush);
	}
	ShowCalloutValue(NSLOCTEXT("TwinHmi", "CalloutPreviewValue", "108.9"), FText::FromString(TEXT("°C")), TwinHmiColors::TextPrimary(),
		TwinHmiColors::PanelEdgeHighlight());
}

void UTwinCarCalloutWidget::ShowCalloutValue(const FText& ValueText, const FText& UnitText, const FLinearColor& ValueColor, const FLinearColor& EdgeColor)
{
	if (CalloutValueText)
	{
		CalloutValueText->SetText(ValueText);
		CalloutValueText->SetColorAndOpacity(FSlateColor(ValueColor));
	}
	if (CalloutUnitText)
	{
		CalloutUnitText->SetText(UnitText);
	}
	if (CalloutBoxBorder)
	{
		CalloutBoxBorder->SetBrush(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::PanelFill(), EdgeColor, 1.f, 4.f));
	}
}