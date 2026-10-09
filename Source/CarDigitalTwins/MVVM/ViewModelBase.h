// Base class for the project's ViewModels: remembers which fields changed and reports them in one batched event per flush
// (docs/SPEC.md §5). Subclasses add the fields, their enum and the getters.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MVVM/ViewModelFieldMask.h"
#include <type_traits>
#include "ViewModelBase.generated.h"

class UGameInstance;
class UViewModelBase;

// One event per flush, carrying every field that changed since the previous one. Listeners pull the values through getters.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModelFieldsChanged, FViewModelFieldMask /*ChangedFieldMask*/);

// Owns one listener registration on a ViewModel and removes it when reset, overwritten or destroyed, so a widget that keeps its
// subscriptions as members can never be called after it is gone. Move-only: exactly one owner per registration.
class FViewModelSubscription
{
public:
	FViewModelSubscription() = default;
	FViewModelSubscription(UViewModelBase* InSubscribedViewModel, FDelegateHandle InListenerDelegateHandle);
	~FViewModelSubscription();

	FViewModelSubscription(FViewModelSubscription&& OtherSubscription);
	FViewModelSubscription& operator=(FViewModelSubscription&& OtherSubscription);
	FViewModelSubscription(const FViewModelSubscription&) = delete;
	FViewModelSubscription& operator=(const FViewModelSubscription&) = delete;

	// Removes the listener now. Safe to call twice, and after the ViewModel is gone.
	void Reset();

	bool IsActive() const { return ListenerDelegateHandle.IsValid() && SubscribedViewModel.IsValid(); }

private:
	TWeakObjectPtr<UViewModelBase> SubscribedViewModel;
	FDelegateHandle ListenerDelegateHandle;
};

UCLASS(Abstract)
class UViewModelBase : public UObject
{
	GENERATED_BODY()

public:
	// Calls ListenerFunction once right away with every field set, so the widget fills itself in instead of starting blank, then once
	// per flush with the fields that changed. Keep the returned handle: the listener stays registered exactly as long as it lives.
	template <typename ListenerObjectType>
	[[nodiscard]] FViewModelSubscription Subscribe(ListenerObjectType* ListenerObject, void (ListenerObjectType::*ListenerFunction)(FViewModelFieldMask))
	{
		check(IsInGameThread());
		const FDelegateHandle NewListenerDelegateHandle = OnFieldsChanged.AddUObject(ListenerObject, ListenerFunction);
		(ListenerObject->*ListenerFunction)(GetAllFields());
		return FViewModelSubscription(this, NewListenerDelegateHandle);
	}

	// Same for a lambda, e.g. in tests or for listeners that aren't UObjects. The lambda must not outlive what it captures.
	[[nodiscard]] FViewModelSubscription SubscribeLambda(TFunction<void(FViewModelFieldMask)> ListenerCallback);

	// Used by FViewModelSubscription; call Reset on the handle instead.
	void Unsubscribe(FDelegateHandle ListenerDelegateHandle);

	// Called once by UViewModelSubsystem right after it creates the ViewModel: connect to the data sources here.
	virtual void InitializeViewModel(UGameInstance& OwningGameInstance) {}

	// Called once when the game instance shuts down: disconnect from the data sources.
	virtual void DeinitializeViewModel() {}

	// Called by UViewModelSubsystem every frame just before the flush, for ViewModels that poll a source instead of being pushed
	// (the connection state). Most ViewModels leave it empty.
	virtual void UpdateViewModel(float DeltaSeconds) {}

	// Broadcasts the fields changed since the last flush, if any, and starts collecting again. Called once per frame by
	// UViewModelSubsystem; by hand in tests.
	void Flush();

	// True when a change is waiting for the next flush.
	bool HasChangedFieldsWaitingForFlush() const { return !ChangedFieldsSinceLastFlush.IsEmpty(); }

	FViewModelFieldMask GetChangedFieldsSinceLastFlush() const { return ChangedFieldsSinceLastFlush; }

	// Number of values in the subclass's field enum, not counting its Count entry. At most FViewModelFieldMask::MaxFieldCount.
	virtual int32 GetFieldCount() const PURE_VIRTUAL(UViewModelBase::GetFieldCount, return 0;);

	// Every field of this ViewModel: what a new listener gets first, so it never starts blank.
	FViewModelFieldMask GetAllFields() const { return FViewModelFieldMask::AllFields(GetFieldCount()); }

protected:
	// Stores NewValue in FieldStorage and marks Field changed, but only if the value is really different. Returns true if it changed.
	// For ints, bools, enums, strings. Floats must use the tolerance version below, so noise doesn't mark them on every sample.
	// NewValue's type is taken from FieldStorage (std::type_identity_t), so SetField(Field, GearStorage, 3) works for any int type.
	template <typename FieldEnumType, typename FieldValueType>
	bool SetField(FieldEnumType Field, FieldValueType& FieldStorage, const std::type_identity_t<FieldValueType>& NewValue)
	{
		static_assert(!std::is_floating_point_v<FieldValueType>, "Float fields need a tolerance: SetField(Field, Storage, NewValue, Tolerance)");
		if (FieldStorage == NewValue)
		{
			return false;
		}
		FieldStorage = NewValue;
		MarkFieldChanged(Field);
		return true;
	}

	// Float version: a change of at most ChangeTolerance is ignored, and FieldStorage then keeps the last reported value. So slow drift
	// still gets through once it adds up past the tolerance, instead of being swallowed step by step. A tolerance of 0 = any change.
	template <typename FieldEnumType, typename FloatFieldType>
	bool SetField(FieldEnumType Field, FloatFieldType& FieldStorage, std::type_identity_t<FloatFieldType> NewValue,
		std::type_identity_t<FloatFieldType> ChangeTolerance)
	{
		static_assert(std::is_floating_point_v<FloatFieldType>, "The tolerance version is for float and double fields");
		if (FMath::Abs(FieldStorage - NewValue) <= ChangeTolerance)
		{
			return false;
		}
		FieldStorage = NewValue;
		MarkFieldChanged(Field);
		return true;
	}

	// Records that Field changed; it goes out with the next Flush. Several changes to one field before a flush give one bit, one event.
	// Game thread only: the mask isn't locked.
	template <typename FieldEnumType>
	void MarkFieldChanged(FieldEnumType Field)
	{
		check(IsInGameThread());
		checkf(static_cast<int32>(Field) >= 0 && static_cast<int32>(Field) < GetFieldCount(), TEXT("%s: field %d is outside its field enum (%d fields)."),
			*GetName(), static_cast<int32>(Field), GetFieldCount());
		ChangedFieldsSinceLastFlush.AddField(Field);
	}

private:
	FViewModelFieldMask ChangedFieldsSinceLastFlush;

	// Private: listeners go through Subscribe, which hands out the handle that removes them again.
	FOnViewModelFieldsChanged OnFieldsChanged;
};