#include "UI/TwinTyresPanelWidget.h"

#include "UI/TwinTyreCardWidget.h"

void UTwinTyresPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SubscribeToViewModel(TelemetryViewModel, this, &UTwinTyresPanelWidget::HandleTelemetryFieldsChanged);
}

void UTwinTyresPanelWidget::HandleTelemetryFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!TelemetryViewModel)
	{
		return;
	}
	using TelemetryField = EVehicleTelemetryViewModelField;
	// Each card redraws only when its own tyre's bit is set.
	if (ChangedFieldMask.HasField(TelemetryField::TyrePressureFrontLeftKpa) && TyreCardFrontLeft)
	{
		TyreCardFrontLeft->ShowPressure(TelemetryViewModel->GetTyrePressureFrontLeftKpa());
	}
	if (ChangedFieldMask.HasField(TelemetryField::TyrePressureFrontRightKpa) && TyreCardFrontRight)
	{
		TyreCardFrontRight->ShowPressure(TelemetryViewModel->GetTyrePressureFrontRightKpa());
	}
	if (ChangedFieldMask.HasField(TelemetryField::TyrePressureRearLeftKpa) && TyreCardRearLeft)
	{
		TyreCardRearLeft->ShowPressure(TelemetryViewModel->GetTyrePressureRearLeftKpa());
	}
	if (ChangedFieldMask.HasField(TelemetryField::TyrePressureRearRightKpa) && TyreCardRearRight)
	{
		TyreCardRearRight->ShowPressure(TelemetryViewModel->GetTyrePressureRearRightKpa());
	}
}