// Automation tests for AVehicleTwinActor (docs/TESTS.md §3, V1–V10) on AVehicleTwinTestActor, a stand-in built from engine shapes,
// so no Jeep assets are needed. Each test spawns the car in its own empty game world. Run in the editor: Tools > Session Frontend >
// Automation, filter "CarDigitalTwins.Vehicle". Headless: UnrealEditor-Cmd.exe CarDigitalTwins.uproject
//   -ExecCmds="Automation RunTests CarDigitalTwins.Vehicle; Quit" -nullrhi -unattended -log
// Uses M_CarPaint's instances and PP_VehicleStatusOutline from /Game/Materials (in git).

#include "Misc/AutomationTest.h"
#include "Components/ChildActorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Tests/VehicleTwinTestActor.h"
#include "Vehicle/VehicleTwinActor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace VehicleTwinActorTestAssets
{
	const TCHAR* const CylinderMesh = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* const CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
	// No StatusColor parameter: stands in for glass.
	const TCHAR* const PlainMaterial = TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial");
	const TCHAR* const BodyPaintMaterial = TEXT("/Game/Materials/MI_CarPaint.MI_CarPaint");
	const TCHAR* const TrimPaintMaterial = TEXT("/Game/Materials/MI_CarPaintBlack.MI_CarPaintBlack");
	const TCHAR* const OutlineMaterial = TEXT("/Game/Materials/PP_VehicleStatusOutline.PP_VehicleStatusOutline");
}

BEGIN_DEFINE_SPEC(FVehicleTwinActorSpec, "CarDigitalTwins.Vehicle.TwinActor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
	AVehicleTwinTestActor* Car = nullptr;

	// An empty game world that has begun play, so spawned actors get BeginPlay (where the car logs its setup problems).
	void CreateTestWorld()
	{
		TestWorld = UWorld::CreateWorld(EWorldType::Game, false, TEXT("VehicleTwinTestWorld"));
		FWorldContext& TestWorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		TestWorldContext.SetCurrentWorld(TestWorld);
		TestWorld->InitializeActorsForPlay(FURL());
		TestWorld->BeginPlay();
		// Without a game mode nothing starts play, so start it here.
		if (!TestWorld->HasBegunPlay())
		{
			TestWorld->GetWorldSettings()->NotifyBeginPlay();
		}
	}

	void DestroyTestWorld()
	{
		if (TestWorld)
		{
			GEngine->DestroyWorldContext(TestWorld);
			TestWorld->DestroyWorld(false);
			TestWorld->RemoveFromRoot();
			TestWorld = nullptr;
		}
		Car = nullptr;
	}

	// Spawns the stand-in car: meshes and paint assigned, then BeforeConstruction (tags, outline material), then construction and
	// BeginPlay. Returns null if an asset is missing.
	AVehicleTwinTestActor* SpawnCar(TFunction<void(AVehicleTwinTestActor&)> BeforeConstruction = nullptr)
	{
		using namespace VehicleTwinActorTestAssets;
		UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, CylinderMesh);
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, CubeMesh);
		UMaterialInterface* BodyPaint = LoadObject<UMaterialInterface>(nullptr, BodyPaintMaterial);
		UMaterialInterface* TrimPaint = LoadObject<UMaterialInterface>(nullptr, TrimPaintMaterial);
		UMaterialInterface* Plain = LoadObject<UMaterialInterface>(nullptr, PlainMaterial);
		if (!TestTrue(TEXT("engine shapes and car paint load"), Cylinder && Cube && BodyPaint && TrimPaint && Plain))
		{
			return nullptr;
		}

		AVehicleTwinTestActor* NewCar = TestWorld->SpawnActorDeferred<AVehicleTwinTestActor>(AVehicleTwinTestActor::StaticClass(), FTransform::Identity);
		for (UStaticMeshComponent* Tyre : NewCar->Tyres)
		{
			Tyre->SetStaticMesh(Cylinder);
		}
		NewCar->BodyPaintMesh->SetStaticMesh(Cube);
		NewCar->BodyPaintMesh->SetMaterial(0, BodyPaint);
		NewCar->TrimMesh->SetStaticMesh(Cube);
		NewCar->TrimMesh->SetMaterial(0, TrimPaint);
		NewCar->GlassMesh->SetStaticMesh(Cube);
		NewCar->GlassMesh->SetMaterial(0, Plain);
		if (BeforeConstruction)
		{
			BeforeConstruction(*NewCar);
		}
		NewCar->FinishSpawning(FTransform::Identity);
		TestTrue(TEXT("BeginPlay ran"), NewCar->HasActorBegunPlay());
		return NewCar;
	}

	FVector GetTyreCentreWorld(int32 WheelIndex) const
	{
		const UStaticMeshComponent* Tyre = Car->Tyres[WheelIndex];
		return Tyre->CalcBounds(Tyre->GetComponentTransform()).Origin;
	}

	// Where the material point that was at PointWorldBefore is now, after the hub moved from HubWorldBefore.
	static FVector FollowPointOnHub(const FVector& PointWorldBefore, const FTransform& HubWorldBefore, const USceneComponent& Hub)
	{
		return Hub.GetComponentTransform().TransformPosition(HubWorldBefore.InverseTransformPosition(PointWorldBefore));
	}

	float GetScalarParameter(UMaterialInterface* Material, FName ParameterName)
	{
		float ParameterValue = -1.f;
		if (Material)
		{
			Material->GetScalarParameterValue(FHashedMaterialParameterInfo(ParameterName), ParameterValue);
		}
		return ParameterValue;
	}
