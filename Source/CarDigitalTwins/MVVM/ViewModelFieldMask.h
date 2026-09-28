// Set of changed fields, one bit per field of a ViewModel's field enum. See docs/SPEC.md §5.

#pragma once

#include "CoreMinimal.h"
#include "ViewModelFieldMask.generated.h"

USTRUCT(BlueprintType)
struct FViewModelFieldMask
{
	GENERATED_BODY()

	// Not a UPROPERTY: Blueprint has no uint64. Blueprint access comes via a function library (next step).
	uint64 Bits = 0;

	static constexpr int32 MaxFields = 64;

	template <typename EnumT>
	static constexpr uint64 BitOf(EnumT Field)
	{
		static_assert(std::is_enum_v<EnumT>, "Field must be an enum");
		return uint64(1) << static_cast<uint32>(Field);
	}

	static FViewModelFieldMask All(int32 NumFields)
	{
		check(NumFields >= 0 && NumFields <= MaxFields);
		FViewModelFieldMask Mask;
		Mask.Bits = NumFields == MaxFields ? ~uint64(0) : (uint64(1) << NumFields) - 1;
		return Mask;
	}

	template <typename EnumT>
	void Add(EnumT Field) { Bits |= BitOf(Field); }

	template <typename EnumT>
	bool Has(EnumT Field) const { return (Bits & BitOf(Field)) != 0; }

	bool HasIndex(int32 FieldIndex) const
	{
		return FieldIndex >= 0 && FieldIndex < MaxFields && (Bits & (uint64(1) << FieldIndex)) != 0;
	}

	bool IsEmpty() const { return Bits == 0; }
	void Reset() { Bits = 0; }

	bool operator==(const FViewModelFieldMask& Other) const { return Bits == Other.Bits; }
	bool operator!=(const FViewModelFieldMask& Other) const { return Bits != Other.Bits; }
};
