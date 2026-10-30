#include "UI/TwinCarCalloutLayerWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Dashboard/TwinOpsPlayerController.h"
#include "Telemetry/VehicleStatusEvaluator.h"
#include "UI/TwinCarCalloutWidget.h"
#include "UI/TwinHmiStyle.h"
#include "UI/TwinPartRingWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogTwinCallouts, Log, All);

namespace CalloutLayerTiming
{
	static constexpr float FadeSeconds = 0.35f;
}

namespace CalloutLayerRings
{
	struct FRingTarget
	{
		const TCHAR* WidgetName;
		ETwinPartRingSignal RingSignal;
		const TCHAR* AnchorTag;
	};

	// The ring copies of the build page (E10), set from here so nothing has to be set on each copy in the WBP.
	static const FRingTarget RingTargets[] = {
		{ TEXT("RingHood"), ETwinPartRingSignal::Engine, TEXT("CalloutAnchor.Engine") },
		{ TEXT("RingTyreFL"), ETwinPartRingSignal::TyreFrontLeft, TEXT("Wheel.FL") },
		{ TEXT("RingTyreFR"), ETwinPartRingSignal::TyreFrontRight, TEXT("Wheel.FR") },
		{ TEXT("RingTyreRL"), ETwinPartRingSignal::TyreRearLeft, TEXT("Wheel.RL") },
		{ TEXT("RingTyreRR"), ETwinPartRingSignal::TyreRearRight, TEXT("Wheel.RR") },
	};

	static const FRingTarget* FindRingTarget(const UWidget& RingWidget)
	{
		for (const FRingTarget& RingTarget : RingTargets)
		{
			if (RingWidget.GetFName() == FName(RingTarget.WidgetName))
			{
				return &RingTarget;
			}
		}
		return nullptr;
	}
}

void UTwinCarCalloutLayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsDesignTime() || !CalloutCanvas)
	{
		return;
	}

	CarCallouts.Reset();
	PartRings.Reset();
	for (UWidget* CanvasChild : CalloutCanvas->GetAllChildren())
	{
		UCanvasPanelSlot* ChildSlot = Cast<UCanvasPanelSlot>(CanvasChild->Slot);
		if (UTwinCarCalloutWidget* CarCallout = Cast<UTwinCarCalloutWidget>(CanvasChild))
		{
			CarCallouts.Add(CarCallout);
			// The stem's bottom end is the point that sits on the anchor.
			ChildSlot->SetAlignment(FVector2D(0.5f, 1.f));
			ChildSlot->SetAutoSize(true);
			CarCallout->SetRenderOpacity(0.f);
		}
		else if (UTwinPartRingWidget* PartRing = Cast<UTwinPartRingWidget>(CanvasChild))
		{
			PartRings.Add(PartRing);
			if (const CalloutLayerRings::FRingTarget* RingTarget = CalloutLayerRings::FindRingTarget(*PartRing))
			{
				PartRing->SetRingTarget(RingTarget->RingSignal, FName(RingTarget->AnchorTag));
			}
			ChildSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ChildSlot->SetAutoSize(true);
			UE_LOG(LogTwinCallouts, Log, TEXT("Part ring %s: signal %s, anchor %s."), *PartRing->GetName(),
				*UEnum::GetValueAsString(PartRing->GetRingSignal()), *PartRing->GetAnchorTag().ToString());
		}
	}
	CalloutOpacities.Init(0.f, CarCallouts.Num());

	SubscribeToViewModel(DashboardViewModel, this, &UTwinCarCalloutLayerWidget::HandleDashboardFieldsChanged);
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinCarCalloutLayerWidget::HandleTelemetryFieldsChanged);
}

void UTwinCarCalloutLayerWidget::HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (DashboardViewModel && ChangedFieldMask.HasField(EDashboardViewModelField::ActiveView))
	{
		ActiveView = DashboardViewModel->GetActiveView();
	}
}

void UTwinCarCalloutLayerWidget::HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	RefreshCalloutValues();
}

