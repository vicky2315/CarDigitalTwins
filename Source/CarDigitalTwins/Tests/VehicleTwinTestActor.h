// A stand-in for BP_VehicleTwin in the automation tests (Tests/VehicleTwinActorTests.cpp), so they run without the Jeep assets.
// Same tags and layout as the real car, built from engine shapes: car faces +X, right is +Y, four hubs tagged Wheel.FL ... Wheel.RR,
// each with a cylinder tyre tagged Tyre that is offset from the hub (the hub isn't the wheel centre, like the CAD wheels), a Paint
// and a Trim mesh, a Scene Component tagged CalloutAnchor.Hood and one child actor. The test assigns the meshes and materials
// before it finishes spawning, so this class loads no assets itself. Not for levels.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/VehicleTwinActor.h"
#include "VehicleTwinTestActor.generated.h"

class UChildActorComponent;
class UStaticMeshComponent;

UCLASS(NotPlaceable, NotBlueprintable, Transient, HideDropdown)
class AVehicleTwinTestActor : public AVehicleTwinActor
{
	GENERATED_BODY()

public:
	AVehicleTwinTestActor();

	// Tyre radius the cylinder is scaled to, cm; the real Jeep tyres measure 43.5.
	static constexpr float TyreRadiusCm = 43.5f;

	// Where each tyre sits in its hub's space: forward and down, so spinning around the hub origin instead of the tyre centre shows.
	static const FVector TyreOffsetInHubSpace;

	// Same order as the wheel tags: FL, FR, RL, RR.
	UPROPERTY(VisibleAnywhere, Category = "Test")
	TArray<TObjectPtr<USceneComponent>> Hubs;

	UPROPERTY(VisibleAnywhere, Category = "Test")
	TArray<TObjectPtr<UStaticMeshComponent>> Tyres;

	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<UStaticMeshComponent> BodyPaintMesh;

	// In the Trim paint group, which this class gives a blend scale of 0.5.
	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<UStaticMeshComponent> TrimMesh;

	// No tag by default; V7 tags it Paint with a material that has no StatusColor, like glass on a painted mesh.
	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<UStaticMeshComponent> GlassMesh;

	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<USceneComponent> HoodAnchor;

	// Spawns an AStaticMeshActor, standing in for the Harvest Components child actors whose meshes also take the outline.
	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<UChildActorComponent> HarvestedPart;

	// Lets the test set the outline material without a Blueprint.
	void SetStatusOutlineMaterialForTest(UMaterialInterface* OutlineMaterial) { StatusOutlineMaterial = OutlineMaterial; }
};
