#include "Vehicle/VehicleTwinActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTwin, Log, All);

namespace VehicleTwinTags
{
	// Index order matters: the first two are the steered front wheels, and each left wheel is followed by its right partner.
	static const FName Wheels[] = { TEXT("Wheel.FL"), TEXT("Wheel.FR"), TEXT("Wheel.RL"), TEXT("Wheel.RR") };
	static const FName Tyre = TEXT("Tyre");
	static const FName Paint = TEXT("Paint");
}

namespace VehicleTwinComponentLookup
{
	// Harvest Components wraps each imported StaticMeshActor in a ChildActorComponent: the tag sits on that component and the
	// mesh on the child actor's own components. Adds the mesh components a tagged component stands for.
	static void CollectMeshComponentsForTaggedComponent(USceneComponent* TaggedComponent, TArray<UPrimitiveComponent*>& OutMeshComponents)
	{
		if (UPrimitiveComponent* TaggedPrimitiveComponent = Cast<UPrimitiveComponent>(TaggedComponent))
		{
			OutMeshComponents.Add(TaggedPrimitiveComponent);
			return;
		}
		if (const UChildActorComponent* TaggedChildActorComponent = Cast<UChildActorComponent>(TaggedComponent))
		{
			if (AActor* SpawnedChildActor = TaggedChildActorComponent->GetChildActor())
			{
				TInlineComponentArray<UPrimitiveComponent*> SpawnedChildActorPrimitiveComponents(SpawnedChildActor);
				OutMeshComponents.Append(SpawnedChildActorPrimitiveComponents);
			}
		}
	}

	// Puts a Blueprint-created component back to the relative transform authored in the Blueprint (its archetype).
	static void ResetToAuthoredRelativeTransform(USceneComponent* ComponentToReset)
	{
		const USceneComponent* AuthoredTemplate = Cast<USceneComponent>(ComponentToReset->GetArchetype());
		if (AuthoredTemplate && !AuthoredTemplate->HasAnyFlags(RF_ClassDefaultObject))
		{
			ComponentToReset->SetRelativeTransform(AuthoredTemplate->GetRelativeTransform());
		}
	}

	static float GetWorldBoundsSphereRadius(const UPrimitiveComponent* MeasuredComponent)
	{
		return MeasuredComponent->CalcBounds(MeasuredComponent->GetComponentTransform()).SphereRadius;
	}
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

