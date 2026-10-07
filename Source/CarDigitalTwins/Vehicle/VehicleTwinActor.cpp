#include "Vehicle/VehicleTwinActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshResources.h"
#include "Telemetry/TelemetrySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogVehicleTwin, Log, All);

namespace VehicleTwinTags
{
	// Index order matters: the first two are the steered front wheels, and each left wheel is followed by its right partner.
	static const FName Wheels[] = { TEXT("Wheel.FL"), TEXT("Wheel.FR"), TEXT("Wheel.RL"), TEXT("Wheel.RR") };
	static const FName Tyre = TEXT("Tyre");
	static const FName Paint = TEXT("Paint");
	static const FName Trim = TEXT("Trim");
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

	FVehicleTwinPaintGroup& BodyPaintGroup = PaintGroups.AddDefaulted_GetRef();
	BodyPaintGroup.ComponentTag = VehicleTwinTags::Paint;
	FVehicleTwinPaintGroup& TrimPaintGroup = PaintGroups.AddDefaulted_GetRef();
	TrimPaintGroup.ComponentTag = VehicleTwinTags::Trim;
}

void AVehicleTwinActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs on every edit in the editor, so problems are only logged from BeginPlay.
	SetUpWheels(false);

	if (bPreviewStatusColor)
	{
		CreatePaintMaterials(false);
		SetStatusColor(PreviewStatusColor, PreviewStatusBlend);
	}
	else if (!PaintMaterials.IsEmpty())
	{
		// Preview was just switched off: back to the original paint.
		SetStatusColor(FLinearColor::White, 0.f);
	}

	// Construction can recreate the Blueprint's components, so the outline mesh list is rebuilt every time.
	CollectStatusOutlineMeshComponents();
	if (bPreviewStatusOutline)
	{
		SetStatusOutline(PreviewVehicleStatus);
	}
}

void AVehicleTwinActor::BeginPlay()
{
	Super::BeginPlay();

	// Placed actors in a cooked build don't rerun construction, so resolve the wheels here as well.
	SetUpWheels(true);
	CreatePaintMaterials(true);

	if (bPreviewStatusColor)
	{
		SetStatusColor(PreviewStatusColor, PreviewStatusBlend);
	}

	CollectStatusOutlineMeshComponents();
	if (bDriveFromTelemetry || bPreviewStatusOutline)
	{
		// Logs a missing or wrong outline material once here instead of every frame.
		EnsureStatusOutlinePostProcess(true);
	}

	if (bDriveFromTelemetry)
	{
		const UGameInstance* OwningGameInstance = GetGameInstance();
		UTelemetrySubsystem* TelemetrySubsystem = OwningGameInstance ? OwningGameInstance->GetSubsystem<UTelemetrySubsystem>() : nullptr;
		if (TelemetrySubsystem)
		{
			SubscribedTelemetrySubsystem = TelemetrySubsystem;
			TelemetryUpdatedDelegateHandle = TelemetrySubsystem->OnTelemetryUpdated.AddUObject(this, &AVehicleTwinActor::HandleTelemetrySampleReceived);
		}
		else
		{
			UE_LOG(LogVehicleTwin, Warning, TEXT("%s: no UTelemetrySubsystem, the car won't follow telemetry."), *GetName());
		}
	}
}

void AVehicleTwinActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get())
	{
		TelemetrySubsystem->OnTelemetryUpdated.Remove(TelemetryUpdatedDelegateHandle);
	}
	SubscribedTelemetrySubsystem.Reset();
	TelemetryUpdatedDelegateHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void AVehicleTwinActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Telemetry wins once samples flow; until then (and in the editor) the preview settings drive the car.
	const UTelemetrySubsystem* TelemetrySubsystem = SubscribedTelemetrySubsystem.Get();
	const bool bTelemetryDrivesCar = TelemetrySubsystem && TelemetrySubsystem->HasReceivedAnyTelemetrySample();
	if (bTelemetryDrivesCar)
	{
		DriveFromTelemetry(*TelemetrySubsystem, DeltaSeconds);
	}
	else if (bPreviewStatusOutline)
	{
		SetStatusOutline(PreviewVehicleStatus);
	}

	if (DisplayedOutlineStatus != EVehicleStatus::Normal)
	{
		UpdateStatusOutlinePulse(DeltaSeconds);
	}

	if (bPreviewInEditor && !bTelemetryDrivesCar)
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
	// The outline preview ticks too, for its pulse.
	return bPreviewInEditor || bPreviewStatusOutline;
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

