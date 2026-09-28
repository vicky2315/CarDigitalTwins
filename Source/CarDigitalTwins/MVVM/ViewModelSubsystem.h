// Creates and holds ViewModels (one per class) and flushes dirty ones once per frame. See docs/SPEC.md §5.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MVVM/ViewModelBase.h"
#include "ViewModelSubsystem.generated.h"

UCLASS()
class CARDIGITALTWINS_API UViewModelSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// Returns the ViewModel of this class, creating and initializing it on first request.
	UFUNCTION(BlueprintCallable, Category = "ViewModel", meta = (DeterminesOutputType = "ViewModelClass"))
	UViewModelBase* GetViewModel(TSubclassOf<UViewModelBase> ViewModelClass);

	template <typename T>
	T* GetViewModel()
	{
		return CastChecked<T>(GetViewModel(T::StaticClass()));
	}

	// Called by a ViewModel when its first field becomes dirty since the last flush.
	void RequestFlush(UViewModelBase* ViewModel);

	// Flushes every ViewModel that requested it. Requests made during this flush wait for the next one.
	void FlushAll();

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	UPROPERTY()
	TMap<TObjectPtr<UClass>, TObjectPtr<UViewModelBase>> ViewModels;

	TArray<TWeakObjectPtr<UViewModelBase>> PendingFlush;
};