		// Debug: yellow dot = wheel centre, red line = spin axis, blue = up, green circle = measured tyre radius (drawn around the axle).
		for (const FVehicleTwinWheel& Wheel : Wheels)
		{
			const FTransform HubWorldTransform = Wheel.Hub->GetComponentTransform();
			const FVector WheelCentreWorld = HubWorldTransform.TransformPosition(Wheel.WheelCentreInHubSpace);
			const FVector AxleWorldDirection = HubWorldTransform.TransformVectorNoScale(Wheel.SpinAxis);
			const FVector ActorUpDirection = GetActorUpVector();
			DrawDebugPoint(GetWorld(), WheelCentreWorld, 12.f, FColor::Yellow, false, -1.f, SDPG_Foreground);
			DrawDebugLine(GetWorld(), WheelCentreWorld - AxleWorldDirection * 60.f, WheelCentreWorld + AxleWorldDirection * 60.f, FColor::Red, false, -1.f, SDPG_Foreground, 1.5f);
			DrawDebugLine(GetWorld(), WheelCentreWorld, WheelCentreWorld + ActorUpDirection * 60.f, FColor::Blue, false, -1.f, SDPG_Foreground, 1.5f);
			FVector CirclePlaneAxisA, CirclePlaneAxisB;
			AxleWorldDirection.FindBestAxisVectors(CirclePlaneAxisA, CirclePlaneAxisB);
			DrawDebugCircle(GetWorld(), WheelCentreWorld, Wheel.RadiusCm, 32, FColor::Green, false, -1.f, SDPG_Foreground, 1.f,
				CirclePlaneAxisA, CirclePlaneAxisB, false);
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

void AVehicleTwinActor::SetStatusColor(FLinearColor NewStatusColor)
{
	if (PaintMaterials.IsEmpty())
	{
		CreatePaintMaterials();
	}

	for (UMaterialInstanceDynamic* PaintMaterialInstance : PaintMaterials)
	{
		PaintMaterialInstance->SetVectorParameterValue(StatusColorParameter, NewStatusColor);
	}
}

void AVehicleTwinActor::SetUpWheels(bool bLogProblems)
{
	Wheels.Reset();

	TInlineComponentArray<USceneComponent*> SceneComponents(this);

	// Parallel to Wheels.
	TArray<UPrimitiveComponent*, TInlineAllocator<4>> WheelTyres;
	TArray<int32, TInlineAllocator<4>> WheelTagIndices;

	for (int32 TagIndex = 0; TagIndex < UE_ARRAY_COUNT(VehicleTwinTags::Wheels); ++TagIndex)
	{
		const FName WheelTag = VehicleTwinTags::Wheels[TagIndex];
		USceneComponent* const* FoundHubComponent = SceneComponents.FindByPredicate(
			[WheelTag](const USceneComponent* Component) { return Component->ComponentHasTag(WheelTag); });
		if (!FoundHubComponent)
		{
			if (bLogProblems)
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: no component tagged %s (SPEC.md §1)."), *GetName(), *WheelTag.ToString());
			}
			continue;
		}
		USceneComponent* HubComponent = *FoundHubComponent;

		// Back to the authored pose before measuring, so a wheel caught mid-spin (setup reruns on every edit while previewing) is
		// not measured or baked in that pose. Earlier versions of this class moved the hub's direct children, so reset those too.
		VehicleTwinComponentLookup::ResetToAuthoredRelativeTransform(HubComponent);
		TArray<USceneComponent*> HubChildren;
		HubComponent->GetChildrenComponents(false, HubChildren);
		for (USceneComponent* HubChildComponent : HubChildren)
		{
			VehicleTwinComponentLookup::ResetToAuthoredRelativeTransform(HubChildComponent);
		}

		// The meshes can sit deeper (Harvest keeps the import's Jeep_Wheel group), so the tyre is searched in all descendants.
		TArray<USceneComponent*> HubDescendants;
		HubComponent->GetChildrenComponents(true, HubDescendants);

		// The tyre is round, so its bounds centre is on the axle whatever the mesh pivot is.
		// Candidates are the meshes behind the component tagged Tyre; without one, every mesh under the hub. The largest wins.
		TArray<UPrimitiveComponent*> TyreCandidateMeshComponents;
		for (USceneComponent* HubDescendantComponent : HubDescendants)
		{
			if (HubDescendantComponent->ComponentHasTag(VehicleTwinTags::Tyre))
			{
				VehicleTwinComponentLookup::CollectMeshComponentsForTaggedComponent(HubDescendantComponent, TyreCandidateMeshComponents);
			}
		}
		const bool bTyreFoundByTag = !TyreCandidateMeshComponents.IsEmpty();
		if (!bTyreFoundByTag)
		{
			for (USceneComponent* HubDescendantComponent : HubDescendants)
			{
				if (UPrimitiveComponent* HubDescendantPrimitiveComponent = Cast<UPrimitiveComponent>(HubDescendantComponent))
				{
					TyreCandidateMeshComponents.Add(HubDescendantPrimitiveComponent);
				}
			}
		}

		UPrimitiveComponent* TyreComponent = nullptr;
		for (UPrimitiveComponent* TyreCandidateMeshComponent : TyreCandidateMeshComponents)
		{
			if (!TyreComponent || VehicleTwinComponentLookup::GetWorldBoundsSphereRadius(TyreCandidateMeshComponent) > VehicleTwinComponentLookup::GetWorldBoundsSphereRadius(TyreComponent))
			{
				TyreComponent = TyreCandidateMeshComponent;
			}
		}
		if (!TyreComponent)
		{
			if (bLogProblems)
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: %s has no meshes under it; attach the rim and tyre to it."), *GetName(), *HubComponent->GetName());
			}
			continue;
		}
		if (bLogProblems && !bTyreFoundByTag)
		{
			UE_LOG(LogVehicleTwin, Warning, TEXT("%s: no mesh under %s tagged Tyre; using the largest one (%s). Components under the hub:"),
				*GetName(), *HubComponent->GetName(), *TyreComponent->GetName());
			for (const USceneComponent* HubDescendantComponent : HubDescendants)
			{
				const UStaticMeshComponent* StaticMeshComponentForLog = Cast<UStaticMeshComponent>(HubDescendantComponent);
				FString TagListForLog;
				for (const FName& ComponentTagName : HubDescendantComponent->ComponentTags)
				{
					TagListForLog += ComponentTagName.ToString() + TEXT(" ");
				}
				UE_LOG(LogVehicleTwin, Warning, TEXT("    %s (%s, owner %s, mesh %s, tags [%s])"), *HubDescendantComponent->GetName(), *HubDescendantComponent->GetClass()->GetName(),
					*GetNameSafe(HubDescendantComponent->GetOwner()), StaticMeshComponentForLog ? *GetNameSafe(StaticMeshComponentForLog->GetStaticMesh()) : TEXT("-"), *TagListForLog.TrimEnd());
			}
		}

		const FTransform HubWorldTransform = HubComponent->GetComponentTransform();
		const FVector WheelCentreWorld = TyreComponent->CalcBounds(TyreComponent->GetComponentTransform()).Origin;

		FVehicleTwinWheel& Wheel = Wheels.AddDefaulted_GetRef();
		Wheel.Hub = HubComponent;
		Wheel.BaseRelativeTransform = HubComponent->GetRelativeTransform();
		Wheel.WheelCentreInHubSpace = HubWorldTransform.InverseTransformPosition(WheelCentreWorld);
		Wheel.SteerAxisInHubSpace = HubWorldTransform.InverseTransformVectorNoScale(GetActorUpVector()).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
		Wheel.bFront = TagIndex < 2;
		WheelTyres.Add(TyreComponent);
		WheelTagIndices.Add(TagIndex);
	}

