// The dashboard's push button (Show, Acknowledge, Request derate, Cancel derate). C++ draws the fill, edge, padding and label colour
// for every state (normal, hovered, pressed, disabled; primary = cyan edge and label), so WBP_HmiButton only holds the two named
// widgets and the label's font. Each placed copy sets Button Label and Is Primary.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TwinHmiButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FOnTwinHmiButtonClicked);

UCLASS()
class UTwinHmiButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetButtonLabel(const FText& NewButtonLabel);
	void SetButtonEnabled(bool bEnabled);

	// The panels that own the button listen here.
	FOnTwinHmiButtonClicked OnButtonClicked;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ActionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionLabelText;

	UPROPERTY(EditAnywhere, Category = "HMI Button")
	FText ButtonLabel = NSLOCTEXT("TwinHmi", "DefaultButtonLabel", "Button");

	// Cyan edge and label: the action the panel suggests (Show, Request derate).
	UPROPERTY(EditAnywhere, Category = "HMI Button")
	bool bIsPrimary = false;

private:
	UFUNCTION()
	void HandleActionButtonClicked();
};