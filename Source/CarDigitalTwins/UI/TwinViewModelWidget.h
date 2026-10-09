// Base for dashboard widgets that read ViewModels (docs/SPEC.md §5): finds them through UViewModelSubsystem and keeps the
// subscriptions, which end when the widget is destructed. Does nothing at design time: the UMG designer has no game running.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MVVM/ViewModelBase.h"
#include "MVVM/ViewModelSubsystem.h"
#include "TwinViewModelWidget.generated.h"

UCLASS(Abstract)
class UTwinViewModelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeDestruct() override
	{
		ViewModelSubscriptions.Reset();
		Super::NativeDestruct();
	}

	// The game's ViewModel of this class, or null at design time.
	template <typename ViewModelType>
	ViewModelType* FindViewModel() const
	{
		UViewModelSubsystem* ViewModelSubsystem = IsDesignTime() ? nullptr : UViewModelSubsystem::FindForWorldContext(this);
		return ViewModelSubsystem ? ViewModelSubsystem->GetViewModel<ViewModelType>() : nullptr;
	}

	// Stores the ViewModel in OutViewModel first, then subscribes: Subscribe calls the listener right away, and the listener reads
	// the values through that member. A widget with two ViewModels gets its first call while the second member is still null, so
	// listeners check for null.
	template <typename ViewModelType, typename WidgetType>
	void SubscribeToViewModel(TObjectPtr<ViewModelType>& OutViewModel, WidgetType* ListenerWidget,
		void (WidgetType::*ListenerFunction)(FViewModelFieldMask))
	{
		OutViewModel = FindViewModel<ViewModelType>();
		if (OutViewModel)
		{
			ViewModelSubscriptions.Add(OutViewModel->Subscribe(ListenerWidget, ListenerFunction));
		}
	}

private:
	TArray<FViewModelSubscription> ViewModelSubscriptions;
};