void AVehicleTwinActor::SetStatusColor(FLinearColor NewStatusColor, float NewStatusBlend)
{
	if (PaintMaterials.IsEmpty())
	{
		CreatePaintMaterials(false);
	}

	const float ClampedStatusBlend = FMath::Clamp(NewStatusBlend, 0.f, 1.f);
	for (int32 PaintMaterialIndex = 0; PaintMaterialIndex < PaintMaterials.Num(); ++PaintMaterialIndex)
	{
		if (UMaterialInstanceDynamic* PaintMaterialInstance = PaintMaterials[PaintMaterialIndex])
		{
			PaintMaterialInstance->SetVectorParameterValue(StatusColorParameter, NewStatusColor);
			PaintMaterialInstance->SetScalarParameterValue(StatusBlendParameter, ClampedStatusBlend * PaintMaterialStatusBlendScales[PaintMaterialIndex]);
		}
	}
}

void AVehicleTwinActor::SetStatusOutline(EVehicleStatus NewVehicleStatus)
{
	if (NewVehicleStatus == DisplayedOutlineStatus)
	{
		return;
	}

	const bool bShowOutline = NewVehicleStatus != EVehicleStatus::Normal;
	if (bShowOutline && !EnsureStatusOutlinePostProcess(false))
	{
		// No outline material: nothing could draw it, so don't pay for the custom depth pass either.
		return;
	}
	if (StatusOutlineMeshComponents.IsEmpty())
	{
		CollectStatusOutlineMeshComponents();
	}

	// Only while the outline shows: custom depth draws the whole car a second time (SPEC.md §7).
	for (UPrimitiveComponent* OutlineMeshComponent : StatusOutlineMeshComponents)
	{
		if (OutlineMeshComponent)
		{
			OutlineMeshComponent->SetCustomDepthStencilValue(StatusOutlineStencilValue);
			OutlineMeshComponent->SetRenderCustomDepth(bShowOutline);
		}
	}

	DisplayedOutlineStatus = NewVehicleStatus;

	// Each new status starts at full glow, so the change is visible straight away.
	StatusOutlinePulseSeconds = 0.f;
	UpdateStatusOutlinePulse(0.f);
}

bool AVehicleTwinActor::EnsureStatusOutlinePostProcess(bool bLogProblems)
{
	if (StatusOutlinePostProcess && StatusOutlineMaterialInstance)
	{
		return true;
	}
	if (!StatusOutlineMaterial)
	{
		if (bLogProblems)
		{
			UE_LOG(LogVehicleTwin, Warning, TEXT("%s: StatusOutlineMaterial is not set, so the status outline won't show (SPEC.md §1)."), *GetName());
		}
		return false;
	}
	if (bLogProblems)
	{
		const UMaterial* OutlineBaseMaterial = StatusOutlineMaterial->GetMaterial();
		if (OutlineBaseMaterial && OutlineBaseMaterial->MaterialDomain != MD_PostProcess)
		{
			UE_LOG(LogVehicleTwin, Warning, TEXT("%s: %s is not a Post Process material, so the status outline won't show."),
				*GetName(), *StatusOutlineMaterial->GetName());
		}
	}

	if (!StatusOutlinePostProcess)
	{
		// Unbound, so its position doesn't matter and it isn't attached: construction can recreate the Blueprint's root.
		StatusOutlinePostProcess = NewObject<UPostProcessComponent>(this, NAME_None, RF_Transient);
		StatusOutlinePostProcess->bUnbound = true;
		StatusOutlinePostProcess->RegisterComponent();
	}

	StatusOutlineMaterialInstance = UMaterialInstanceDynamic::Create(StatusOutlineMaterial, this);
	StatusOutlinePostProcess->Settings.WeightedBlendables.Array.Reset();
	StatusOutlinePostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, StatusOutlineMaterialInstance));
	return true;
}

