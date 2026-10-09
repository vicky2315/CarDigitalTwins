// Which fields of a ViewModel changed: one bit per value of the ViewModel's field enum. The change event carries only this mask;
// widgets then pull the values they need through getters ("notify, then pull", docs/SPEC.md §5).

#pragma once

#include "CoreMinimal.h"
#include <type_traits>
#include "ViewModelFieldMask.generated.h"

USTRUCT(BlueprintType)
struct FViewModelFieldMask
{
	GENERATED_BODY()

	// A field enum may have at most this many values (bits in a uint64).
	static constexpr int32 MaxFieldCount = 64;

	// Not a UPROPERTY: Blueprint has no uint64. Blueprint reads the mask through a function library (later step).
	uint64 ChangedFieldBits = 0;

	// The bit for one field. Enum values must run 0, 1, 2, ... below MaxFieldCount; the ViewModel's field enum checks that with a
	// static_assert on its Count value (step 5).
	template <typename FieldEnumType>
	static constexpr uint64 BitOfField(FieldEnumType Field)
	{
		static_assert(std::is_enum_v<FieldEnumType>, "BitOfField takes a ViewModel field enum value");
		return uint64(1) << static_cast<uint32>(Field);
	}

	// Mask with the first FieldCount fields set: what a new subscriber gets, so its widget never starts blank.
	static FViewModelFieldMask AllFields(int32 FieldCount)
	{
		check(FieldCount >= 0 && FieldCount <= MaxFieldCount);
		FViewModelFieldMask AllFieldsMask;
		// 1 << 64 is undefined in C++, so a full 64-field mask is written out instead of computed.
		AllFieldsMask.ChangedFieldBits = FieldCount == MaxFieldCount ? ~uint64(0) : (uint64(1) << FieldCount) - 1;
		return AllFieldsMask;
	}

	template <typename FieldEnumType>
	void AddField(FieldEnumType Field) { ChangedFieldBits |= BitOfField(Field); }

	template <typename FieldEnumType>
	bool HasField(FieldEnumType Field) const { return (ChangedFieldBits & BitOfField(Field)) != 0; }

	// Same check by number, for Blueprint, which sees the field enum as a byte. Out-of-range indices are simply false.
	bool HasFieldIndex(int32 FieldIndex) const
	{
		return FieldIndex >= 0 && FieldIndex < MaxFieldCount && (ChangedFieldBits & (uint64(1) << FieldIndex)) != 0;
	}

	// Merges another mask in: fields changed in either.
	void AddFields(const FViewModelFieldMask& OtherFieldMask) { ChangedFieldBits |= OtherFieldMask.ChangedFieldBits; }

	bool IsEmpty() const { return ChangedFieldBits == 0; }
	void Reset() { ChangedFieldBits = 0; }

	bool operator==(const FViewModelFieldMask& OtherFieldMask) const { return ChangedFieldBits == OtherFieldMask.ChangedFieldBits; }
	bool operator!=(const FViewModelFieldMask& OtherFieldMask) const { return ChangedFieldBits != OtherFieldMask.ChangedFieldBits; }
};