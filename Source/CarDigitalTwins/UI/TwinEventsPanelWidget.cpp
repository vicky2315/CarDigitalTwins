#include "UI/TwinEventsPanelWidget.h"

#include "Components/TextBlock.h"
#include "UI/TwinEventRowWidget.h"

void UTwinEventsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(EventLogViewModel, this, &UTwinEventsPanelWidget::HandleEventLogFieldsChanged);
}

void UTwinEventsPanelWidget::HandleEventLogFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!EventLogViewModel || !ChangedFieldMask.HasField(EVehicleEventLogViewModelField::RecentEvents))
	{
		return;
	}
	const TArray<FVehicleEventLogEntry>& RecentEvents = EventLogViewModel->GetRecentEvents();
	UTwinEventRowWidget* EventRows[] = { EventRow0, EventRow1, EventRow2, EventRow3, EventRow4 };
	for (int32 RowIndex = 0; RowIndex < UE_ARRAY_COUNT(EventRows); ++RowIndex)
	{
		if (!EventRows[RowIndex])
		{
			continue;
		}
		if (RecentEvents.IsValidIndex(RowIndex))
		{
			EventRows[RowIndex]->ShowEvent(RecentEvents[RowIndex]);
		}
		else
		{
			EventRows[RowIndex]->ShowNoEvent();
		}
	}
	if (EmptyEventsText)
	{
		EmptyEventsText->SetVisibility(RecentEvents.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}