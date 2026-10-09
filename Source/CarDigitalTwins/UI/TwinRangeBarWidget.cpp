#include "UI/TwinRangeBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/TwinHmiStyle.h"

// Px at 1080p, as in the mock: track 10 high at y 10, marker 6 × 26 at y 2, tick labels from y 26.
namespace RangeBarLayout
{
	static constexpr float TotalHeight = 44.f;
	static constexpr float TrackTop = 10.f;
	static constexpr float TrackHeight = 10.f;
	static constexpr float MarkerTop = 2.f;
	static constexpr float MarkerWidth = 6.f;
	static constexpr float MarkerHeight = 26.f;
	static constexpr float MarkerOutline = 2.f;
	static constexpr float LimitLineTop = 4.f;
	static constexpr float LimitLineHeight = 22.f;
	static constexpr float LimitLineWidth = 2.f;
	static constexpr float TickTop = 26.f;
}

void UTwinRangeBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BuildBarParts();
}

void UTwinRangeBarWidget::SetBarValue(float NewValue, EVehicleStatus NewStatus)
{
	CurrentValue = NewValue;
	CurrentStatus = NewStatus;
	bHasRuntimeValue = true;
	if (!MarkerImage)
	{
		BuildBarParts();
	}
	PlaceMarkerAndLimitLine();
}

void UTwinRangeBarWidget::SetLimitLine(bool bShowLimitLine, float NewLimitValue)
{
	bIsLimitLineShown = bShowLimitLine;
	LimitValue = NewLimitValue;
	if (!MarkerImage)
	{
		BuildBarParts();
	}
	PlaceMarkerAndLimitLine();
}

float UTwinRangeBarWidget::ValueToBarX(float Value) const
{
	const float RangeSize = FMath::Max(MaxValue - MinValue, KINDA_SMALL_NUMBER);
	return FMath::Clamp((Value - MinValue) / RangeSize, 0.f, 1.f) * BarWidth;
}

void UTwinRangeBarWidget::BuildBarParts()
{
	if (!BarCanvas || !WidgetTree)
	{
		return;
	}
	using namespace RangeBarLayout;
	if (BarSizeBox)
	{
		BarSizeBox->SetWidthOverride(BarWidth);
		BarSizeBox->SetHeightOverride(TotalHeight);
	}
	BarCanvas->ClearChildren();

	auto AddImage = [this](const FSlateBrush& ImageBrush, FVector2D Position, FVector2D Size)
	{
		UImage* NewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		NewImage->SetBrush(ImageBrush);
		UCanvasPanelSlot* ImageSlot = BarCanvas->AddChildToCanvas(NewImage);
		ImageSlot->SetAutoSize(false);
		ImageSlot->SetPosition(Position);
		ImageSlot->SetSize(Size);
		return NewImage;
	};
	auto AddZone = [&](float FromValue, float ToValue, const FLinearColor& ZoneColor)
	{
		const float ZoneLeft = ValueToBarX(FromValue);
		const float ZoneRight = ValueToBarX(ToValue);
		if (ZoneRight > ZoneLeft)
		{
			AddImage(TwinHmiBrushes::MakeRoundedBox(ZoneColor, ZoneColor, 0.f, 0.f), FVector2D(ZoneLeft, TrackTop), FVector2D(ZoneRight - ZoneLeft, TrackHeight));
		}
	};

	AddImage(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::BarTrack(), TwinHmiColors::BarTrack(), 0.f, 2.f), FVector2D(0.f, TrackTop),
		FVector2D(BarWidth, TrackHeight));
	if (bHasCriticalBelow)
	{
		AddZone(MinValue, CriticalBelow, TwinHmiColors::BarCriticalZone());
	}
	if (bHasWarningBelow)
	{
		AddZone(bHasCriticalBelow ? CriticalBelow : MinValue, WarningBelow, TwinHmiColors::BarWarningZone());
	}
	if (bHasWarningAbove)
	{
		AddZone(WarningAbove, bHasCriticalAbove ? CriticalAbove : MaxValue, TwinHmiColors::BarWarningZone());
	}
	if (bHasCriticalAbove)
	{
		AddZone(CriticalAbove, MaxValue, TwinHmiColors::BarCriticalZone());
	}

	// UMG has no dashed line: a solid amber line at 80 % opacity stands in for the mock's dashed cap.
	LimitLineImage = AddImage(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::Warning().CopyWithNewOpacity(0.8f), TwinHmiColors::Transparent(), 0.f, 0.f),
		FVector2D(0.f, LimitLineTop), FVector2D(LimitLineWidth, LimitLineHeight));
	MarkerOutlineImage = AddImage(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::SceneBackground(), TwinHmiColors::Transparent(), 0.f, 3.f),
		FVector2D::ZeroVector, FVector2D(MarkerWidth + 2.f * MarkerOutline, MarkerHeight + 2.f * MarkerOutline));
	MarkerImage = AddImage(TwinHmiBrushes::MakeRoundedBox(TwinHmiColors::TextPrimary(), TwinHmiColors::Transparent(), 0.f, 2.f),
		FVector2D::ZeroVector, FVector2D(MarkerWidth, MarkerHeight));

	for (const float TickValue : TickValues)
	{
		UTextBlock* TickText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TickText->SetText(TwinHmiFormat::FormatFixed(TickValue, 0));
		TickText->SetFont(TickFont);
		TickText->SetColorAndOpacity(FSlateColor(TwinHmiColors::TextTertiary()));
		UCanvasPanelSlot* TickSlot = BarCanvas->AddChildToCanvas(TickText);
		TickSlot->SetAutoSize(true);
		TickSlot->SetAlignment(FVector2D(0.5f, 0.f));
		TickSlot->SetPosition(FVector2D(ValueToBarX(TickValue), TickTop));
	}

	if (!bHasRuntimeValue)
	{
		CurrentValue = PreviewValue;
	}
	PlaceMarkerAndLimitLine();
}

void UTwinRangeBarWidget::PlaceMarkerAndLimitLine()
{
	if (!MarkerImage || !MarkerOutlineImage || !LimitLineImage)
	{
		return;
	}
	using namespace RangeBarLayout;
	const float MarkerCentreX = ValueToBarX(CurrentValue);
	Cast<UCanvasPanelSlot>(MarkerImage->Slot)->SetPosition(FVector2D(MarkerCentreX - MarkerWidth * 0.5f, MarkerTop));
	Cast<UCanvasPanelSlot>(MarkerOutlineImage->Slot)->SetPosition(
		FVector2D(MarkerCentreX - MarkerWidth * 0.5f - MarkerOutline, MarkerTop - MarkerOutline));
	MarkerImage->SetColorAndOpacity(TwinHmiColors::ForStatus(CurrentStatus, TwinHmiColors::TextPrimary()));

	LimitLineImage->SetVisibility(bIsLimitLineShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Cast<UCanvasPanelSlot>(LimitLineImage->Slot)->SetPosition(FVector2D(ValueToBarX(LimitValue) - LimitLineWidth * 0.5f, LimitLineTop));
}