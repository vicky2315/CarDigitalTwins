#include "Vehicle/VehicleTwinActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTwin, Log, All);

namespace VehicleTwinTags
{
	// Index order matters: the first two are the steered front wheels.
	static const FName Wheels[] = { TEXT("Wheel.FL"), TEXT("Wheel.FR"), TEXT("Wheel.RL"), TEXT("Wheel.RR") };
	static const FName Tyre = TEXT("Tyre");
	static const FName Paint = TEXT("Paint");
}

AVehicleTwinActor::AVehicleTwinActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AVehicleTwinActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs on every edit in the editor, so problems are only logged from BeginPlay.
	SetUpWheels(false);
}

void AVehicleTwinActor::BeginPlay()
{
	Super::BeginPlay();

	// Placed actors in a cooked build don't rerun construction, so resolve the wheels here as well.
	SetUpWheels(true);
	CreatePaintMaterials();
}

void AVehicleTwinActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPreviewInEditor)
	{
		UpdateWheels(PreviewSpeedKmh, PreviewSteerDeg, DeltaSeconds);

		// Debug: yellow dot = hub centre, red line = spin axis, blue = up, green circle = measured tyre radius.
		for (const FVehicleTwinWheel& Wheel : Wheels)
		{
			const FTransform HubTransform = Wheel.Hub->GetComponentTransform();
			const FVector Centre = HubTransform.GetLocation();
			const FVector Axle = HubTransform.TransformVectorNoScale(Wheel.SpinAxis);
			const FVector Up = GetActorUpVector();
			DrawDebugPoint(GetWorld(), Centre, 12.f, FColor::Yellow, false, -1.f, SDPG_Foreground);
			DrawDebugLine(GetWorld(), Centre - Axle * 60.f, Centre + Axle * 60.f, FColor::Red, false, -1.f, SDPG_Foreground, 1.5f);
			DrawDebugLine(GetWorld(), Centre, Centre + Up * 60.f, FColor::Blue, false, -1.f, SDPG_Foreground, 1.5f);
			DrawDebugCircle(GetWorld(), Centre, Wheel.RadiusCm, 32, FColor::Green, false, -1.f, SDPG_Foreground, 1.f,
				GetActorForwardVector(), GetActorUpVector(), false);
		}
	}
}

bool AVehicleTwinActor::ShouldTickIfViewportsOnly() const
{
	return bPreviewInEditor;
}

void AVehicleTwinActor::UpdateWheels(float SpeedKmh, float SteerDeg, float DeltaSeconds)
{
	const float SpeedCmPerS = SpeedKmh * (100000.f / 3600.f);

	for (FVehicleTwinWheel& Wheel : Wheels)
	{
		if (Wheel.RadiusCm > KINDA_SMALL_NUMBER)
		{
			const float DeltaDeg = FMath::RadiansToDegrees(SpeedCmPerS * DeltaSeconds / Wheel.RadiusCm);
			Wheel.SpinDeg = FMath::Fmod(Wheel.SpinDeg + DeltaDeg, 360.f);
		}
		ApplyWheelRotation(Wheel, SteerDeg);
	}
}

void AVehicleTwinActor::SetStatusColor(FLinearColor Color)
{
	if (PaintMaterials.IsEmpty())
	{
		CreatePaintMaterials();
	}

	for (UMaterialInstanceDynamic* Material : PaintMaterials)
	{
		Material->SetVectorParameterValue(StatusColorParameter, Color);
	}
}

