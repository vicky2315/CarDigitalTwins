// The dashboard's colours, brushes and number formats in one place (docs/SPEC.md §8, "Colours and type" on the Twin Ops Dashboard page).
// Colours are written as the sRGB hex values of the mock and converted to linear here, the same conversion the UMG colour picker's
// "Hex sRGB" field does. Rules: red and amber only mean Critical and Warning; cyan only means selected or pressable.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Telemetry/VehicleTelemetry.h"

namespace TwinHmiColors
{
	// 0xRRGGBB as in the mock, plus opacity 0..1.
	FLinearColor FromSrgbHex(uint32 RedGreenBlueHex, float Opacity = 1.f);

	FLinearColor PanelFill();          // 0E1419 A 0.80
	FLinearColor PanelEdge();          // 2A3741
	FLinearColor PanelEdgeHighlight(); // 3B4B57
	FLinearColor TextPrimary();        // E7EDF1: normal values
	FLinearColor TextSecondary();      // 93A2AD: labels, units
	FLinearColor TextTertiary();       // 5F6E79: "Closed", Normal status, tick labels
	FLinearColor Accent();             // 56B8DE: selected, pressable
	FLinearColor AccentSoft();         // 56B8DE A 0.16
	FLinearColor Warning();            // F0A73A
	FLinearColor WarningSoft();        // F0A73A A 0.16
	FLinearColor Critical();           // F2564F
	FLinearColor CriticalSoft();       // F2564F A 0.18
	FLinearColor LinkLive();           // 47C389: only the live link dot
	FLinearColor BarTrack();           // 24313A
	FLinearColor BarWarningZone();     // 4B3B1D
	FLinearColor BarCriticalZone();    // 4F2423
	FLinearColor SceneBackground();    // 0B1014: the marker's outline, so it stands off the bar
	FLinearColor ButtonFill();         // 18222A
	FLinearColor ButtonFillHovered();  // 202C35
	FLinearColor ButtonFillPressed();  // 141C22
	FLinearColor HoverTint();          // FFFFFF A 0.04: rows and tabs under the mouse
	FLinearColor AlarmFill();          // 2A1414 A 0.95
	FLinearColor AlarmEdgeDim();       // 5A2A28: the alarm frame's "off" phase while it flashes
	FLinearColor Transparent();

	// Warning or Critical colour; NormalColor for Normal.
	FLinearColor ForStatus(EVehicleStatus Status, const FLinearColor& NormalColor);
}

namespace TwinHmiBrushes
{
	// A filled rounded box with an outline, like the mock's borders. OutlineWidth 0 = no outline.
	FSlateBrush MakeRoundedBox(const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth, float CornerRadius);
}

namespace TwinHmiFormat
{
	// Fixed decimals, no thousands separator: "52", "115.0", "12451.3".
	FText FormatFixed(double Value, int32 DecimalCount);

	// "Normal", "Warning", "Critical". Text blocks show it in capitals (Transform Policy: To Upper).
	FText StatusText(EVehicleStatus Status);

	// One tyre's own status, without hysteresis, for its card and callout: Critical below 140 kPa, Warning below 180 or above 300
	// (SPEC.md §3). The Tyres status in the telemetry ViewModel is the hysteresis-aware worst of the four.
	EVehicleStatus SingleTyreStatus(float PressureKpa);

	// "R", "N", "1".."6".
	FText GearText(int32 Gear);
}
