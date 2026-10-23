#include "UI/TwinMarqueeTextWidget.h"

#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void UTwinMarqueeTextWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (MarqueeSizeBox)
	{
		MarqueeSizeBox->SetMaxDesiredWidth(MaxVisibleWidth);
	}
	if (MarqueeScrollBox)
	{
		// Only C++ moves it: no bar, no wheel, no overscroll; clipped so the hidden part never draws over neighbours.
		MarqueeScrollBox->SetOrientation(Orient_Horizontal);
		MarqueeScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		MarqueeScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
		MarqueeScrollBox->SetAllowOverscroll(false);
		MarqueeScrollBox->SetAnimateWheelScrolling(false);
		MarqueeScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
		MarqueeScrollBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (MarqueeText)
	{
		MarqueeText->SetAutoWrapText(false);
	}
	RestartScroll();
}

void UTwinMarqueeTextWidget::SetText(const FText& NewText)
{
	if (MarqueeText)
	{
		MarqueeText->SetText(NewText);
	}
}

void UTwinMarqueeTextWidget::SetTextColor(const FSlateColor& NewColor)
{
	if (MarqueeText)
	{
		MarqueeText->SetColorAndOpacity(NewColor);
	}
}

void UTwinMarqueeTextWidget::RestartScroll()
{
	Phase = EMarqueePhase::HoldStart;
	PhaseSeconds = 0.f;
	ScrollOffset = 0.f;
	if (MarqueeScrollBox)
	{
		MarqueeScrollBox->SetScrollOffset(0.f);
	}
}

void UTwinMarqueeTextWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!MarqueeScrollBox)
	{
		return;
	}
	// How far the text runs past the visible width, from the last layout; 0 when it fits.
	const float OverflowWidth = MarqueeScrollBox->GetScrollOffsetOfEnd();
	if (OverflowWidth <= 0.f)
	{
		if (ScrollOffset != 0.f)
		{
			RestartScroll();
		}
		return;
	}

	PhaseSeconds += InDeltaTime;
	switch (Phase)
	{
	case EMarqueePhase::HoldStart:
		if (PhaseSeconds >= HoldSeconds)
		{
			Phase = EMarqueePhase::Scrolling;
			PhaseSeconds = 0.f;
		}
		break;
	case EMarqueePhase::Scrolling:
		ScrollOffset = FMath::Min(ScrollOffset + ScrollSpeed * InDeltaTime, OverflowWidth);
		if (ScrollOffset >= OverflowWidth)
		{
			Phase = EMarqueePhase::HoldEnd;
			PhaseSeconds = 0.f;
		}
		break;
	case EMarqueePhase::HoldEnd:
		if (PhaseSeconds >= HoldSeconds)
		{
			RestartScroll();
			return;
		}
		break;
	}
	// The text can get shorter mid-pass (e.g. a countdown): never scroll past its new end.
	ScrollOffset = FMath::Min(ScrollOffset, OverflowWidth);
	MarqueeScrollBox->SetScrollOffset(ScrollOffset);
}
