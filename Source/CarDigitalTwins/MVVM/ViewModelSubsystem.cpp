#include "MVVM/ViewModelSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Telemetry/TelemetrySubsystem.h"

void UViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// ViewModels connect to the telemetry subsystem when they are created, so it must exist first.
	Collection.InitializeDependency<UTelemetrySubsystem>();
	Super::Initialize(Collection);
	bIsViewModelTickingEnabled = true;
}

void UViewModelSubsystem::Deinitialize()
{
	bIsViewModelTickingEnabled = false;
	for (UViewModelBase* ViewModel : ViewModelsInCreationOrder)
	{
		ViewModel->DeinitializeViewModel();
	}
	ViewModelsInCreationOrder.Reset();
	Super::Deinitialize();
}

void UViewModelSubsystem::Tick(float DeltaSeconds)
{
	// By index: a listener may ask for a ViewModel that doesn't exist yet during the flush, which appends to the list.
	for (int32 ViewModelIndex = 0; ViewModelIndex < ViewModelsInCreationOrder.Num(); ++ViewModelIndex)
	{
		ViewModelsInCreationOrder[ViewModelIndex]->UpdateViewModel(DeltaSeconds);
	}
	for (int32 ViewModelIndex = 0; ViewModelIndex < ViewModelsInCreationOrder.Num(); ++ViewModelIndex)
	{
		ViewModelsInCreationOrder[ViewModelIndex]->Flush();
	}
}

TStatId UViewModelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UViewModelSubsystem, STATGROUP_Tickables);
}

ETickableTickType UViewModelSubsystem::GetTickableTickType() const
{
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

UWorld* UViewModelSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* OwningGameInstance = GetGameInstance();
	return OwningGameInstance ? OwningGameInstance->GetWorld() : nullptr;
}

UViewModelBase* UViewModelSubsystem::GetViewModelOfClass(TSubclassOf<UViewModelBase> ViewModelClass)
{
	check(IsInGameThread());
	check(ViewModelClass);
	for (UViewModelBase* ExistingViewModel : ViewModelsInCreationOrder)
	{
		if (ExistingViewModel->GetClass() == ViewModelClass)
		{
			return ExistingViewModel;
		}
	}

	UViewModelBase* NewViewModel = NewObject<UViewModelBase>(this, ViewModelClass);
	ViewModelsInCreationOrder.Add(NewViewModel);
	NewViewModel->InitializeViewModel(*GetGameInstance());
	return NewViewModel;
}

UViewModelSubsystem* UViewModelSubsystem::FindForWorldContext(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UViewModelSubsystem>() : nullptr;
}