void AVehicleTwinActor::SetUpWheels(bool bLogProblems)
{
	Wheels.Reset();

	TInlineComponentArray<USceneComponent*> SceneComponents(this);
	const FQuat ActorRotation = GetActorQuat();

	for (int32 TagIndex = 0; TagIndex < UE_ARRAY_COUNT(VehicleTwinTags::Wheels); ++TagIndex)
	{
		const FName WheelTag = VehicleTwinTags::Wheels[TagIndex];
		USceneComponent* const* HubPtr = SceneComponents.FindByPredicate(
			[WheelTag](const USceneComponent* Component) { return Component->ComponentHasTag(WheelTag); });
		if (!HubPtr)
		{
			if (bLogProblems)
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: no component tagged %s (SPEC.md §1)."), *GetName(), *WheelTag.ToString());
			}
			continue;
		}
		USceneComponent* Hub = *HubPtr;

		// Direct children are moved back after the hub moves; their own children follow them.
		// The meshes can sit deeper (Harvest keeps the import's Jeep_Wheel group), so the tyre is searched in all descendants.
		TArray<USceneComponent*> HubChildren;
		Hub->GetChildrenComponents(false, HubChildren);
		TArray<USceneComponent*> HubDescendants;
		Hub->GetChildrenComponents(true, HubDescendants);

		// The tyre is round, so its bounds centre is on the axle whatever the mesh pivot is.
		// Without a Tyre tag, fall back to the largest mesh.
		UPrimitiveComponent* Tyre = nullptr;
		for (USceneComponent* Descendant : HubDescendants)
		{
			UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Descendant);
			if (!Primitive)
			{
				continue;
			}
			if (Primitive->ComponentHasTag(VehicleTwinTags::Tyre))
			{
				Tyre = Primitive;
				break;
			}
			if (!Tyre || Primitive->CalcBounds(Primitive->GetComponentTransform()).SphereRadius > Tyre->CalcBounds(Tyre->GetComponentTransform()).SphereRadius)
			{
				Tyre = Primitive;
			}
		}
		if (!Tyre)
		{
			if (bLogProblems)
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: %s has no meshes under it; attach the rim and tyre to it."), *GetName(), *Hub->GetName());
			}
			continue;
		}
		if (bLogProblems && !Tyre->ComponentHasTag(VehicleTwinTags::Tyre))
		{
			UE_LOG(LogVehicleTwin, Warning, TEXT("%s: no mesh under %s tagged Tyre; using the largest one (%s)."), *GetName(), *Hub->GetName(), *Tyre->GetName());
		}

		// Move the hub without moving the meshes: remember their world transforms and put them back afterwards.
		TArray<FTransform> ChildWorldTransforms;
		for (USceneComponent* Child : HubChildren)
		{
			ChildWorldTransforms.Add(Child->GetComponentTransform());
		}

		const FVector WheelCentre = Tyre->CalcBounds(Tyre->GetComponentTransform()).Origin;
		Hub->SetWorldLocationAndRotation(WheelCentre, ActorRotation);

		for (int32 ChildIndex = 0; ChildIndex < HubChildren.Num(); ++ChildIndex)
		{
			HubChildren[ChildIndex]->SetWorldTransform(ChildWorldTransforms[ChildIndex]);
		}

		// Hub axes now match the actor. A tyre is thinnest along its axle: that axis is the spin axis,
		// and the radius is the larger of the other two half-extents.
		const FTransform TyreInHubSpace = Tyre->GetComponentTransform().GetRelativeTransform(Hub->GetComponentTransform());
		const FVector Extent = Tyre->CalcBounds(TyreInHubSpace).BoxExtent;
		const int32 AxleIndex = (Extent.X <= Extent.Y && Extent.X <= Extent.Z) ? 0 : (Extent.Y <= Extent.Z ? 1 : 2);

		FVehicleTwinWheel& Wheel = Wheels.AddDefaulted_GetRef();
		Wheel.Hub = Hub;
		Wheel.BaseRelativeRotation = Hub->GetRelativeRotation().Quaternion();
		Wheel.SpinAxis = FVector::ZeroVector;
		Wheel.SpinAxis[AxleIndex] = 1.f;
		Wheel.RadiusCm = FMath::Max(Extent[(AxleIndex + 1) % 3], Extent[(AxleIndex + 2) % 3]);
		Wheel.bFront = TagIndex < 2;

		if (bLogProblems)
		{
			UE_LOG(LogVehicleTwin, Log, TEXT("%s: %s axle along %s, radius %.1f cm."), *GetName(), *WheelTag.ToString(),
				AxleIndex == 0 ? TEXT("X") : (AxleIndex == 1 ? TEXT("Y") : TEXT("Z")), Wheel.RadiusCm);
			if (AxleIndex != 1)
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: axle is not along Y, so the car doesn't face +X. Adjust CarRoot's rotation (SPEC.md §1)."), *GetName());
			}
		}
	}

	if (bLogProblems)
	{
		UE_LOG(LogVehicleTwin, Log, TEXT("%s: %d of 4 road wheels set up."), *GetName(), Wheels.Num());
	}
}

void AVehicleTwinActor::CreatePaintMaterials()
{
	PaintMaterials.Reset();

	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!Primitive->ComponentHasTag(VehicleTwinTags::Paint))
		{
			continue;
		}
		for (int32 MaterialIndex = 0; MaterialIndex < Primitive->GetNumMaterials(); ++MaterialIndex)
		{
			if (UMaterialInstanceDynamic* Material = Primitive->CreateDynamicMaterialInstance(MaterialIndex))
			{
				PaintMaterials.Add(Material);
			}
		}
	}
}

void AVehicleTwinActor::ApplyWheelRotation(const FVehicleTwinWheel& Wheel, float SteerDeg) const
{
	// Spin around the axle first, then steer around up, so the wheel rolls about the steered axle.
	// All hubs share the actor axes, so there is no per-side sign. Positive spin about +Y rolls the top of the wheel towards +X.
	const float Steer = Wheel.bFront ? SteerDeg : 0.f;
	const FQuat SteerRotation(FVector::UpVector, FMath::DegreesToRadians(Steer));
	const FQuat SpinRotation(Wheel.SpinAxis, FMath::DegreesToRadians(Wheel.SpinDeg));
	Wheel.Hub->SetRelativeRotation(Wheel.BaseRelativeRotation * SteerRotation * SpinRotation);
}
