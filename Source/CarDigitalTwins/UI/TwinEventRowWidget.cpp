#include "UI/TwinEventRowWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"

void UTwinEventRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime())
	{
		FVehicleEventLogEntry DesignerEvent;
		DesignerEvent.TripSeconds = 119.1;
		DesignerEvent.Severity = EVehicleEventSeverity::Critical;
		DesignerEvent.EventMessage = TEXT("Coolant Critical · 115.0 °C");
		ShowEvent(DesignerEvent);
	}
}

void UTwinEventRowWidget::ShowEvent(const FVehicleEventLogEntry& EventEntry)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (EventTimeText)
	{
		EventTimeText->SetText(FText::Format(NSLOCTEXT("TwinHmi", "EventTime", "{0} s"), TwinHmiFormat::FormatFixed(EventEntry.TripSeconds, 1)));
		EventTimeText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}
	if (EventMessageText)
	{
		EventMessageText->SetText(FText::FromString(EventEntry.EventMessage));
		EventMessageText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextPrimary()));
	}
	if (EventSeverityImage)
	{
		// Round dot for vehicle events, a small square for commands, as in the mock.
		const bool bIsCommand = EventEntry.Severity == EVehicleEventSeverity::Command;
		FLinearColor SeverityColor = TwinHmiColors::TextTertiary();
		if (EventEntry.Severity == EVehicleEventSeverity::Warning)
		{
			SeverityColor = TwinHmiColors::Warning();
		}
		else if (EventEntry.Severity == EVehicleEventSeverity::Critical)
		{
			SeverityColor = TwinHmiColors::Critical();
		}
		else if (bIsCommand)
		{
			SeverityColor = TwinHmiColors::Accent();
		}
		FSlateBrush SeverityBrush = TwinHmiBrushes::MakeRoundedBox(SeverityColor, TwinHmiColors::Transparent(), 0.f, bIsCommand ? 2.f : 5.f);
		SeverityBrush.ImageSize = FVector2D(10.f, 10.f);
		EventSeverityImage->SetBrush(SeverityBrush);
	}
}

void UTwinEventRowWidget::ShowNoEvent()
{
	SetVisibility(ESlateVisibility::Collapsed);
}