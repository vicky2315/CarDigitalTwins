// The Critical alarm banner: shown while the coolant is Critical, its red frame flashing once a second until acknowledged (HMI rule 5).
// Show opens the Powertrain view; the camera never moves by itself (decision D3). Hidden when there is no alarm.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/VehicleEventLogViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinAlarmBannerWidget.generated.h"

class UBorder;
class UTextBlock;
class UTwinHmiButtonWidget;

UCLASS()
class UTwinAlarmBannerWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> AlarmFrameBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AlarmTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AlarmDetailText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinHmiButtonWidget> ShowButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTwinHmiButtonWidget> AcknowledgeButton;

private:
	void HandleViewModelFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void RefreshAlarm();
	void SetFrameEdge(const FLinearColor& EdgeColor);
	void HandleShowClicked();
	void HandleAcknowledgeClicked();

	UPROPERTY(Transient)
	TObjectPtr<UVehicleEventLogViewModel> EventLogViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;

	float FlashSeconds = 0.f;
	bool bIsFlashing = false;
	bool bIsFrameDim = false;
};