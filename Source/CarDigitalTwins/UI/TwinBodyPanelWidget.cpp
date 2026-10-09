#include "UI/TwinBodyPanelWidget.h"

#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"
#include "UI/TwinOpeningCardWidget.h"

void UTwinBodyPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinBodyPanelWidget::HandleTelemetryFieldsChanged);
}

void UTwinBodyPanelWidget::HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!TelemetryViewModel)
	{
		return;
	}
	using TelemetryField = EVehicleTelemetryViewModelField;
	if (ChangedFieldMask.HasField(TelemetryField::Openings))
	{
		for (UTwinOpeningCardWidget* OpeningCard : { OpeningCardDoorFrontLeft.Get(), OpeningCardDoorFrontRight.Get(), OpeningCardDoorRearLeft.Get(),
			OpeningCardDoorRearRight.Get(), OpeningCardHood.Get(), OpeningCardTailgate.Get() })
		{
			if (OpeningCard)
			{
				OpeningCard->ShowOpenState(TelemetryViewModel->IsOpen(OpeningCard->GetOpening()));
			}
		}
	}
	if (ChangedFieldMask.HasField(TelemetryField::OdometerKm) && OdometerValueText)
	{
		OdometerValueText->SetText(TwinHmiFormat::FormatFixed(TelemetryViewModel->GetOdometerKm(), 1));
	}
}