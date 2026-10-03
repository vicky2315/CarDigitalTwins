// Base actor for the vehicle twin. BP_VehicleTwin supplies the meshes; this class finds the parts by component tag
// (docs/SPEC.md §1) and moves them.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VehicleTwinActor.generated.h"

class UMaterialInstanceDynamic;

// One road wheel, resolved from a hub tagged Wheel.FL / Wheel.FR / Wheel.RL / Wheel.RR.
USTRUCT()
struct FVehicleTwinWheel
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USceneComponent> Hub = nullptr;

	// Hub rotation relative to its parent once aligned with the actor axes. Spin and steer are applied on top.
	FQuat BaseRelativeRotation = FQuat::Identity;

	// Measured from the tyre mesh, so there is one source of truth for wheel speed (SPEC.md §2.1).
	float RadiusCm = 0.f;

	// Axle direction in hub space (the tyre's thinnest bounds axis), so spin doesn't depend on which way the car faces.
	FVector SpinAxis = FVector::RightVector;

	float SpinDeg = 0.f;

	bool bFront = false;
};

UCLASS()
class CARDIGITALTWINS_API AVehicleTwinActor : public AActor
{
	GENERATED_BODY()

public:
	AVehicleTwinActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	// Rolls the road wheels for SpeedKmh over DeltaSeconds and turns the front wheels to SteerDeg (+ = right).
	UFUNCTION(BlueprintCallable, Category = "Vehicle Twin")
	void UpdateWheels(float SpeedKmh, float SteerDeg, float DeltaSeconds);

	// Sets the StatusColor vector parameter on every material of the components tagged Paint.
	UFUNCTION(BlueprintCallable, Category = "Vehicle Twin")
	void SetStatusColor(FLinearColor Color);

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

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle Twin")
	FName StatusColorParameter = TEXT("StatusColor");

private:
	// Finds the tagged hubs, moves each onto its tyre's centre and aligns it with the actor axes. Safe to call again.
	void SetUpWheels(bool bLogProblems);

	void CreatePaintMaterials();

	void ApplyWheelRotation(const FVehicleTwinWheel& Wheel, float SteerDeg) const;

	UPROPERTY(Transient)
	TArray<FVehicleTwinWheel> Wheels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PaintMaterials;
};
