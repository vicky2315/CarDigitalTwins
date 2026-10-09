// A full-screen canvas holding the callouts and rings that sit on the car. Every frame it asks the player controller where each
// anchor is on screen and moves the widget there; callouts of the open view fade in over 0.35 s, the others fade out. Values come
// from the telemetry ViewModel. Place the WBP_CarCallout and WBP_PartRing copies anywhere in CalloutCanvas: their position is set here.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/DashboardViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinCarCalloutLayerWidget.generated.h"

class UCanvasPanel;
class UTwinCarCalloutWidget;
class UTwinPartRingWidget;

UCLASS()
class UTwinCarCalloutLayerWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CalloutCanvas;

private:
	void HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void RefreshCalloutValues();

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UDashboardViewModel> DashboardViewModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTwinCarCalloutWidget>> CarCallouts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTwinPartRingWidget>> PartRings;

	// Parallel to CarCallouts: 0 = hidden, 1 = shown; eased towards the target each frame.
	TArray<float> CalloutOpacities;

	ETwinDashboardView ActiveView = ETwinDashboardView::Overview;
};