void UTwinCarCalloutLayerWidget::RefreshCalloutValues()
{
	if (!TelemetryViewModel)
	{
		return;
	}
	const UVehicleTelemetryViewModel& Telemetry = *TelemetryViewModel;
	const FText KpaUnit = NSLOCTEXT("TwinHmi", "UnitKpa", "kPa");

	auto ShowTyre = [&KpaUnit](UTwinCarCalloutWidget& CarCallout, float PressureKpa)
	{
		const EVehicleStatus TyreStatus = TwinHmiFormat::SingleTyreStatus(PressureKpa);
		CarCallout.ShowCalloutValue(TwinHmiFormat::FormatFixed(PressureKpa, 0), KpaUnit,
			TwinHmiColors::ForStatus(TyreStatus, TwinHmiColors::TextPrimary()), TwinHmiColors::ForStatus(TyreStatus, TwinHmiColors::PanelEdgeHighlight()));
	};
	auto ShowOpening = [](UTwinCarCalloutWidget& CarCallout, bool bIsOpen)
	{
		CarCallout.ShowCalloutValue(bIsOpen ? NSLOCTEXT("TwinHmi", "CalloutOpen", "Open") : NSLOCTEXT("TwinHmi", "CalloutClosed", "Closed"), FText::GetEmpty(),
			bIsOpen ? TwinHmiColors::Warning() : TwinHmiColors::TextPrimary(), TwinHmiColors::PanelEdgeHighlight());
	};

	for (UTwinCarCalloutWidget* CarCallout : CarCallouts)
	{
		switch (CarCallout->GetCalloutValue())
		{
		case ETwinCalloutValue::CoolantTemperature:
		{
			const EVehicleStatus CoolantStatus = Telemetry.GetCoolantTemperatureStatus();
			CarCallout->ShowCalloutValue(TwinHmiFormat::FormatFixed(Telemetry.GetCoolantTempC(), 1), FText::FromString(TEXT("°C")),
				TwinHmiColors::ForStatus(CoolantStatus, TwinHmiColors::TextPrimary()), TwinHmiColors::ForStatus(CoolantStatus, TwinHmiColors::PanelEdgeHighlight()));
			break;
		}
		case ETwinCalloutValue::EngineRpm:
		{
			const EVehicleStatus RpmStatus = Telemetry.GetEngineRpmStatus();
			CarCallout->ShowCalloutValue(TwinHmiFormat::FormatFixed(FMath::RoundToFloat(Telemetry.GetEngineRpm() / 10.f) * 10.f, 0),
				NSLOCTEXT("TwinHmi", "UnitRpm", "rpm"), TwinHmiColors::ForStatus(RpmStatus, TwinHmiColors::TextPrimary()),
				TwinHmiColors::ForStatus(RpmStatus, TwinHmiColors::PanelEdgeHighlight()));
			break;
		}
		case ETwinCalloutValue::TyreFrontLeft: ShowTyre(*CarCallout, Telemetry.GetTyrePressureFrontLeftKpa()); break;
		case ETwinCalloutValue::TyreFrontRight: ShowTyre(*CarCallout, Telemetry.GetTyrePressureFrontRightKpa()); break;
		case ETwinCalloutValue::TyreRearLeft: ShowTyre(*CarCallout, Telemetry.GetTyrePressureRearLeftKpa()); break;
		case ETwinCalloutValue::TyreRearRight: ShowTyre(*CarCallout, Telemetry.GetTyrePressureRearRightKpa()); break;
		case ETwinCalloutValue::DoorFrontLeft: ShowOpening(*CarCallout, Telemetry.IsOpen(EVehicleOpening::DoorFL)); break;
		case ETwinCalloutValue::Hood: ShowOpening(*CarCallout, Telemetry.IsOpen(EVehicleOpening::Hood)); break;
		}
	}

	// Rings: the hood for coolant or rpm; a wheel only while the Tyres status is off, then that wheel's own status.
	const bool bAnyTyreIsOff = Telemetry.GetTyrePressureStatus() != EVehicleStatus::Normal;
	auto TyreRingStatus = [bAnyTyreIsOff](float PressureKpa)
	{
		return bAnyTyreIsOff ? TwinHmiFormat::SingleTyreStatus(PressureKpa) : EVehicleStatus::Normal;
	};
	for (UTwinPartRingWidget* PartRing : PartRings)
	{
		switch (PartRing->GetRingSignal())
		{
		case ETwinPartRingSignal::Engine:
			PartRing->ShowPartStatus(FVehicleStatusEvaluator::WorseStatus(Telemetry.GetCoolantTemperatureStatus(), Telemetry.GetEngineRpmStatus()));
			break;
		case ETwinPartRingSignal::TyreFrontLeft: PartRing->ShowPartStatus(TyreRingStatus(Telemetry.GetTyrePressureFrontLeftKpa())); break;
		case ETwinPartRingSignal::TyreFrontRight: PartRing->ShowPartStatus(TyreRingStatus(Telemetry.GetTyrePressureFrontRightKpa())); break;
		case ETwinPartRingSignal::TyreRearLeft: PartRing->ShowPartStatus(TyreRingStatus(Telemetry.GetTyrePressureRearLeftKpa())); break;
		case ETwinPartRingSignal::TyreRearRight: PartRing->ShowPartStatus(TyreRingStatus(Telemetry.GetTyrePressureRearRightKpa())); break;
		}
	}
}

void UTwinCarCalloutLayerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ATwinOpsPlayerController* TwinOpsPlayerController = Cast<ATwinOpsPlayerController>(GetOwningPlayer());
	if (!TwinOpsPlayerController)
	{
		return;
	}
	const bool bHasTelemetry = TelemetryViewModel && TelemetryViewModel->HasReceivedTelemetry();
	const float FadeStep = InDeltaTime / CalloutLayerTiming::FadeSeconds;

	// Positions follow the camera every frame, also during the 0.9 s blend between views.
	for (int32 CalloutIndex = 0; CalloutIndex < CarCallouts.Num(); ++CalloutIndex)
	{
		UTwinCarCalloutWidget* CarCallout = CarCallouts[CalloutIndex];
		FVector2D CalloutScreenPosition;
		const bool bIsOnScreen = TwinOpsPlayerController->ProjectCarAnchorToWidgetPosition(CarCallout->GetAnchorTag(),
			CarCallout->GetAnchorOffsetInCarSpace(), CalloutScreenPosition);
		if (bIsOnScreen)
		{
			Cast<UCanvasPanelSlot>(CarCallout->Slot)->SetPosition(CalloutScreenPosition);
		}
		const float TargetOpacity = bHasTelemetry && bIsOnScreen && CarCallout->GetShownInView() == ActiveView ? 1.f : 0.f;
		CalloutOpacities[CalloutIndex] = FMath::FInterpConstantTo(CalloutOpacities[CalloutIndex], TargetOpacity, 1.f, FadeStep);
		CarCallout->SetRenderOpacity(CalloutOpacities[CalloutIndex]);
		// Collapse once faded out: the box's rounded outline ignores render opacity and would stay as an empty frame.
		const ESlateVisibility CalloutVisibility = CalloutOpacities[CalloutIndex] > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (CarCallout->GetVisibility() != CalloutVisibility)
		{
			CarCallout->SetVisibility(CalloutVisibility);
		}
	}
	for (UTwinPartRingWidget* PartRing : PartRings)
	{
		FVector2D RingScreenPosition;
		const bool bIsOnScreen = TwinOpsPlayerController->ProjectCarAnchorToWidgetPosition(PartRing->GetAnchorTag(), FVector::ZeroVector,
			RingScreenPosition);
		if (bIsOnScreen)
		{
			Cast<UCanvasPanelSlot>(PartRing->Slot)->SetPosition(RingScreenPosition);
		}
		if (!bIsOnScreen && !UnplacedPartRings.Contains(PartRing))
		{
			UnplacedPartRings.Add(PartRing);
			UE_LOG(LogTwinCallouts, Warning, TEXT("Part ring %s can't be placed: no component on the car carries %s (or it is off screen)."),
				*PartRing->GetName(), *PartRing->GetAnchorTag().ToString());
		}
		// No car or no anchor: hide it rather than leave it at its designer position (top left).
		PartRing->SetRenderOpacity(bIsOnScreen ? 1.f : 0.f);
	}
}