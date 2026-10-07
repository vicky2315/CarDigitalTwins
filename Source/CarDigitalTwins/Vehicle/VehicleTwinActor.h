// Base actor for the vehicle twin. BP_VehicleTwin supplies the meshes; this class finds the parts by component tag
// (docs/SPEC.md §1) and moves them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Telemetry/VehicleTelemetry.h"
#include "VehicleTwinActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;
class UTelemetrySubsystem;

// One road wheel, resolved from a hub tagged Wheel.FL / Wheel.FR / Wheel.RL / Wheel.RR.
USTRUCT()
struct FVehicleTwinWheel
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USceneComponent> Hub = nullptr;

	// Hub transform relative to its parent as authored in BP_VehicleTwin. Spin and steer are applied on top; the hub is never moved,
	// so the meshes under it keep their authored transforms.
	FTransform BaseRelativeTransform = FTransform::Identity;

	// Tyre centre in hub space: the pivot that spin and steer rotate around.
	FVector WheelCentreInHubSpace = FVector::ZeroVector;

	// Measured from the tyre mesh, so there is one source of truth for wheel speed (SPEC.md §2.1).
	float RadiusCm = 0.f;

	// Axle direction in hub space, from the left wheel's centre towards its right partner's, so spin doesn't depend on which way the car faces.
	FVector SpinAxis = FVector::RightVector;

	// The actor's up direction in hub space, for steering.
	FVector SteerAxisInHubSpace = FVector::UpVector;

	float SpinDeg = 0.f;

	bool bFront = false;
};

// A set of components that take the status colour, found by component tag (e.g. Paint = body, Trim = black plastic parts).
USTRUCT()
struct FVehicleTwinPaintGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin")
	FName ComponentTag;

	// Multiplies the status blend for this group: 1 = full status colour, 0.5 = half, 0 = never changes.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin", meta = (ClampMin = "0", ClampMax = "1"))
	float StatusBlendScale = 1.f;
};

UCLASS()
class CARDIGITALTWINS_API AVehicleTwinActor : public AActor
{
	GENERATED_BODY()

public:
	AVehicleTwinActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	// Rolls the road wheels for SpeedKmh over DeltaSeconds and turns the front wheels to SteerDeg (+ = right).
	UFUNCTION(BlueprintCallable, Category = "Vehicle Twin")
	void UpdateWheels(float SpeedKmh, float SteerDeg, float DeltaSeconds);

	// Tints every paint group (PaintGroups) towards NewStatusColor. NewStatusBlend 0 = original colours, 1 = fully the status colour,
	// scaled per group by its StatusBlendScale.
	UFUNCTION(BlueprintCallable, Category = "Vehicle Twin")
	void SetStatusColor(FLinearColor NewStatusColor, float NewStatusBlend = 1.f);

	// Shows a glowing, pulsing outline around the whole car for Warning (amber) and Critical (red); Normal removes it. Leaves the
	// paint alone, so it reads on the red body (SPEC.md §1). Cheap to call every frame: the meshes are only touched on a change.
	UFUNCTION(BlueprintCallable, Category = "Vehicle Twin")
	void SetStatusOutline(EVehicleStatus NewVehicleStatus);

	UFUNCTION(BlueprintPure, Category = "Vehicle Twin")
	int32 GetNumWheels() const { return Wheels.Num(); }

protected:
	// Spins and steers the wheels in the level viewport without telemetry, to check axes and pivots.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview")
	bool bPreviewInEditor = false;

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview", meta = (EditCondition = "bPreviewInEditor", Units = "km/h"))
	float PreviewSpeedKmh = 10.f;

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview", meta = (EditCondition = "bPreviewInEditor", ClampMin = "-40", ClampMax = "40"))
	float PreviewSteerDeg = 0.f;

