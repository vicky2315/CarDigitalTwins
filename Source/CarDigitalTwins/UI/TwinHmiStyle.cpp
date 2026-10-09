#include "UI/TwinHmiStyle.h"

namespace TwinHmiColors
{
	FLinearColor FromSrgbHex(uint32 RedGreenBlueHex, float Opacity)
	{
		// FLinearColor(FColor) decodes sRGB to linear, the same as pasting the hex into the colour picker's "Hex sRGB" field.
		const FColor SrgbColor((RedGreenBlueHex >> 16) & 0xFF, (RedGreenBlueHex >> 8) & 0xFF, RedGreenBlueHex & 0xFF);
		return FLinearColor(SrgbColor).CopyWithNewOpacity(Opacity);
	}

	FLinearColor PanelFill() { return FromSrgbHex(0x0E1419, 0.80f); }
	FLinearColor PanelEdge() { return FromSrgbHex(0x2A3741); }
	FLinearColor PanelEdgeHighlight() { return FromSrgbHex(0x3B4B57); }
	FLinearColor TextPrimary() { return FromSrgbHex(0xE7EDF1); }
	FLinearColor TextSecondary() { return FromSrgbHex(0x93A2AD); }
	FLinearColor TextTertiary() { return FromSrgbHex(0x5F6E79); }
	FLinearColor Accent() { return FromSrgbHex(0x56B8DE); }
	FLinearColor AccentSoft() { return FromSrgbHex(0x56B8DE, 0.16f); }
	FLinearColor Warning() { return FromSrgbHex(0xF0A73A); }
	FLinearColor WarningSoft() { return FromSrgbHex(0xF0A73A, 0.16f); }
	FLinearColor Critical() { return FromSrgbHex(0xF2564F); }
	FLinearColor CriticalSoft() { return FromSrgbHex(0xF2564F, 0.18f); }
	FLinearColor LinkLive() { return FromSrgbHex(0x47C389); }
	FLinearColor BarTrack() { return FromSrgbHex(0x24313A); }
	FLinearColor BarWarningZone() { return FromSrgbHex(0x4B3B1D); }
	FLinearColor BarCriticalZone() { return FromSrgbHex(0x4F2423); }
	FLinearColor SceneBackground() { return FromSrgbHex(0x0B1014); }
	FLinearColor ButtonFill() { return FromSrgbHex(0x18222A); }
	FLinearColor ButtonFillHovered() { return FromSrgbHex(0x202C35); }
	FLinearColor ButtonFillPressed() { return FromSrgbHex(0x141C22); }
	FLinearColor HoverTint() { return FromSrgbHex(0xFFFFFF, 0.04f); }
	FLinearColor AlarmFill() { return FromSrgbHex(0x2A1414, 0.95f); }
	FLinearColor AlarmEdgeDim() { return FromSrgbHex(0x5A2A28); }
	FLinearColor Transparent() { return FLinearColor::Transparent; }

	FLinearColor ForStatus(EVehicleStatus Status, const FLinearColor& NormalColor)
	{
		switch (Status)
		{
		case EVehicleStatus::Warning: return Warning();
		case EVehicleStatus::Critical: return Critical();
		default: return NormalColor;
		}
	}
}

namespace TwinHmiBrushes
{
	FSlateBrush MakeRoundedBox(const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth, float CornerRadius)
	{
		FSlateBrush RoundedBoxBrush;
		RoundedBoxBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
		RoundedBoxBrush.TintColor = FSlateColor(FillColor);
		RoundedBoxBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius),
			FSlateColor(OutlineColor), OutlineWidth);
		RoundedBoxBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return RoundedBoxBrush;
	}
}

namespace TwinHmiFormat
{
	FText FormatFixed(double Value, int32 DecimalCount)
	{
		FNumberFormattingOptions FixedDecimalOptions;
		FixedDecimalOptions.SetUseGrouping(false);
		FixedDecimalOptions.SetMinimumFractionalDigits(DecimalCount);
		FixedDecimalOptions.SetMaximumFractionalDigits(DecimalCount);
		return FText::AsNumber(Value, &FixedDecimalOptions);
	}

	FText StatusText(EVehicleStatus Status)
	{
		switch (Status)
		{
		case EVehicleStatus::Warning: return NSLOCTEXT("TwinHmi", "StatusWarning", "Warning");
		case EVehicleStatus::Critical: return NSLOCTEXT("TwinHmi", "StatusCritical", "Critical");
		default: return NSLOCTEXT("TwinHmi", "StatusNormal", "Normal");
		}
	}

	EVehicleStatus SingleTyreStatus(float PressureKpa)
	{
		if (PressureKpa < 140.f)
		{
			return EVehicleStatus::Critical;
		}
		return PressureKpa < 180.f || PressureKpa > 300.f ? EVehicleStatus::Warning : EVehicleStatus::Normal;
	}

	FText GearText(int32 Gear)
	{
		if (Gear < 0)
		{
			return NSLOCTEXT("TwinHmi", "GearReverse", "R");
		}
		return Gear == 0 ? NSLOCTEXT("TwinHmi", "GearNeutral", "N") : FText::AsNumber(Gear);
	}
}