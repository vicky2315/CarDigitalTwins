#include "MVVM/ViewModelBase.h"

FViewModelSubscription::FViewModelSubscription(UViewModelBase* InSubscribedViewModel, FDelegateHandle InListenerDelegateHandle)
	: SubscribedViewModel(InSubscribedViewModel)
	, ListenerDelegateHandle(InListenerDelegateHandle)
{
}

FViewModelSubscription::~FViewModelSubscription()
{
	Reset();
}

FViewModelSubscription::FViewModelSubscription(FViewModelSubscription&& OtherSubscription)
	: SubscribedViewModel(MoveTemp(OtherSubscription.SubscribedViewModel))
	, ListenerDelegateHandle(OtherSubscription.ListenerDelegateHandle)
{
	// The moved-from handle must not remove the listener it no longer owns.
	OtherSubscription.SubscribedViewModel.Reset();
	OtherSubscription.ListenerDelegateHandle.Reset();
}

FViewModelSubscription& FViewModelSubscription::operator=(FViewModelSubscription&& OtherSubscription)
{
	if (this != &OtherSubscription)
	{
		Reset();
		SubscribedViewModel = MoveTemp(OtherSubscription.SubscribedViewModel);
		ListenerDelegateHandle = OtherSubscription.ListenerDelegateHandle;
		OtherSubscription.SubscribedViewModel.Reset();
		OtherSubscription.ListenerDelegateHandle.Reset();
	}
	return *this;
}

void FViewModelSubscription::Reset()
{
	if (UViewModelBase* StillAliveViewModel = SubscribedViewModel.Get())
	{
		StillAliveViewModel->Unsubscribe(ListenerDelegateHandle);
	}
	SubscribedViewModel.Reset();
	ListenerDelegateHandle.Reset();
}

FViewModelSubscription UViewModelBase::SubscribeLambda(TFunction<void(FViewModelFieldMask)> ListenerCallback)
{
	check(IsInGameThread());
	ListenerCallback(GetAllFields());
	const FDelegateHandle NewListenerDelegateHandle = OnFieldsChanged.AddLambda(MoveTemp(ListenerCallback));
	return FViewModelSubscription(this, NewListenerDelegateHandle);
}

void UViewModelBase::Unsubscribe(FDelegateHandle ListenerDelegateHandle)
{
	OnFieldsChanged.Remove(ListenerDelegateHandle);
}

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
