#include "UI/TwinHmiButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"

void UTwinHmiButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!ActionButton || !ActionLabelText)
	{
		return;
	}

	// Rounded 4, 1 px edge; the edge is cyan on a primary button. The label stays put when pressed: the darker fill is the feedback.
	const FLinearColor EdgeColor = bIsPrimary ? TwinHmiColors::Accent() : TwinHmiColors::PanelEdgeHighlight();
	FButtonStyle HmiButtonStyle;
	HmiButtonStyle.SetNormal(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::ButtonFill(), EdgeColor, 1.f, 4.f));
	HmiButtonStyle.SetHovered(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::ButtonFillHovered(), EdgeColor, 1.f, 4.f));
	HmiButtonStyle.SetPressed(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::ButtonFillPressed(), EdgeColor, 1.f, 4.f));
	HmiButtonStyle.SetDisabled(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::ButtonFill(), TwinHmiColors::PanelEdge(), 1.f, 4.f));
	HmiButtonStyle.SetNormalPadding(FMargin(18.f, 0.f));
	HmiButtonStyle.SetPressedPadding(FMargin(18.f, 0.f));
	ActionButton->SetStyle(HmiButtonStyle);

	ActionLabelText->SetColorAndOpacity(FSlateColor(bIsPrimary ? TwinHmiColors::Accent() : TwinHmiColors::TextPrimary()));
	ActionLabelText->SetText(ButtonLabel);
}

void UTwinHmiButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ActionButton)
	{
		ActionButton->OnClicked.AddDynamic(this, &UTwinHmiButtonWidget::HandleActionButtonClicked);
	}
}

void UTwinHmiButtonWidget::SetButtonLabel(const FText& NewButtonLabel)
{
	ButtonLabel = NewButtonLabel;
	if (ActionLabelText)
	{
		ActionLabelText->SetText(ButtonLabel);
	}
}

void UTwinHmiButtonWidget::SetButtonEnabled(bool bEnabled)
{
	if (ActionButton)
	{
		ActionButton->SetIsEnabled(bEnabled);
	}
	// The mock fades a disabled button to 40 %.
	SetRenderOpacity(bEnabled ? 1.f : 0.4f);
}

void UTwinHmiButtonWidget::HandleActionButtonClicked()
{
	OnButtonClicked.Broadcast();
}