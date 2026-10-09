#include "UI/TwinTopBarWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TwinHmiStyle.h"

namespace TopBarText
{
	static const FString Separator = TEXT(" · ");
}

void UTwinTopBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (TellTaleIconImage)
	{
		TellTaleIconImage->SetDesiredSizeOverride(FVector2D(38.f, 38.f));
	}
	if (LinkDotImage)
	{
		LinkDotImage->SetDesiredSizeOverride(FVector2D(10.f, 10.f));
	}
	// Designer preview: the Warning state from the mock.
	if (IsDesignTime())
	{
		ShowTellTale(EVehicleStatus::Warning, NSLOCTEXT("TwinHmi", "PreviewWarning", "Warning"),
			FText::FromString(TEXT("Coolant 108.9 °C")), true);
	}
	RefreshTopBar();
}

void UTwinTopBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinTopBarWidget::HandleViewModelFieldsChanged);
	SubscribeToViewModel(ConnectionViewModel, this, &UTwinTopBarWidget::HandleViewModelFieldsChanged);
}

void UTwinTopBarWidget::HandleViewModelFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	// Small widget, few fields: redraw all of it on any change from either ViewModel.
	RefreshTopBar();
}

void UTwinTopBarWidget::ShowTellTale(EVehicleStatus OverallStatus, const FText& StateWord, const FText& Reason, bool bShowIcon)
{
	if (TellTaleBorder)
	{
		const FLinearColor TellTaleFill = OverallStatus == EVehicleStatus::Critical ? TwinHmiColors::CriticalSoft()
			: OverallStatus == EVehicleStatus::Warning ? TwinHmiColors::WarningSoft() : TwinHmiColors::FromSrgbHex(0xFFFFFF, 0.03f);
		TellTaleBorder->SetBrush(TwinHmiBrushes::MakeRoundedBox(TellTaleFill,
			TwinHmiColors::ForStatus(OverallStatus, TwinHmiColors::PanelEdgeHighlight()), 1.f, 5.f));
	}
	if (TellTaleIconImage)
	{
		UTexture2D* IconTexture = OverallStatus == EVehicleStatus::Normal ? StatusOkIcon.Get() : StatusAlertIcon.Get();
		if (IconTexture)
		{
			TellTaleIconImage->SetBrushFromTexture(IconTexture);
			TellTaleIconImage->SetDesiredSizeOverride(FVector2D(38.f, 38.f));
		}
		TellTaleIconImage->SetColorAndOpacity(TwinHmiColors::ForStatus(OverallStatus, TwinHmiColors::TextSecondary()));
		TellTaleIconImage->SetVisibility(bShowIcon && IconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (TellTaleStateText)
	{
		TellTaleStateText->SetText(StateWord);
		TellTaleStateText->SetColorAndOpacity(FSlateColor(bShowIcon ? TwinHmiColors::ForStatus(OverallStatus, TwinHmiColors::TextPrimary())
			: TwinHmiColors::TextSecondary()));
	}
	if (TellTaleReasonText)
	{
		TellTaleReasonText->SetText(Reason);
		TellTaleReasonText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}
}

void UTwinTopBarWidget::RefreshTopBar()
{
	if (!TelemetryViewModel || !ConnectionViewModel)
	{
		return;
	}
	const UVehicleTelemetryViewModel& Telemetry = *TelemetryViewModel;
	const ETelemetryConnectionState ConnectionState = ConnectionViewModel->GetConnectionState();
	const bool bHasTelemetry = Telemetry.HasReceivedTelemetry();
	const bool bIsLinkLive = ConnectionState == ETelemetryConnectionState::Live;

	// Trip clock.
	if (TripClockText)
	{
		FString TripClock = VehicleModelText.ToString() + TopBarText::Separator;
		TripClock += bHasTelemetry ? FString::Printf(TEXT("trip %.1f s"), Telemetry.GetSampleTimeS()) : TEXT("no trip yet");
		if (bHasTelemetry && !bIsLinkLive)
		{
			TripClock += TopBarText::Separator + TEXT("last data shown");
		}
		TripClockText->SetText(FText::FromString(TripClock));
		TripClockText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}

	// Tell-tale: the worst status and every signal at that level.
	if (!bHasTelemetry)
	{
		ShowTellTale(EVehicleStatus::Normal, NSLOCTEXT("TwinHmi", "NoData", "No data"), NSLOCTEXT("TwinHmi", "WaitingFirstSample", "Waiting for the first sample"), false);
	}
	else
	{
		const EVehicleStatus OverallStatus = Telemetry.GetOverallStatus();
		TArray<FString> WorstSignals;
		if (OverallStatus != EVehicleStatus::Normal)
		{
			if (Telemetry.GetCoolantTemperatureStatus() == OverallStatus)
			{
				WorstSignals.Add(FString::Printf(TEXT("Coolant %.1f °C"), Telemetry.GetCoolantTempC()));
			}
			if (Telemetry.GetTyrePressureStatus() == OverallStatus)
			{
				const TPair<const TCHAR*, float> Tyres[] = { { TEXT("FL"), Telemetry.GetTyrePressureFrontLeftKpa() },
					{ TEXT("FR"), Telemetry.GetTyrePressureFrontRightKpa() }, { TEXT("RL"), Telemetry.GetTyrePressureRearLeftKpa() },
					{ TEXT("RR"), Telemetry.GetTyrePressureRearRightKpa() } };
				const TPair<const TCHAR*, float>* LowestTyre = &Tyres[0];
				for (const TPair<const TCHAR*, float>& Tyre : Tyres)
				{
					LowestTyre = Tyre.Value < LowestTyre->Value ? &Tyre : LowestTyre;
				}
				WorstSignals.Add(FString::Printf(TEXT("Tyre %s %.0f kPa"), LowestTyre->Key, LowestTyre->Value));
			}
			if (Telemetry.GetFuelLevelStatus() == OverallStatus)
			{
				WorstSignals.Add(TEXT("Fuel"));
			}
			if (Telemetry.GetBatteryVoltageStatus() == OverallStatus)
			{
				WorstSignals.Add(TEXT("Battery"));
			}
			if (Telemetry.GetEngineRpmStatus() == OverallStatus)
			{
				WorstSignals.Add(TEXT("Engine rpm"));
			}
		}
		const FText Reason = WorstSignals.IsEmpty() ? NSLOCTEXT("TwinHmi", "AllNormal", "All signals normal")
			: FText::FromString(FString::Join(WorstSignals, *TopBarText::Separator));
		ShowTellTale(OverallStatus, TwinHmiFormat::StatusText(OverallStatus), Reason, true);
	}

	// Drive mode chip: amber while the vehicle reports the derate.
	const bool bIsDerated = bHasTelemetry && Telemetry.GetDriveMode() == EVehicleDriveMode::EngineDerate;
	if (DriveModeChipBorder)
	{
		DriveModeChipBorder->SetBrush(bIsDerated
			? TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::WarningSoft(), TwinHmiColors::Warning(), 1.f, 4.f)
			: TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(), TwinHmiColors::PanelEdgeHighlight(), 1.f, 4.f));
	}
	if (DriveModeLabelText)
	{
		DriveModeLabelText->SetColorAndOpacity(FSlateColor(bIsDerated ? TwinHmiColors::Warning() : TwinHmiColors::TextSecondary()));
	}
	if (DriveModeValueText)
	{
		DriveModeValueText->SetText(!bHasTelemetry ? FText::FromString(TEXT("–"))
			: bIsDerated ? NSLOCTEXT("TwinHmi", "DriveModeDerate", "Engine derate") : NSLOCTEXT("TwinHmi", "DriveModeNormal", "Normal"));
		DriveModeValueText->SetColorAndOpacity(FSlateColor(bIsDerated ? TwinHmiColors::Warning() : TwinHmiColors::TextSecondary()));
	}

	// Link chip.
	FText LinkState;
	FString LinkDetail;
	FLinearColor LinkColor = TwinHmiColors::TextPrimary();
	FLinearColor ChipEdge = TwinHmiColors::PanelEdgeHighlight();
	FSlateBrush DotBrush = TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::TextTertiary(), TwinHmiColors::Transparent(), 0.f, 5.f);
	switch (ConnectionState)
	{
	case ETelemetryConnectionState::Live:
		LinkState = NSLOCTEXT("TwinHmi", "LinkLive", "Live");
		LinkDetail = FString::Printf(TEXT("%.0f ms%s%lld lost"), ConnectionViewModel->GetAverageReceiveLatencyMs(), *TopBarText::Separator,
			ConnectionViewModel->GetDroppedMessageCount());
		DotBrush = TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::LinkLive(), TwinHmiColors::Transparent(), 0.f, 5.f);
		break;
	case ETelemetryConnectionState::Stale:
		LinkState = NSLOCTEXT("TwinHmi", "LinkStale", "Stale");
		LinkDetail = FString::Printf(TEXT("no data %.1f s"), ConnectionViewModel->GetSecondsSinceLastSample());
		LinkColor = ChipEdge = TwinHmiColors::Warning();
		DotBrush = TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Warning(), TwinHmiColors::Transparent(), 0.f, 5.f);
		break;
	case ETelemetryConnectionState::Disconnected:
		LinkState = NSLOCTEXT("TwinHmi", "LinkDown", "Disconnected");
		LinkDetail = FString::Printf(TEXT("retry in %.1f s"), ConnectionViewModel->GetSecondsUntilReconnectAttempt());
		LinkColor = ChipEdge = TwinHmiColors::Warning();
		// Hollow dot: the link is gone, not just slow.
		DotBrush = TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(), TwinHmiColors::Warning(), 2.f, 5.f);
		break;
	default:
		LinkState = NSLOCTEXT("TwinHmi", "LinkConnecting", "Connecting");
		LinkDetail = ConnectionViewModel->GetReceiverDisplayName();
		break;
	}
	if (LinkChipBorder)
	{
		LinkChipBorder->SetBrush(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Transparent(), ChipEdge, 1.f, 4.f));
	}
	if (LinkDotImage)
	{
		DotBrush.ImageSize = FVector2D(10.f, 10.f);
		LinkDotImage->SetBrush(DotBrush);
	}
	if (LinkStateText)
	{
		LinkStateText->SetText(LinkState);
		LinkStateText->SetColorAndOpacity(FSlateColor(LinkColor));
	}
	if (LinkDetailText)
	{
		LinkDetailText->SetText(FText::FromString(LinkDetail));
		LinkDetailText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextSecondary()));
	}
}