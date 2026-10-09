#include "UI/TwinAlarmBannerWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "MVVM/DashboardViewModel.h"
#include "UI/TwinHmiButtonWidget.h"
#include "UI/TwinHmiStyle.h"

namespace AlarmBannerTiming
{
	// The mock flashes the frame at 1 Hz: half a second bright red, half a second dim.
	static constexpr float FlashPeriodSeconds = 1.f;
}

void UTwinAlarmBannerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetFrameEdge(TwinHmiColors::Critical());
	if (AlarmTitleText)
	{
		AlarmTitleText->SetColorAndOpacity(FSlateColor(TwinHmiColors::Critical()));
	}
	if (AlarmDetailText)
	{
		AlarmDetailText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextPrimary()));
	}
	// Hidden at Play until an alarm comes; visible in the designer.
	SetVisibility(IsDesignTime() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UTwinAlarmBannerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Once per widget: NativeConstruct can run again when the HUD is re-added, which would bind the buttons twice.
	if (ShowButton)
	{
		ShowButton->OnButtonClicked.AddUObject(this, &UTwinAlarmBannerWidget::HandleShowClicked);
	}
	if (AcknowledgeButton)
	{
		AcknowledgeButton->OnButtonClicked.AddUObject(this, &UTwinAlarmBannerWidget::HandleAcknowledgeClicked);
	}
}

void UTwinAlarmBannerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinAlarmBannerWidget::HandleViewModelFieldsChanged);
	SubscribeToViewModel(EventLogViewModel, this, &UTwinAlarmBannerWidget::HandleViewModelFieldsChanged);
}

void UTwinAlarmBannerWidget::HandleViewModelFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	RefreshAlarm();
}

void UTwinAlarmBannerWidget::RefreshAlarm()
{
	if (!EventLogViewModel)
	{
		return;
	}
	const FVehicleAlarm& ActiveAlarm = EventLogViewModel->GetActiveAlarm();
	SetVisibility(ActiveAlarm.bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!ActiveAlarm.bIsActive)
	{
		bIsFlashing = false;
		return;
	}

	const bool bVehicleReportsDerate = TelemetryViewModel && TelemetryViewModel->GetDriveMode() == EVehicleDriveMode::EngineDerate;
	if (AlarmTitleText)
	{
		AlarmTitleText->SetText(FText::FromString(ActiveAlarm.AlarmTitle));
	}
	if (AlarmDetailText)
	{
		AlarmDetailText->SetText(FText::FromString(ActiveAlarm.AlarmDetail
			+ (bVehicleReportsDerate ? TEXT(" · vehicle reports Engine derate") : TEXT(""))));
	}
	if (AcknowledgeButton)
	{
		AcknowledgeButton->SetButtonLabel(ActiveAlarm.bIsAcknowledged ? NSLOCTEXT("TwinHmi", "Acknowledged", "Acknowledged")
			: NSLOCTEXT("TwinHmi", "Acknowledge", "Acknowledge"));
		AcknowledgeButton->SetButtonEnabled(!ActiveAlarm.bIsAcknowledged);
	}

	// Flashing until acknowledged, then a steady red frame for as long as the coolant stays Critical.
	bIsFlashing = !ActiveAlarm.bIsAcknowledged;
	if (!bIsFlashing)
	{
		SetFrameEdge(TwinHmiColors::Critical());
		bIsFrameDim = false;
	}
}

void UTwinAlarmBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bIsFlashing)
	{
		return;
	}
	FlashSeconds = FMath::Fmod(FlashSeconds + InDeltaTime, AlarmBannerTiming::FlashPeriodSeconds);
	const bool bShouldBeDim = FlashSeconds >= AlarmBannerTiming::FlashPeriodSeconds * 0.5f;
	if (bShouldBeDim != bIsFrameDim)
	{
		bIsFrameDim = bShouldBeDim;
		SetFrameEdge(bIsFrameDim ? TwinHmiColors::AlarmEdgeDim() : TwinHmiColors::Critical());
	}
}

void UTwinAlarmBannerWidget::SetFrameEdge(const FLinearColor& EdgeColor)
{
	if (AlarmFrameBorder)
	{
		AlarmFrameBorder->SetBrush(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::AlarmFill(), EdgeColor, 2.f, 5.f));
	}
}

void UTwinAlarmBannerWidget::HandleShowClicked()
{
	if (UDashboardViewModel* DashboardViewModel = FindViewModel<UDashboardViewModel>())
	{
		DashboardViewModel->SelectView(ETwinDashboardView::Powertrain);
	}
}

void UTwinAlarmBannerWidget::HandleAcknowledgeClicked()
{
	if (EventLogViewModel)
	{
		EventLogViewModel->AcknowledgeAlarm();
	}
}