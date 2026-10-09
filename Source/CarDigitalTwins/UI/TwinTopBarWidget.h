// The bar across the top: vehicle and trip clock, the tell-tale naming the worst signal (the one-glance answer), the drive mode chip
// and the link chip. Reads the telemetry and connection ViewModels; C++ draws every state-dependent colour and edge.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/ConnectionViewModel.h"
#include "MVVM/VehicleTelemetryViewModel.h"
#include "UI/TwinViewModelWidget.h"
#include "TwinTopBarWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class UTwinTopBarWidget : public UTwinViewModelWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TripClockText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TellTaleBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TellTaleIconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TellTaleStateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TellTaleReasonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> DriveModeChipBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DriveModeLabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DriveModeValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> LinkChipBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LinkDotImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LinkStateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LinkDetailText;

	// T_Icon_StatusOk: the circled tick shown while everything is Normal.
	UPROPERTY(EditAnywhere, Category = "Top Bar")
	TObjectPtr<UTexture2D> StatusOkIcon;

	// T_Icon_StatusAlert: the triangle shown for Warning and Critical, tinted amber or red.
	UPROPERTY(EditAnywhere, Category = "Top Bar")
	TObjectPtr<UTexture2D> StatusAlertIcon;

	UPROPERTY(EditAnywhere, Category = "Top Bar")
	FText VehicleModelText = NSLOCTEXT("TwinHmi", "VehicleModel", "2010 Wrangler");

private:
	void HandleViewModelFieldsChanged(FViewModelFieldMask ChangedFieldMask);
	void RefreshTopBar();
	void ShowTellTale(EVehicleStatus OverallStatus, const FText& StateWord, const FText& Reason, bool bShowIcon);

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryViewModel> TelemetryViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UConnectionViewModel> ConnectionViewModel;
};