void AVehicleTwinActor::CollectStatusOutlineMeshComponents()
{
	// Components from an earlier collection may outlive it (construction reruns); take them out of the outline first.
	for (UPrimitiveComponent* PreviousOutlineMeshComponent : StatusOutlineMeshComponents)
	{
		if (IsValid(PreviousOutlineMeshComponent))
		{
			PreviousOutlineMeshComponent->SetRenderCustomDepth(false);
		}
	}
	StatusOutlineMeshComponents.Reset();
	DisplayedOutlineStatus = EVehicleStatus::Normal;

	TInlineComponentArray<UPrimitiveComponent*> CarMeshComponents(this, /*bIncludeFromChildActors*/ true);
	for (UPrimitiveComponent* CarMeshComponent : CarMeshComponents)
	{
		StatusOutlineMeshComponents.Add(CarMeshComponent);
	}
}

void AVehicleTwinActor::UpdateStatusOutlinePulse(float DeltaSeconds)
{
	if (!StatusOutlineMaterialInstance)
	{
		return;
	}

	StatusOutlinePulseSeconds += DeltaSeconds;

	const bool bCritical = DisplayedOutlineStatus == EVehicleStatus::Critical;
	const float OutlinePulseHz = bCritical ? CriticalOutlinePulseHz : WarningOutlinePulseHz;

	// Cosine pulse starting at full glow: dims by up to OutlinePulseDepth and back, OutlinePulseHz times a second.
	const float PulseDimming = OutlinePulseDepth * 0.5f * (1.f - FMath::Cos(UE_TWO_PI * OutlinePulseHz * StatusOutlinePulseSeconds));
	const float GlowIntensity = DisplayedOutlineStatus == EVehicleStatus::Normal ? 0.f : OutlineGlowIntensity * (1.f - PulseDimming);

	StatusOutlineMaterialInstance->SetVectorParameterValue(StatusOutlineColorParameter, bCritical ? CriticalOutlineColor : WarningOutlineColor);
	StatusOutlineMaterialInstance->SetScalarParameterValue(StatusOutlineGlowIntensityParameter, GlowIntensity);
}

void AVehicleTwinActor::HandleTelemetrySampleReceived(const FVehicleTelemetry& NewTelemetrySample)
{
	const double NowS = GetWorld()->GetTimeSeconds();
	if (LatestSampleArrivalTimeS >= 0.0)
	{
		// Several samples can arrive in one frame (high playback speed); keep the last real span instead of zero.
		const double SinceLatestSampleS = NowS - LatestSampleArrivalTimeS;
		if (SinceLatestSampleS > UE_KINDA_SMALL_NUMBER)
		{
			SampleArrivalIntervalS = SinceLatestSampleS;
		}
	}
	LatestSampleArrivalTimeS = NowS;
}