END_DEFINE_SPEC(FVehicleTwinActorSpec)

void FVehicleTwinActorSpec::Define()
{
	BeforeEach([this]()
	{
		CreateTestWorld();
	});

	AfterEach([this]()
	{
		DestroyTestWorld();
	});

	Describe("Wheels", [this]()
	{
		It("V1: four tagged hubs give four wheels with the axle to the right; a missing tag gives three and a warning", [this]()
		{
			Car = SpawnCar();
			if (!Car)
			{
				return;
			}
			const FName ExpectedTags[] = { TEXT("Wheel.FL"), TEXT("Wheel.FR"), TEXT("Wheel.RL"), TEXT("Wheel.RR") };
			if (!TestEqual(TEXT("four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
			{
				const FVehicleTwinWheel& Wheel = Car->Wheels[WheelIndex];
				TestTrue(TEXT("tag order FL, FR, RL, RR"), Wheel.WheelTag == ExpectedTags[WheelIndex]);
				TestTrue(TEXT("only FL and FR steer"), Wheel.bFront == (WheelIndex < 2));
				const FVector SpinAxisWorld = Wheel.Hub->GetComponentTransform().TransformVectorNoScale(Wheel.SpinAxis);
				TestTrue(FString::Printf(TEXT("%s axle points right (+Y): %s"), *Wheel.WheelTag.ToString(), *SpinAxisWorld.ToCompactString()),
					SpinAxisWorld.Equals(FVector::RightVector, 1e-3));
			}

			// Second car without the RR tag: RR is missing and RL has no partner to take its axle from.
			AddExpectedError(TEXT("no component tagged Wheel.RR"), EAutomationExpectedErrorFlags::Contains, 1, false);
			AddExpectedError(TEXT("Wheel.RL has no partner wheel"), EAutomationExpectedErrorFlags::Contains, 1, false);
			AVehicleTwinTestActor* CarWithoutRearRight = SpawnCar([](AVehicleTwinTestActor& NewCar)
			{
				NewCar.Hubs[3]->ComponentTags.Remove(TEXT("Wheel.RR"));
			});
			if (CarWithoutRearRight && TestEqual(TEXT("three wheels"), CarWithoutRearRight->GetNumWheels(), 3))
			{
				const FVehicleTwinWheel& RearLeft = CarWithoutRearRight->Wheels[2];
				TestTrue(TEXT("RL falls back to the actor's right axis"),
					RearLeft.Hub->GetComponentTransform().TransformVectorNoScale(RearLeft.SpinAxis).Equals(FVector::RightVector, 1e-3));
			}
		});

		It("V2: spin = distance / radius, positive rolls the top forward, and it wraps below 360", [this]()
		{
			Car = SpawnCar();
			if (!Car || !TestEqual(TEXT("four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			// Point on top of the FL tyre, followed as the hub turns.
			const FVector TopPointBefore = GetTyreCentreWorld(0) + FVector::UpVector * AVehicleTwinTestActor::TyreRadiusCm;
			const FTransform HubWorldBefore = Car->Hubs[0]->GetComponentTransform();

			// 36 km/h = 1000 cm/s; 0.1 s = 100 cm of road = 100 / 43.5 rad.
			Car->UpdateWheels(36.f, 0.f, 0.1f);
			const float ExpectedDeg = FMath::RadiansToDegrees(100.f / AVehicleTwinTestActor::TyreRadiusCm);
			for (const FVehicleTwinWheel& Wheel : Car->Wheels)
			{
				TestEqual(FString::Printf(TEXT("%s spin"), *Wheel.WheelTag.ToString()), Wheel.SpinDeg, ExpectedDeg, 0.01f);
			}
			const FVector TopPointAfter = FollowPointOnHub(TopPointBefore, HubWorldBefore, *Car->Hubs[0]);
			TestTrue(FString::Printf(TEXT("top of the tyre moved forward: %.2f -> %.2f"), TopPointBefore.X, TopPointAfter.X), TopPointAfter.X > TopPointBefore.X + 1.f);

			// 100 km/h for 100 s: far past one turn, still inside one turn.
			for (int32 StepIndex = 0; StepIndex < 100; ++StepIndex)
			{
				Car->UpdateWheels(100.f, 0.f, 1.f);
			}
			const double TotalDeg = ExpectedDeg + 100.0 * FMath::RadiansToDegrees((100.0 * 100000.0 / 3600.0) / AVehicleTwinTestActor::TyreRadiusCm);
			const float SpinDeg = Car->Wheels[0].SpinDeg;
			TestTrue(FString::Printf(TEXT("wrapped: %.2f in [0, 360)"), SpinDeg), SpinDeg >= 0.f && SpinDeg < 360.f);
			TestEqual(TEXT("no angle lost to the wrap"), static_cast<double>(SpinDeg), FMath::Fmod(TotalDeg, 360.0), 0.5);
		});

		It("V3: steer turns only the front wheels, positive to the right", [this]()
		{
			Car = SpawnCar();
			if (!Car || !TestEqual(TEXT("four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			const FTransform RearLeftRelativeBefore = Car->Hubs[2]->GetRelativeTransform();
			const FVector AheadOfFrontLeftBefore = GetTyreCentreWorld(0) + FVector::ForwardVector * AVehicleTwinTestActor::TyreRadiusCm;
			const FTransform FrontLeftHubWorldBefore = Car->Hubs[0]->GetComponentTransform();

			Car->UpdateWheels(0.f, 20.f, 0.f);
			const FVector AheadOfFrontLeftAfter = FollowPointOnHub(AheadOfFrontLeftBefore, FrontLeftHubWorldBefore, *Car->Hubs[0]);
			TestTrue(FString::Printf(TEXT("front of the FL tyre turned right: Y %.2f -> %.2f"), AheadOfFrontLeftBefore.Y, AheadOfFrontLeftAfter.Y),
				AheadOfFrontLeftAfter.Y > AheadOfFrontLeftBefore.Y + 1.f);
			const float TurnedDeg = FMath::RadiansToDegrees(FMath::Atan2(AheadOfFrontLeftAfter.Y - GetTyreCentreWorld(0).Y,
				AheadOfFrontLeftAfter.X - GetTyreCentreWorld(0).X));
			TestEqual(TEXT("by 20 degrees"), TurnedDeg, 20.f, 0.05f);
			TestTrue(TEXT("rear wheel not steered"), Car->Hubs[2]->GetRelativeTransform().Equals(RearLeftRelativeBefore, 1e-3));
		});

		It("V4: the tyre centre stays put while the wheel spins and steers (pivot regression, 9f64240)", [this]()
		{
			Car = SpawnCar();
			if (!Car || !TestEqual(TEXT("four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			TArray<FVector> TyreCentresBefore;
			for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
			{
				TyreCentresBefore.Add(GetTyreCentreWorld(WheelIndex));
			}
			// The tyre is offset from its hub, so a spin about the hub origin would move it in a circle.
			TestFalse(TEXT("hub origin is not the tyre centre"), Car->Hubs[0]->GetComponentLocation().Equals(TyreCentresBefore[0], 1.f));

			const float SteerAngles[] = { 0.f, 25.f, -25.f };
			for (int32 StepIndex = 0; StepIndex < 12; ++StepIndex)
			{
				Car->UpdateWheels(30.f, SteerAngles[StepIndex % 3], 0.13f);
				for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
				{
					const FVector TyreCentreNow = GetTyreCentreWorld(WheelIndex);
					if (!TyreCentreNow.Equals(TyreCentresBefore[WheelIndex], 0.05))
					{
						AddError(FString::Printf(TEXT("step %d, wheel %d: centre moved %s -> %s"), StepIndex, WheelIndex,
							*TyreCentresBefore[WheelIndex].ToCompactString(), *TyreCentreNow.ToCompactString()));
						return;
					}
				}
			}
		});

		It("V5: setting up again mid-spin returns the hubs to the authored pose with the same measurements", [this]()
		{
			Car = SpawnCar();
			if (!Car || !TestEqual(TEXT("four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			TArray<FTransform> AuthoredHubTransforms;
			for (const USceneComponent* Hub : Car->Hubs)
			{
				AuthoredHubTransforms.Add(Hub->GetRelativeTransform());
			}
			const TArray<FVehicleTwinWheel> WheelsBefore = Car->Wheels;

			Car->UpdateWheels(50.f, 15.f, 0.37f);
			TestFalse(TEXT("FL is mid-spin"), Car->Hubs[0]->GetRelativeTransform().Equals(AuthoredHubTransforms[0], 1e-3));

			Car->SetUpWheels(false);
			if (!TestEqual(TEXT("still four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
			{
				const FVehicleTwinWheel& Wheel = Car->Wheels[WheelIndex];
				const FString WheelName = Wheel.WheelTag.ToString();
				TestTrue(FString::Printf(TEXT("%s back to the authored pose"), *WheelName),
					Car->Hubs[WheelIndex]->GetRelativeTransform().Equals(AuthoredHubTransforms[WheelIndex], 1e-3));
				TestTrue(FString::Printf(TEXT("%s base transform is the authored one"), *WheelName),
					Wheel.BaseRelativeTransform.Equals(AuthoredHubTransforms[WheelIndex], 1e-3));
				TestEqual(FString::Printf(TEXT("%s same radius"), *WheelName), Wheel.RadiusCm, WheelsBefore[WheelIndex].RadiusCm, 0.01f);
				TestTrue(FString::Printf(TEXT("%s same centre"), *WheelName),
					Wheel.WheelCentreInHubSpace.Equals(WheelsBefore[WheelIndex].WheelCentreInHubSpace, 0.01));
				TestEqual(FString::Printf(TEXT("%s spin starts at 0"), *WheelName), Wheel.SpinDeg, 0.f);
			}
		});

		It("V6: tyre radius comes from the vertices; without vertex data it falls back to the bounds", [this]()
		{
			Car = SpawnCar();
			if (!Car || !TestEqual(TEXT("four wheels"), Car->GetNumWheels(), 4))
			{
				return;
			}
			for (const FVehicleTwinWheel& Wheel : Car->Wheels)
			{
				TestEqual(FString::Printf(TEXT("%s radius"), *Wheel.WheelTag.ToString()), Wheel.RadiusCm, AVehicleTwinTestActor::TyreRadiusCm, 0.01f);
			}
			const FVehicleTwinWheel& FrontLeft = Car->Wheels[0];
			bool bFromVertices = false;
			AVehicleTwinActor::MeasureTyreRadius(Car->Tyres[0], FrontLeft.Hub->GetComponentTransform(), FrontLeft.WheelCentreInHubSpace,
				FrontLeft.SpinAxis, bFromVertices);
			TestTrue(TEXT("measured from vertices"), bFromVertices);

			// Not a static mesh: no vertices to read, so the bounding sphere is used.
			USphereComponent* RoundPart = NewObject<USphereComponent>(Car);
			RoundPart->SetupAttachment(FrontLeft.Hub);
			RoundPart->SetSphereRadius(40.f);
			RoundPart->RegisterComponent();
			const float FallbackRadiusCm = AVehicleTwinActor::MeasureTyreRadius(RoundPart, FrontLeft.Hub->GetComponentTransform(),
				FVector::ZeroVector, FrontLeft.SpinAxis, bFromVertices);
			TestFalse(TEXT("fallback: not from vertices"), bFromVertices);
			TestEqual(TEXT("fallback: bounding sphere radius"), FallbackRadiusCm, 40.f, 0.01f);
		});
	});

	Describe("Status", [this]()
	{
		It("V7: status colour clamps the blend, scales it per group, leaves other slots alone and never stacks instances", [this]()
		{
			// The glass mesh joins the Paint group with a material that has no StatusColor.
			AddExpectedError(TEXT("have no material with a StatusColor parameter"), EAutomationExpectedErrorFlags::Contains, 1, false);
			Car = SpawnCar([](AVehicleTwinTestActor& NewCar)
			{
				NewCar.GlassMesh->ComponentTags.Add(TEXT("Paint"));
			});
			if (!Car)
			{
				return;
			}
			UMaterialInstanceDynamic* BodyMaterial = Cast<UMaterialInstanceDynamic>(Car->BodyPaintMesh->GetMaterial(0));
			UMaterialInstanceDynamic* TrimMaterial = Cast<UMaterialInstanceDynamic>(Car->TrimMesh->GetMaterial(0));
			TestNotNull(TEXT("body slot has a dynamic instance"), BodyMaterial);
			TestNotNull(TEXT("trim slot has a dynamic instance"), TrimMaterial);
			TestNull(TEXT("glass slot untouched"), Cast<UMaterialInstanceDynamic>(Car->GlassMesh->GetMaterial(0)));
			if (!BodyMaterial || !TrimMaterial)
			{
				return;
			}
			TestEqual(TEXT("two paint materials (body, trim)"), Car->PaintMaterials.Num(), 2);

			const FName StatusBlend = TEXT("StatusBlend");
			Car->SetStatusColor(FLinearColor::Red, 2.f);
			TestEqual(TEXT("blend 2 clamps to 1 on the body"), GetScalarParameter(BodyMaterial, StatusBlend), 1.f, 1e-4f);
			TestEqual(TEXT("trim group scale 0.5"), GetScalarParameter(TrimMaterial, StatusBlend), 0.5f, 1e-4f);
			FLinearColor BodyStatusColor = FLinearColor::Black;
			BodyMaterial->GetVectorParameterValue(FHashedMaterialParameterInfo(FName(TEXT("StatusColor"))), BodyStatusColor);
			TestTrue(TEXT("body status colour is red"), BodyStatusColor.Equals(FLinearColor::Red));
			Car->SetStatusColor(FLinearColor::Red, -1.f);
			TestEqual(TEXT("blend -1 clamps to 0"), GetScalarParameter(BodyMaterial, StatusBlend), 0.f, 1e-4f);

			// Running material setup again (construction reruns on every edit) reuses the instances.
			Car->CreatePaintMaterials(false);
			Car->SetStatusColor(FLinearColor::Yellow, 1.f);
			TestTrue(TEXT("same body instance"), Car->BodyPaintMesh->GetMaterial(0) == BodyMaterial);
			TestTrue(TEXT("same trim instance"), Car->TrimMesh->GetMaterial(0) == TrimMaterial);
			TestEqual(TEXT("still two paint materials"), Car->PaintMaterials.Num(), 2);
		});

		It("V8: Warning and Critical put every mesh (child actors too) in the outline stencil, Normal takes them out, a repeat touches nothing", [this]()
		{
			UMaterialInterface* Outline = LoadObject<UMaterialInterface>(nullptr, VehicleTwinActorTestAssets::OutlineMaterial);
			if (!TestNotNull(TEXT("PP_VehicleStatusOutline loads"), Outline))
			{
				return;
			}
			Car = SpawnCar([Outline](AVehicleTwinTestActor& NewCar)
			{
				NewCar.SetStatusOutlineMaterialForTest(Outline);
			});
			if (!Car)
			{
				return;
			}
			TInlineComponentArray<UPrimitiveComponent*> CarMeshes(Car, /*bIncludeFromChildActors*/ true);
			const AActor* HarvestedActor = Car->HarvestedPart->GetChildActor();
			const bool bHasChildActorMesh = HarvestedActor && CarMeshes.ContainsByPredicate(
				[HarvestedActor](const UPrimitiveComponent* Mesh) { return Mesh->GetOwner() == HarvestedActor; });
			TestTrue(TEXT("the child actor's mesh is part of the car"), bHasChildActorMesh);

			auto CountMeshesInOutline = [&CarMeshes]()
			{
				int32 OutlinedMeshCount = 0;
				for (const UPrimitiveComponent* Mesh : CarMeshes)
				{
					OutlinedMeshCount += Mesh->bRenderCustomDepth && Mesh->CustomDepthStencilValue == 1 ? 1 : 0;
				}
				return OutlinedMeshCount;
			};
			TestEqual(TEXT("Normal at start: none"), CountMeshesInOutline(), 0);

			Car->SetStatusOutline(EVehicleStatus::Warning);
			TestEqual(TEXT("Warning: all"), CountMeshesInOutline(), CarMeshes.Num());
			Car->SetStatusOutline(EVehicleStatus::Critical);
			TestEqual(TEXT("Critical: all"), CountMeshesInOutline(), CarMeshes.Num());

			// Same status again must not touch the meshes: a mesh changed by hand stays as it is.
			Car->BodyPaintMesh->SetRenderCustomDepth(false);
			Car->SetStatusOutline(EVehicleStatus::Critical);
			TestFalse(TEXT("repeat Critical left the body alone"), Car->BodyPaintMesh->bRenderCustomDepth);

			Car->SetStatusOutline(EVehicleStatus::Normal);
			TestEqual(TEXT("Normal: none"), CountMeshesInOutline(), 0);

			// Without an outline material nothing could draw the outline, so the custom depth pass isn't turned on either.
			AVehicleTwinTestActor* CarWithoutOutline = SpawnCar();
			if (CarWithoutOutline)
			{
				CarWithoutOutline->SetStatusOutline(EVehicleStatus::Critical);
				TestFalse(TEXT("no outline material: body not in custom depth"), CarWithoutOutline->BodyPaintMesh->bRenderCustomDepth);
			}
		});
	});

	Describe("Telemetry", [this]()
	{
		It("V9: blend goes 0 to 1 over the arrival interval, a trip loop snaps, and several samples in one frame keep the last real interval", [this]()
		{
			// Previous sample at trip time 1.0 s, latest at 1.1 s, arriving 0.1 s apart.
			TestEqual(TEXT("just arrived: 0"), AVehicleTwinActor::ComputeTelemetryBlendAlpha(1.0, 1.1, 0.0, 0.1), 0.f, 1e-5f);
			TestEqual(TEXT("halfway: 0.5"), AVehicleTwinActor::ComputeTelemetryBlendAlpha(1.0, 1.1, 0.05, 0.1), 0.5f, 1e-5f);
			TestEqual(TEXT("late: held at 1"), AVehicleTwinActor::ComputeTelemetryBlendAlpha(1.0, 1.1, 0.3, 0.1), 1.f, 1e-5f);
			TestEqual(TEXT("trip loop 189.9 -> 0: snaps to latest"), AVehicleTwinActor::ComputeTelemetryBlendAlpha(189.9, 0.0, 0.0, 0.1), 1.f, 1e-5f);
			TestEqual(TEXT("no interval: latest"), AVehicleTwinActor::ComputeTelemetryBlendAlpha(1.0, 1.1, 0.0, 0.0), 1.f, 1e-5f);

			Car = SpawnCar();
			if (!Car)
			{
				return;
			}
			// Arrival times come from the world clock, set by hand here.
			auto ArriveAt = [this](double WorldTimeS)
			{
				TestWorld->TimeSeconds = WorldTimeS;
				Car->HandleTelemetrySampleReceived(FVehicleTelemetry());
			};
			ArriveAt(10.0);
			TestEqual(TEXT("first sample keeps the default interval"), Car->SampleArrivalIntervalS, 0.1, 1e-9);
			ArriveAt(10.25);
			TestEqual(TEXT("second sample 0.25 s later"), Car->SampleArrivalIntervalS, 0.25, 1e-9);
			ArriveAt(10.35);
			ArriveAt(10.35);
			ArriveAt(10.35);
			TestEqual(TEXT("three in one frame keep the last real interval"), Car->SampleArrivalIntervalS, 0.1, 1e-9);
			TestEqual(TEXT("latest arrival time"), Car->LatestSampleArrivalTimeS, 10.35, 1e-9);
		});
	});

	Describe("Callout anchors", [this]()
	{
		It("V10: a wheel tag gives the tyre centre, another tag its component, an unknown tag nothing", [this]()
		{
			Car = SpawnCar();
			if (!Car)
			{
				return;
			}
			FVector AnchorWorldLocation;
			TestTrue(TEXT("Wheel.FR found"), Car->FindCalloutAnchorWorldLocation(TEXT("Wheel.FR"), AnchorWorldLocation));
			TestTrue(TEXT("Wheel.FR = FR tyre centre, not the hub"), AnchorWorldLocation.Equals(GetTyreCentreWorld(1), 0.05));

			Car->UpdateWheels(40.f, 10.f, 0.2f);
			TestTrue(TEXT("Wheel.FL still found while spinning"), Car->FindCalloutAnchorWorldLocation(TEXT("Wheel.FL"), AnchorWorldLocation));
			TestTrue(TEXT("and still at the FL tyre centre"), AnchorWorldLocation.Equals(GetTyreCentreWorld(0), 0.05));

			TestTrue(TEXT("CalloutAnchor.Hood found"), Car->FindCalloutAnchorWorldLocation(TEXT("CalloutAnchor.Hood"), AnchorWorldLocation));
			TestTrue(TEXT("at the hood anchor"), AnchorWorldLocation.Equals(Car->HoodAnchor->GetComponentLocation(), 0.01));

			TestFalse(TEXT("unknown tag: false"), Car->FindCalloutAnchorWorldLocation(TEXT("CalloutAnchor.Missing"), AnchorWorldLocation));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
