// Creates the ViewModels on first request, keeps them for the whole game session and flushes the changed ones once per frame
// (docs/SPEC.md §5). Widgets get their ViewModels here; nothing else creates them.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MVVM/ViewModelBase.h"
#include "ViewModelSubsystem.generated.h"

// GameInstance subsystem, like UTelemetrySubsystem, so ViewModels and their state survive level loads.
UCLASS()
class UViewModelSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//~ USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//~ FTickableGameObject
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override { return bIsViewModelTickingEnabled; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;

	// The one ViewModel of this class, created and initialized on the first call. Game thread only.
	template <typename ViewModelType>
	ViewModelType* GetViewModel()
	{
		return CastChecked<ViewModelType>(GetViewModelOfClass(ViewModelType::StaticClass()));
	}

	UViewModelBase* GetViewModelOfClass(TSubclassOf<UViewModelBase> ViewModelClass);

	// Shortcut for widgets and actors: the subsystem of the game instance WorldContextObject belongs to, or null (editor, no game).
	static UViewModelSubsystem* FindForWorldContext(const UObject* WorldContextObject);

private:
	// Few ViewModels (four), so a list is enough; creation order is also the flush order, which keeps it predictable.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UViewModelBase>> ViewModelsInCreationOrder;

	// True between Initialize and Deinitialize. The class default object is also a tickable object, so ticking must be opt-in.
	bool bIsViewModelTickingEnabled = false;
};
