#include "MVVM/ViewModelSubsystem.h"

#include "Engine/GameInstance.h"

void UViewModelSubsystem::Deinitialize()
{
	for (const TPair<TObjectPtr<UClass>, TObjectPtr<UViewModelBase>>& Pair : ViewModels)
	{
		if (Pair.Value)
		{
			Pair.Value->DeinitializeViewModel();
		}
	}
	ViewModels.Reset();
	PendingFlush.Reset();
	Super::Deinitialize();
}

UViewModelBase* UViewModelSubsystem::GetViewModel(TSubclassOf<UViewModelBase> ViewModelClass)
{
	check(IsInGameThread());
	if (!ViewModelClass || ViewModelClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}

	if (TObjectPtr<UViewModelBase>* Existing = ViewModels.Find(ViewModelClass.Get()))
	{
		return *Existing;
	}

	UViewModelBase* ViewModel = NewObject<UViewModelBase>(this, ViewModelClass);
	ViewModel->OwnerSubsystem = this;
	ViewModels.Add(ViewModelClass.Get(), ViewModel);
	ViewModel->InitializeViewModel(GetGameInstance());
	return ViewModel;
}

void UViewModelSubsystem::RequestFlush(UViewModelBase* ViewModel)
{
	PendingFlush.AddUnique(ViewModel);
}

void UViewModelSubsystem::FlushAll()
{
	// Swap out first: listeners may set fields during Flush(), which re-adds to PendingFlush for next frame.
	TArray<TWeakObjectPtr<UViewModelBase>> ToFlush = MoveTemp(PendingFlush);
	PendingFlush.Reset();
	for (const TWeakObjectPtr<UViewModelBase>& WeakViewModel : ToFlush)
	{
		if (UViewModelBase* ViewModel = WeakViewModel.Get())
		{
			ViewModel->Flush();
		}
	}
}

void UViewModelSubsystem::Tick(float DeltaTime)
{
	FlushAll();
}

TStatId UViewModelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UViewModelSubsystem, STATGROUP_Tickables);
}

ETickableTickType UViewModelSubsystem::GetTickableTickType() const
{
	// The class default object must never tick.
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool UViewModelSubsystem::IsTickable() const
{
	return !PendingFlush.IsEmpty();
}
