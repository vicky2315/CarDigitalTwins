#include "UI/TwinOpsHudWidget.h"

namespace OpsHudLook
{
	// Last data shown, not live: the mock greys the values; here the panel and callouts drop to 60 %.
	static constexpr float NotLiveOpacity = 0.6f;
}

void UTwinOpsHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinOpsHudWidget::HandleViewModelFieldsChanged);
	SubscribeToViewModel(ConnectionViewModel, this, &UTwinOpsHudWidget::HandleViewModelFieldsChanged);
}

void UTwinOpsHudWidget::HandleViewModelFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!TelemetryViewModel || !ConnectionViewModel)
	{
		return;
	}
	const bool bHasTelemetry = TelemetryViewModel->HasReceivedTelemetry();
	const ETelemetryConnectionState ConnectionState = ConnectionViewModel->GetConnectionState();
	const bool bIsShowingOldData = ConnectionState == ETelemetryConnectionState::Stale || ConnectionState == ETelemetryConnectionState::Disconnected;

	if (WaitingPanel)
	{
		WaitingPanel->SetVisibility(bHasTelemetry ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	for (UWidget* DataWidget : { SidePanel.Get(), CalloutLayer.Get() })
	{
		if (DataWidget)
		{
			DataWidget->SetVisibility(bHasTelemetry ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			DataWidget->SetRenderOpacity(bIsShowingOldData ? OpsHudLook::NotLiveOpacity : 1.f);
		}
	}
}