// Base class for the project's ViewModels: remembers which fields changed and reports them in one batched event per flush
// (docs/SPEC.md §5). Subclasses add the fields, their enum and the getters.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MVVM/ViewModelFieldMask.h"
#include <type_traits>
#include "ViewModelBase.generated.h"

// One event per flush, carrying every field that changed since the previous one. Listeners pull the values through getters.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModelFieldsChanged, FViewModelFieldMask /*ChangedFieldMask*/);

UCLASS(Abstract)
class UViewModelBase : public UObject
{
	GENERATED_BODY()

public:
	// Broadcasts the fields changed since the last flush, if any, and starts collecting again. Called once per frame by
	// UViewModelSubsystem (step 8); until then, and in tests, by hand.
	void Flush();

	// True when a change is waiting for the next flush.
	bool HasChangedFieldsWaitingForFlush() const { return !ChangedFieldsSinceLastFlush.IsEmpty(); }

	FViewModelFieldMask GetChangedFieldsSinceLastFlush() const { return ChangedFieldsSinceLastFlush; }

	// Number of values in the subclass's field enum, not counting its Count entry. At most FViewModelFieldMask::MaxFieldCount.
	virtual int32 GetFieldCount() const PURE_VIRTUAL(UViewModelBase::GetFieldCount, return 0;);

	// Every field of this ViewModel: what a new listener gets first, so it never starts blank (used by Subscribe, step 7).
	FViewModelFieldMask GetAllFields() const { return FViewModelFieldMask::AllFields(GetFieldCount()); }

	// Listeners bind here directly for now. Step 7 adds Subscribe with an RAII handle and makes this private.
	FOnViewModelFieldsChanged OnFieldsChanged;

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
};