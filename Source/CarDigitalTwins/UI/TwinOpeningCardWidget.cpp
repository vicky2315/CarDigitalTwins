#include "UI/TwinOpeningCardWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"

void UTwinOpeningCardWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (OpeningNameText)
	{
		OpeningNameText->SetText(OpeningLabel);
		OpeningNameText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextPrimary()));
	}
	ShowOpenState(false);
}

void UTwinOpeningCardWidget::ShowOpenState(bool bIsOpen)
{
	if (CardBorder)
	{
		CardBorder->SetBrush(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(),
			bIsOpen ? TwinHmiColors::Warning() : TwinHmiColors::PanelEdge(), 1.f, 5.f));
	}
	if (OpeningStateText)
	{
		OpeningStateText->SetText(bIsOpen ? NSLOCTEXT("TwinHmi", "OpeningOpen", "Open") : NSLOCTEXT("TwinHmi", "OpeningClosed", "Closed"));
		OpeningStateText->SetColorAndOpacity(FSlateColor(bIsOpen ? TwinHmiColors::Warning() : TwinHmiColors::TextTertiary()));
	}
}