	// The CAD tyre meshes are rotated inside their own local space, so their bounds are loose boxes: neither the thinnest axis
	// nor the extents can be trusted. The axle is the line from the left wheel's centre to its right partner's instead.
	// It points to the car's right, so positive spin rolls the top of the wheel forwards.
	for (int32 WheelIndex = 0; WheelIndex < Wheels.Num(); ++WheelIndex)
	{
		FVehicleTwinWheel& Wheel = Wheels[WheelIndex];
		const int32 TagIndex = WheelTagIndices[WheelIndex];
		const int32 PartnerIndex = WheelTagIndices.IndexOfByKey(TagIndex ^ 1);
		const bool bLeft = (TagIndex % 2) == 0;

		const FTransform HubWorldTransform = Wheel.Hub->GetComponentTransform();
		FVector AxleWorldDirection = GetActorRightVector();
		if (PartnerIndex != INDEX_NONE)
		{
			const FVehicleTwinWheel& LeftWheel = Wheels[bLeft ? WheelIndex : PartnerIndex];
			const FVehicleTwinWheel& RightWheel = Wheels[bLeft ? PartnerIndex : WheelIndex];
			const FVector LeftCentreWorld = LeftWheel.Hub->GetComponentTransform().TransformPosition(LeftWheel.WheelCentreInHubSpace);
			const FVector RightCentreWorld = RightWheel.Hub->GetComponentTransform().TransformPosition(RightWheel.WheelCentreInHubSpace);
			AxleWorldDirection = (RightCentreWorld - LeftCentreWorld).GetSafeNormal(KINDA_SMALL_NUMBER, GetActorRightVector());
		}
		else if (bLogProblems)
		{
			UE_LOG(LogVehicleTwin, Warning, TEXT("%s: %s has no partner wheel; assuming the axle is along the actor's Y axis."),
				*GetName(), *VehicleTwinTags::Wheels[TagIndex].ToString());
		}

		Wheel.SpinAxis = HubWorldTransform.InverseTransformVectorNoScale(AxleWorldDirection).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::RightVector);

		bool bFromVertices = false;
		Wheel.RadiusCm = MeasureTyreRadius(WheelTyres[WheelIndex], HubWorldTransform, Wheel.WheelCentreInHubSpace, Wheel.SpinAxis, bFromVertices);

		if (bLogProblems)
		{
			const FVector AxleActorDirection = GetActorQuat().UnrotateVector(AxleWorldDirection);
			UE_LOG(LogVehicleTwin, Log, TEXT("%s: %s axle %s (actor space), radius %.1f cm%s."), *GetName(), *VehicleTwinTags::Wheels[TagIndex].ToString(),
				*AxleActorDirection.ToCompactString(), Wheel.RadiusCm, bFromVertices ? TEXT("") : TEXT(" (from bounds: no CPU vertex access)"));
			if (FMath::Abs(AxleActorDirection.Y) < 0.95f)
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: axle is not along Y, so the car doesn't face +X. Adjust CarRoot's yaw (SPEC.md §1)."), *GetName());
			}
		}
	}

	if (bLogProblems)
	{
		UE_LOG(LogVehicleTwin, Log, TEXT("%s: %d of 4 road wheels set up."), *GetName(), Wheels.Num());
	}
}