	// Shows a status colour on the paint groups in the level viewport without telemetry, to check the tags and materials.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview")
	bool bPreviewStatusColor = false;

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview", meta = (EditCondition = "bPreviewStatusColor"))
	FLinearColor PreviewStatusColor = FLinearColor(1.f, 0.5f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview", meta = (EditCondition = "bPreviewStatusColor", ClampMin = "0", ClampMax = "1"))
	float PreviewStatusBlend = 1.f;

	// Shows the status outline in the level viewport without telemetry, to tune the outline material and colours.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview")
	bool bPreviewStatusOutline = false;

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Preview", meta = (EditCondition = "bPreviewStatusOutline"))
	EVehicleStatus PreviewVehicleStatus = EVehicleStatus::Warning;

	// In play, wheels and status outline follow UTelemetrySubsystem. Off = the car only moves through the preview settings.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Telemetry")
	bool bDriveFromTelemetry = true;

	// Post-process material that draws the outline around pixels with CustomStencil = StatusOutlineStencilValue
	// (PP_VehicleStatusOutline, SPEC.md §1). Needs Project Settings > Custom Depth-Stencil Pass = Enabled with Stencil.
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin|Status Outline")
	TObjectPtr<UMaterialInterface> StatusOutlineMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin|Status Outline", meta = (ClampMin = "1", ClampMax = "255"))
	int32 StatusOutlineStencilValue = 1;

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Status Outline")
	FLinearColor WarningOutlineColor = FLinearColor(1.f, 0.45f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Status Outline")
	FLinearColor CriticalOutlineColor = FLinearColor(1.f, 0.02f, 0.02f);

	// HDR multiplier on the outline colour at the top of the pulse. Above 1 the outline blooms.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Status Outline", meta = (ClampMin = "0"))
	float OutlineGlowIntensity = 6.f;

	// Pulses per second. Critical pulses faster so the two states differ in rhythm, not only colour.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Status Outline", meta = (ClampMin = "0", Units = "Hz"))
	float WarningOutlinePulseHz = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Status Outline", meta = (ClampMin = "0", Units = "Hz"))
	float CriticalOutlinePulseHz = 2.f;

	// How far the pulse dims the outline: 0 = steady, 1 = fades out completely at the bottom of each pulse.
	UPROPERTY(EditAnywhere, Category = "Vehicle Twin|Status Outline", meta = (ClampMin = "0", ClampMax = "1"))
	float OutlinePulseDepth = 0.6f;

	// Parameters of PP_VehicleStatusOutline.
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin|Status Outline")
	FName StatusOutlineColorParameter = TEXT("StatusOutlineColor");

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin|Status Outline")
	FName StatusOutlineGlowIntensityParameter = TEXT("StatusOutlineGlowIntensity");

	// Component tags that take the status colour, each with its own strength (SPEC.md §1).
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin")
	TArray<FVehicleTwinPaintGroup> PaintGroups;

	// Material parameters of M_CarPaint (SPEC.md §1).
	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin")
	FName StatusColorParameter = TEXT("StatusColor");

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin")
	FName StatusBlendParameter = TEXT("StatusBlend");

private:
	// Finds the tagged hubs, resets them to their authored pose and measures each wheel's centre, axle and radius. Safe to call again.
	void SetUpWheels(bool bLogProblems);

	// Largest distance of the tyre's vertices from the axle line (hub space). Falls back to the bounding sphere without CPU vertex access.
	static float MeasureTyreRadius(const UPrimitiveComponent* TyreComponent, const FTransform& HubWorldTransform, const FVector& WheelCentreInHubSpace,
		const FVector& AxleInHubSpace, bool& bOutFromVertices);

	// Creates a dynamic material instance for every material slot on the paint group meshes whose material has the StatusColor
	// parameter. Slots without it (glass on the same mesh) are left alone.
	void CreatePaintMaterials(bool bLogProblems);

	void ApplyWheelRotation(const FVehicleTwinWheel& Wheel, float SteerDeg) const;

	// Creates the unbound post-process component and the outline material instance on first use. Returns false without a material.
	bool EnsureStatusOutlinePostProcess(bool bLogProblems);

	// Every mesh of the car, including those inside the Harvest child actors: all of them write the outline stencil.
	void CollectStatusOutlineMeshComponents();

	// Sets the outline colour and the pulsing glow for this frame; called every tick while the status isn't Normal.
	void UpdateStatusOutlinePulse(float DeltaSeconds);

	// Records when each sample arrived, for interpolating between the subsystem's previous and latest sample.
	void HandleTelemetrySampleReceived(const FVehicleTelemetry& NewTelemetrySample);

	// Spins and steers the wheels from the telemetry blended between the previous and latest sample, and applies the status outline.
	void DriveFromTelemetry(const UTelemetrySubsystem& TelemetrySubsystem, float DeltaSeconds);

	UPROPERTY(Transient)
	TArray<FVehicleTwinWheel> Wheels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PaintMaterials;

	// Parallel to PaintMaterials: the StatusBlendScale of the group each material belongs to.
	UPROPERTY(Transient)
	TArray<float> PaintMaterialStatusBlendScales;

	// Created at runtime (and for the editor preview), never saved: the outline is a property of the car, not of the level.
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> StatusOutlinePostProcess;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StatusOutlineMaterialInstance;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> StatusOutlineMeshComponents;

	// The status the outline currently shows; the meshes are only updated when it changes.
	EVehicleStatus DisplayedOutlineStatus = EVehicleStatus::Normal;

	float StatusOutlinePulseSeconds = 0.f;

	TWeakObjectPtr<UTelemetrySubsystem> SubscribedTelemetrySubsystem;
	FDelegateHandle TelemetryUpdatedDelegateHandle;

	// World time the latest sample arrived, and the time between the last two arrivals (the interpolation span). Negative = none yet.
	double LatestSampleArrivalTimeS = -1.0;
	double SampleArrivalIntervalS = 0.1;
};
