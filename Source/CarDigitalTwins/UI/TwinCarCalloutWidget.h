// A label pinned to a point on the car: a small box (what, value, unit) on a stem whose bottom end sits on the anchor. Each placed
// copy says which anchor (AnchorTag), in which view it shows, and which value it carries; UTwinCarCalloutLayerWidget places it every
// frame and feeds the value.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MVVM/DashboardViewModel.h"
#include "TwinCarCalloutWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

// The value a callout shows.
UENUM(BlueprintType)
enum class ETwinCalloutValue : uint8
{
	CoolantTemperature,
	EngineRpm,
	TyreFrontLeft,
	TyreFrontRight,
	TyreRearLeft,
	TyreRearRight,
	DoorFrontLeft,
	Hood,
};

UCLASS()
class UTwinCarCalloutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowCalloutValue(const FText& ValueText, const FText& UnitText, const FLinearColor& ValueColor, const FLinearColor& EdgeColor);

	FName GetAnchorTag() const { return AnchorTag; }
	const FVector& GetAnchorOffsetInCarSpace() const { return AnchorOffsetInCarSpace; }
	ETwinDashboardView GetShownInView() const { return ShownInView; }
	ETwinCalloutValue GetCalloutValue() const { return CalloutValue; }

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> CalloutBoxBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CalloutKeyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CalloutValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CalloutUnitText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CalloutStemImage;

	// Wheel.FL ... Wheel.RR for a tyre centre, or CalloutAnchor.Hood / CalloutAnchor.Engine / CalloutAnchor.DoorFrontLeft.
	UPROPERTY(EditAnywhere, Category = "Car Callout")
	FName AnchorTag = TEXT("CalloutAnchor.Hood");

	// Moves the stem's end away from the anchor in the car's axes (cm, X forward, Y right, Z up), e.g. out from a wheel.
	UPROPERTY(EditAnywhere, Category = "Car Callout")
	FVector AnchorOffsetInCarSpace = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Car Callout")
	ETwinDashboardView ShownInView = ETwinDashboardView::Powertrain;

	UPROPERTY(EditAnywhere, Category = "Car Callout")
	ETwinCalloutValue CalloutValue = ETwinCalloutValue::CoolantTemperature;

	// The small caps line above the value: "Coolant", "FL", "Door FL".
	UPROPERTY(EditAnywhere, Category = "Car Callout")
	FText CalloutKey = NSLOCTEXT("TwinHmi", "DefaultCalloutKey", "Coolant");
};