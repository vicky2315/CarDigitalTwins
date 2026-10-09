#include "UI/TwinPartRingWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "UI/TwinHmiStyle.h"

void UTwinPartRingWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	// Visible in the designer so it can be checked; hidden at Play until its part isn't Normal.
	if (RingImage)
	{
		RingImage->SetColorAndOpacity(TwinHmiColors::Warning());
	}
	SetVisibility(IsDesignTime() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTwinPartRingWidget::ShowPartStatus(EVehicleStatus PartStatus)
{
	if (PartStatus == ShownStatus)
	{
		return;
	}
	ShownStatus = PartStatus;
	if (PartStatus == EVehicleStatus::Normal)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		if (PulseAnimation)
		{
			StopAnimation(PulseAnimation);
		}
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (RingImage)
	{
		RingImage->SetColorAndOpacity(TwinHmiColors::ForStatus(PartStatus, TwinHmiColors::Warning()));
	}
	if (PulseAnimation && !IsAnimationPlaying(PulseAnimation))
	{
		// 0 loops = forever.
		PlayAnimation(PulseAnimation, 0.f, 0);
	}
}