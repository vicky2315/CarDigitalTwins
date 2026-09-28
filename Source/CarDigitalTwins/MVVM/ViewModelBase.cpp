#include "MVVM/ViewModelBase.h"

#include "MVVM/ViewModelSubsystem.h"

FViewModelSubscription::FViewModelSubscription(UViewModelBase* InViewModel, FDelegateHandle InHandle)
	: ViewModel(InViewModel)
	, Handle(InHandle)
{
}

FViewModelSubscription::~FViewModelSubscription()
{
	Reset();
}

FViewModelSubscription::FViewModelSubscription(FViewModelSubscription&& Other)
	: ViewModel(MoveTemp(Other.ViewModel))
	, Handle(Other.Handle)
{
	Other.ViewModel.Reset();
	Other.Handle.Reset();
}

FViewModelSubscription& FViewModelSubscription::operator=(FViewModelSubscription&& Other)
{
	if (this != &Other)
	{
		Reset();
		ViewModel = MoveTemp(Other.ViewModel);
		Handle = Other.Handle;
		Other.ViewModel.Reset();
		Other.Handle.Reset();
	}
	return *this;
}

void FViewModelSubscription::Reset()
{
	if (UViewModelBase* VM = ViewModel.Get())
	{
		VM->Unsubscribe(Handle);
	}
	ViewModel.Reset();
	Handle.Reset();
}

FViewModelSubscription UViewModelBase::Subscribe(TFunction<void(FViewModelFieldMask)> Callback)
{
	check(IsInGameThread());
	Callback(AllFields());
	const FDelegateHandle NewHandle = OnFieldsChanged.AddLambda(MoveTemp(Callback));
	return FViewModelSubscription(this, NewHandle);
}

void UViewModelBase::Unsubscribe(FDelegateHandle Handle)
{
	OnFieldsChanged.Remove(Handle);
}

void UViewModelBase::Flush()
{
	check(IsInGameThread());
	if (DirtyFields.IsEmpty())
	{
		return;
	}
	// Clear before broadcasting so a listener that sets a field re-registers for the next flush.
	const FViewModelFieldMask Changed = DirtyFields;
	DirtyFields.Reset();
	OnFieldsChanged.Broadcast(Changed);
}

void UViewModelBase::RequestFlush()
{
	if (UViewModelSubsystem* Subsystem = OwnerSubsystem.Get())
	{
		Subsystem->RequestFlush(this);
	}
}
