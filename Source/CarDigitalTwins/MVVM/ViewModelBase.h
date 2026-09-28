// Base class for our custom ViewModels: change detection, dirty bits, one batched change event per flush.
// See docs/SPEC.md §5.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MVVM/ViewModelFieldMask.h"
#include "ViewModelBase.generated.h"

class UViewModelBase;
class UViewModelSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModelFieldsChanged, FViewModelFieldMask /*ChangedFields*/);

// Owns one subscription to a ViewModel; unsubscribes when reset or destroyed. Move-only.
class CARDIGITALTWINS_API FViewModelSubscription
{
public:
	FViewModelSubscription() = default;
	FViewModelSubscription(UViewModelBase* InViewModel, FDelegateHandle InHandle);
	~FViewModelSubscription();

	FViewModelSubscription(FViewModelSubscription&& Other);
	FViewModelSubscription& operator=(FViewModelSubscription&& Other);
	FViewModelSubscription(const FViewModelSubscription&) = delete;
	FViewModelSubscription& operator=(const FViewModelSubscription&) = delete;

	void Reset();
	bool IsActive() const { return Handle.IsValid() && ViewModel.IsValid(); }

private:
	TWeakObjectPtr<UViewModelBase> ViewModel;
	FDelegateHandle Handle;
};

UCLASS(Abstract)
class CARDIGITALTWINS_API UViewModelBase : public UObject
{
	GENERATED_BODY()

public:
	// Calls Func once immediately with all fields set, then once per flush with the fields that changed.
	template <typename UserClass>
	[[nodiscard]] FViewModelSubscription Subscribe(UserClass* Object, void (UserClass::*Func)(FViewModelFieldMask))
	{
		check(IsInGameThread());
		const FDelegateHandle NewHandle = OnFieldsChanged.AddUObject(Object, Func);
		(Object->*Func)(AllFields());
		return FViewModelSubscription(this, NewHandle);
	}

	// Same, for lambdas (tests, non-UObject listeners).
	[[nodiscard]] FViewModelSubscription Subscribe(TFunction<void(FViewModelFieldMask)> Callback);

	void Unsubscribe(FDelegateHandle Handle);

	// Broadcasts the pending changes (if any) and clears them. Normally called by UViewModelSubsystem once per frame.
	void Flush();

	bool IsDirty() const { return !DirtyFields.IsEmpty(); }
	FViewModelFieldMask GetDirtyFields() const { return DirtyFields; }

	// Number of entries in the subclass's field enum (excluding Count).
	virtual int32 GetNumFields() const PURE_VIRTUAL(UViewModelBase::GetNumFields, return 0;);

	// Called once by UViewModelSubsystem after creation; subclasses subscribe to their data sources here.
	virtual void InitializeViewModel(UGameInstance* GameInstance) {}
	virtual void DeinitializeViewModel() {}

	FViewModelFieldMask AllFields() const { return FViewModelFieldMask::All(GetNumFields()); }

protected:
	// Stores NewValue and marks Field dirty if it differs from the current value. Returns true if it changed.
	template <typename EnumT, typename ValueT>
	bool SetField(EnumT Field, ValueT& Storage, const ValueT& NewValue)
	{
		if (Storage == NewValue)
		{
			return false;
		}
		Storage = NewValue;
		MarkDirty(Field);
		return true;
	}

	// Floating-point version: changes within Tolerance are ignored (Storage keeps the old value, so slow drift still
	// triggers once it exceeds Tolerance).
	template <typename EnumT, typename FloatT>
	bool SetField(EnumT Field, FloatT& Storage, FloatT NewValue, FloatT Tolerance)
	{
		static_assert(std::is_floating_point_v<FloatT>, "Tolerance overload is for float/double");
		if (FMath::Abs(Storage - NewValue) <= Tolerance)
		{
			return false;
		}
		Storage = NewValue;
		MarkDirty(Field);
		return true;
	}

	template <typename EnumT>
	void MarkDirty(EnumT Field)
	{
		check(IsInGameThread());
		checkf(static_cast<int32>(Field) < GetNumFields(), TEXT("Field index out of range for %s"), *GetName());
		const bool bWasClean = DirtyFields.IsEmpty();
		DirtyFields.Add(Field);
		if (bWasClean)
		{
			RequestFlush();
		}
	}

private:
	friend class UViewModelSubsystem;

	void RequestFlush();

	// Set by UViewModelSubsystem. Null for ViewModels created outside it (tests), which must call Flush() themselves.
	TWeakObjectPtr<UViewModelSubsystem> OwnerSubsystem;

	FViewModelFieldMask DirtyFields;
	FOnViewModelFieldsChanged OnFieldsChanged;
};
