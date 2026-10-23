// One line of text capped at MaxVisibleWidth. Text that fits stays still; longer text holds at its start, scrolls right to left
// until its end is visible, holds again and jumps back. WBP_MarqueeText holds SizeBox > ScrollBox > TextBlock; C++ sets the
// ScrollBox up (horizontal, no scroll bar, no mouse wheel) and drives its offset.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TwinMarqueeTextWidget.generated.h"

class UScrollBox;
class USizeBox;
class UTextBlock;

UCLASS()
class UTwinMarqueeTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Marquee Text")
	void SetText(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category = "Marquee Text")
	void SetTextColor(const FSlateColor& NewColor);

	// Back to the start of the text and its first hold, e.g. when the text now says something else.
	UFUNCTION(BlueprintCallable, Category = "Marquee Text")
	void RestartScroll();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> MarqueeSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> MarqueeScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MarqueeText;

	// Widest the text shows before it scrolls, in slate units.
	UPROPERTY(EditAnywhere, Category = "Marquee Text", meta = (ClampMin = "1"))
	float MaxVisibleWidth = 200.f;

	UPROPERTY(EditAnywhere, Category = "Marquee Text", meta = (ClampMin = "1"))
	float ScrollSpeed = 30.f;

	// Hold at the start and at the end of each pass, so both ends can be read.
	UPROPERTY(EditAnywhere, Category = "Marquee Text", meta = (ClampMin = "0"))
	float HoldSeconds = 1.5f;

private:
	enum class EMarqueePhase : uint8 { HoldStart, Scrolling, HoldEnd };

	EMarqueePhase Phase = EMarqueePhase::HoldStart;
	float PhaseSeconds = 0.f;
	float ScrollOffset = 0.f;
};