void AVehicleTwinActor::DriveFromTelemetry(const UTelemetrySubsystem& TelemetrySubsystem, float DeltaSeconds)
{
	const FVehicleTelemetry& PreviousTelemetrySample = TelemetrySubsystem.GetPreviousTelemetrySample();
	const FVehicleTelemetry& LatestTelemetrySample = TelemetrySubsystem.GetLatestTelemetrySample();

	// Shows the motion one sample behind, blending previous -> latest over the time the latest one took to arrive, so 10 Hz data
	// moves smoothly at any frame rate. A trip loop jumps back in time: snap to the new sample instead of blending across it.
	const bool bTripLooped = LatestTelemetrySample.SampleTimeS < PreviousTelemetrySample.SampleTimeS;
	const double SinceLatestSampleS = GetWorld()->GetTimeSeconds() - LatestSampleArrivalTimeS;
	const float BlendAlpha = bTripLooped ? 1.f : static_cast<float>(FMath::Clamp(SinceLatestSampleS / SampleArrivalIntervalS, 0.0, 1.0));

	UpdateWheels(
		FMath::Lerp(PreviousTelemetrySample.SpeedKmh, LatestTelemetrySample.SpeedKmh, BlendAlpha),
		FMath::Lerp(PreviousTelemetrySample.SteerDeg, LatestTelemetrySample.SteerDeg, BlendAlpha),
		DeltaSeconds);

	// Status is not blended: it comes from real samples only (FVehicleStatusEvaluator).
	SetStatusOutline(TelemetrySubsystem.GetCurrentVehicleStatusReport().OverallStatus);
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

void AVehicleTwinActor::CreatePaintMaterials(bool bLogProblems)
{
	PaintMaterials.Reset();
	PaintMaterialStatusBlendScales.Reset();

	TInlineComponentArray<USceneComponent*> AllSceneComponents(this);
	for (const FVehicleTwinPaintGroup& PaintGroup : PaintGroups)
	{
		TArray<UPrimitiveComponent*> GroupMeshComponents;
		for (USceneComponent* GroupCandidateComponent : AllSceneComponents)
		{
			if (GroupCandidateComponent->ComponentHasTag(PaintGroup.ComponentTag))
			{
				VehicleTwinComponentLookup::CollectMeshComponentsForTaggedComponent(GroupCandidateComponent, GroupMeshComponents);
			}
		}

		const int32 GroupFirstMaterialIndex = PaintMaterials.Num();
		TArray<FString> MeshesWithoutStatusColorForLog;
		for (UPrimitiveComponent* GroupMeshComponent : GroupMeshComponents)
		{
			bool bMeshHasStatusColorSlot = false;
			for (int32 MaterialSlotIndex = 0; MaterialSlotIndex < GroupMeshComponent->GetNumMaterials(); ++MaterialSlotIndex)
			{
				const UMaterialInterface* SlotMaterial = GroupMeshComponent->GetMaterial(MaterialSlotIndex);
				FLinearColor UnusedParameterValue;
				if (!SlotMaterial || !SlotMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(StatusColorParameter), UnusedParameterValue))
				{
					continue;
				}
				// Reuses the slot's dynamic instance if it already has one, so calling this again doesn't stack instances.
				if (UMaterialInstanceDynamic* PaintMaterialInstance = GroupMeshComponent->CreateDynamicMaterialInstance(MaterialSlotIndex))
				{
					PaintMaterials.Add(PaintMaterialInstance);
					PaintMaterialStatusBlendScales.Add(PaintGroup.StatusBlendScale);
					bMeshHasStatusColorSlot = true;
				}
			}
			if (!bMeshHasStatusColorSlot)
			{
				MeshesWithoutStatusColorForLog.Add(FString::Printf(TEXT("%s (%s)"), *GetNameSafe(GroupMeshComponent->GetOwner()),
					*GetNameSafe(GroupMeshComponent->GetMaterial(0))));
			}
		}

		if (bLogProblems)
		{
			UE_LOG(LogVehicleTwin, Log, TEXT("%s: group %s: %d material slots on %d meshes, blend scale %.2f."), *GetName(),
				*PaintGroup.ComponentTag.ToString(), PaintMaterials.Num() - GroupFirstMaterialIndex, GroupMeshComponents.Num(), PaintGroup.StatusBlendScale);
			if (!MeshesWithoutStatusColorForLog.IsEmpty())
			{
				UE_LOG(LogVehicleTwin, Warning, TEXT("%s: %d meshes tagged %s have no material with a %s parameter, so they won't change colour: %s"),
					*GetName(), MeshesWithoutStatusColorForLog.Num(), *PaintGroup.ComponentTag.ToString(), *StatusColorParameter.ToString(),
					*FString::Join(MeshesWithoutStatusColorForLog, TEXT(", ")));
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
