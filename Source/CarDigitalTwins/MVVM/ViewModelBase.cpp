#include "MVVM/ViewModelBase.h"

void UViewModelBase::Flush()
{
	check(IsInGameThread());
	if (ChangedFieldsSinceLastFlush.IsEmpty())
	{
		return;
	}

	// Copy and clear before broadcasting: a listener that changes a field during the broadcast then marks it for the next flush
	// (next frame) instead of having its change wiped out when this flush finishes (SPEC.md §5).
	const FViewModelFieldMask ChangedFieldMask = ChangedFieldsSinceLastFlush;
	ChangedFieldsSinceLastFlush.Reset();
	OnFieldsChanged.Broadcast(ChangedFieldMask);
}