float AVehicleTwinActor::MeasureTyreRadius(const UPrimitiveComponent* TyreComponent, const FTransform& HubWorldTransform, const FVector& WheelCentreInHubSpace,
	const FVector& AxleInHubSpace, bool& bOutFromVertices)
{
	bOutFromVertices = false;

	// Exact radius: the largest distance of any tyre vertex from the axle line through the wheel centre.
	// In cooked builds the vertices are only kept on the CPU when the mesh has Allow CPU Access set.
	const UStaticMeshComponent* TyreStaticMeshComponent = Cast<UStaticMeshComponent>(TyreComponent);
	const UStaticMesh* TyreStaticMesh = TyreStaticMeshComponent ? TyreStaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* TyreRenderData = TyreStaticMesh ? TyreStaticMesh->GetRenderData() : nullptr;
	if (TyreRenderData && !TyreRenderData->LODResources.IsEmpty())
	{
		const FPositionVertexBuffer& TyreVertexPositions = TyreRenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
		if (TyreVertexPositions.GetNumVertices() > 0 && TyreVertexPositions.GetVertexData())
		{
			const FTransform TyreToHubTransform = TyreComponent->GetComponentTransform().GetRelativeTransform(HubWorldTransform);
			double MaxDistanceSquared = 0.0;
			for (uint32 VertexIndex = 0; VertexIndex < TyreVertexPositions.GetNumVertices(); ++VertexIndex)
			{
				const FVector VertexFromWheelCentre = TyreToHubTransform.TransformPosition(FVector(TyreVertexPositions.VertexPosition(VertexIndex))) - WheelCentreInHubSpace;
				MaxDistanceSquared = FMath::Max(MaxDistanceSquared,
					(VertexFromWheelCentre - AxleInHubSpace * FVector::DotProduct(VertexFromWheelCentre, AxleInHubSpace)).SizeSquared());
			}
			bOutFromVertices = true;
			return static_cast<float>(FMath::Sqrt(MaxDistanceSquared));
		}
	}

	// Fallback: the bounding sphere is slightly larger than the tyre (it reaches the tread's outer corners).
	return TyreComponent->CalcBounds(TyreComponent->GetComponentTransform()).SphereRadius;
}

void AVehicleTwinActor::CreatePaintMaterials()
{
	PaintMaterials.Reset();

	TArray<UPrimitiveComponent*> PaintedMeshComponents;
	TInlineComponentArray<USceneComponent*> AllSceneComponents(this);
	for (USceneComponent* PaintCandidateComponent : AllSceneComponents)
	{
		if (PaintCandidateComponent->ComponentHasTag(VehicleTwinTags::Paint))
		{
			VehicleTwinComponentLookup::CollectMeshComponentsForTaggedComponent(PaintCandidateComponent, PaintedMeshComponents);
		}
	}

	for (UPrimitiveComponent* PaintedMeshComponent : PaintedMeshComponents)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < PaintedMeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			if (UMaterialInstanceDynamic* PaintMaterialInstance = PaintedMeshComponent->CreateDynamicMaterialInstance(MaterialIndex))
			{
				PaintMaterials.Add(PaintMaterialInstance);
			}
		}
	}
}

void AVehicleTwinActor::ApplyWheelRotation(const FVehicleTwinWheel& Wheel, float SteerDeg) const
{
	// Spin around the axle first, then steer around up, so the wheel rolls about the steered axle. Both axes point the same way
	// on every wheel (axle to the car's right), so there is no per-side sign: positive spin rolls the top of the wheel forwards.
	const float AppliedSteerDeg = Wheel.bFront ? SteerDeg : 0.f;
	const FQuat SteerRotation(Wheel.SteerAxisInHubSpace, FMath::DegreesToRadians(AppliedSteerDeg));
	const FQuat SpinRotation(Wheel.SpinAxis, FMath::DegreesToRadians(Wheel.SpinDeg));

	// Rotate about the wheel centre instead of the hub origin: move the centre to the origin, rotate, move it back, then apply
	// the authored hub transform. FTransform composes left to right. Assumes the hub has uniform scale.
	const FTransform RotationAboutWheelCentre =
		FTransform(-Wheel.WheelCentreInHubSpace) * FTransform(SteerRotation * SpinRotation) * FTransform(Wheel.WheelCentreInHubSpace);
	Wheel.Hub->SetRelativeTransform(RotationAboutWheelCentre * Wheel.BaseRelativeTransform);
}
