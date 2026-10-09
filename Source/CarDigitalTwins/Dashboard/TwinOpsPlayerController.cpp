#include "Dashboard/TwinOpsPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Dashboard/TwinDashboardSettings.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "MVVM/ViewModelSubsystem.h"
#include "Vehicle/VehicleTwinActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTwinDashboard, Log, All);

ATwinOpsPlayerController::ATwinOpsPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
}

void ATwinOpsPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	// Mouse for the dashboard, keys 1-4 still reach PlayerTick; the cursor stays visible while a button is held.
	FInputModeGameAndUI DashboardInputMode;
	DashboardInputMode.SetHideCursorDuringCapture(false);
	DashboardInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(DashboardInputMode);

	for (TActorIterator<AVehicleTwinActor> CarActorIterator(GetWorld()); CarActorIterator; ++CarActorIterator)
	{
		TwinCarActor = *CarActorIterator;
		break;
	}
	if (!TwinCarActor.IsValid())
	{
		UE_LOG(LogTwinDashboard, Warning, TEXT("No BP_VehicleTwin in this level: the callouts on the car stay hidden."));
	}

	const UTwinDashboardSettings* DashboardSettings = GetDefault<UTwinDashboardSettings>();
	if (UClass* HudWidgetClass = DashboardSettings->HudWidgetClass.LoadSynchronous())
	{
		HudWidget = CreateWidget<UUserWidget>(this, HudWidgetClass);
		HudWidget->AddToViewport();
	}
	else
	{
		UE_LOG(LogTwinDashboard, Warning, TEXT("No dashboard: set Project Settings > Game > Twin Dashboard > HUD Widget Class to WBP_TwinOpsHUD."));
	}

	// The first call comes right away with the current view: cut to its camera, then blend on every change after that.
	if (UViewModelSubsystem* ViewModelSubsystem = UViewModelSubsystem::FindForWorldContext(this))
	{
		DashboardViewModelSubscription = ViewModelSubsystem->GetViewModel<UDashboardViewModel>()->Subscribe(this,
			&ATwinOpsPlayerController::HandleDashboardFieldsChanged);
	}
}

void ATwinOpsPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DashboardViewModelSubscription.Reset();
	Super::EndPlay(EndPlayReason);
}

void ATwinOpsPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);

	// Keys 1-4 pick a view, like the tabs. Read here rather than through input actions: four fixed keys, no rebinding needed.
	const FKey ViewKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	for (int32 ViewIndex = 0; ViewIndex < UE_ARRAY_COUNT(ViewKeys); ++ViewIndex)
	{
		if (WasInputKeyJustPressed(ViewKeys[ViewIndex]))
		{
			if (UViewModelSubsystem* ViewModelSubsystem = UViewModelSubsystem::FindForWorldContext(this))
			{
				ViewModelSubsystem->GetViewModel<UDashboardViewModel>()->SelectView(static_cast<ETwinDashboardView>(ViewIndex));
			}
		}
	}
}

bool ATwinOpsPlayerController::ProjectCarAnchorToWidgetPosition(FName CalloutAnchorTag, const FVector& AnchorOffsetInCarSpace,
	FVector2D& OutWidgetPosition)
{
	const AVehicleTwinActor* CarActor = TwinCarActor.Get();
	FVector AnchorWorldLocation;
	if (!CarActor || !CarActor->FindCalloutAnchorWorldLocation(CalloutAnchorTag, AnchorWorldLocation))
	{
		return false;
	}
	AnchorWorldLocation += CarActor->GetActorTransform().TransformVectorNoScale(AnchorOffsetInCarSpace);
	// Viewport-relative: the HUD fills the viewport, so this is the HUD canvas's own space, DPI scale included.
	return UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(this, AnchorWorldLocation, OutWidgetPosition, false);
}

void ATwinOpsPlayerController::HandleDashboardFieldsChanged(FViewModelFieldMask ChangedFieldMask)
{
	if (!ChangedFieldMask.HasField(EDashboardViewModelField::ActiveView))
	{
		return;
	}
	if (UViewModelSubsystem* ViewModelSubsystem = UViewModelSubsystem::FindForWorldContext(this))
	{
		const UDashboardViewModel* DashboardViewModel = ViewModelSubsystem->GetViewModel<UDashboardViewModel>();
		ShowViewCamera(DashboardViewModel->GetActiveView(), !bHasShownFirstView);
		bHasShownFirstView = true;
	}
}

void ATwinOpsPlayerController::ShowViewCamera(ETwinDashboardView View, bool bCutWithoutBlend)
{
	AActor* ViewCameraActor = FindViewCameraActor(View);
	if (!ViewCameraActor)
	{
		UE_LOG(LogTwinDashboard, Warning, TEXT("No camera for the %s view: tag a CameraActor in the level with %s%s."), GetTwinDashboardViewName(View),
			*GetDefault<UTwinDashboardSettings>()->CameraActorTagPrefix, GetTwinDashboardViewName(View));
		return;
	}

	// The camera manager moves the view from where it is now (mid-blend included) to the new camera's position, rotation and field
	// of view over the blend time. VTBlend_EaseInOut starts and ends slowly; the exponent sets how gently.
	const UTwinDashboardSettings* DashboardSettings = GetDefault<UTwinDashboardSettings>();
	const float BlendSeconds = bCutWithoutBlend ? 0.f : DashboardSettings->CameraBlendSeconds;
	SetViewTargetWithBlend(ViewCameraActor, BlendSeconds, EViewTargetBlendFunction::VTBlend_EaseInOut, DashboardSettings->CameraBlendExponent);
}

AActor* ATwinOpsPlayerController::FindViewCameraActor(ETwinDashboardView View) const
{
	const FName ViewCameraTag(*(GetDefault<UTwinDashboardSettings>()->CameraActorTagPrefix + GetTwinDashboardViewName(View)));
	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(this, ViewCameraTag, TaggedActors);
	return TaggedActors.IsEmpty() ? nullptr : TaggedActors[0];
}