#include "Tests/VehicleTwinTestActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"

const FVector AVehicleTwinTestActor::TyreOffsetInHubSpace(6.f, 0.f, -4.f);

AVehicleTwinTestActor::AVehicleTwinTestActor()
{
	// No telemetry subsystem in the test world, and the tests drive the wheels themselves.
	bDriveFromTelemetry = false;
	PaintGroups[1].StatusBlendScale = 0.5f;

	USceneComponent* CarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CarRoot"));
	SetRootComponent(CarRoot);

	// FL, FR, RL, RR: left is -Y, front is +X.
	const TCHAR* const WheelNames[] = { TEXT("FL"), TEXT("FR"), TEXT("RL"), TEXT("RR") };
	const FVector HubLocations[] = { FVector(130.f, -75.f, 40.f), FVector(130.f, 75.f, 40.f), FVector(-130.f, -75.f, 40.f), FVector(-130.f, 75.f, 40.f) };
	for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
	{
		USceneComponent* Hub = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("WheelHub_%s"), WheelNames[WheelIndex]));
		Hub->SetupAttachment(CarRoot);
		Hub->SetRelativeLocation(HubLocations[WheelIndex]);
		Hub->ComponentTags.Add(*FString::Printf(TEXT("Wheel.%s"), WheelNames[WheelIndex]));
		Hubs.Add(Hub);

		// The engine cylinder is 100 cm across and tall along Z: rolled 90° so its axis is the axle (Y), scaled to the tyre radius
		// and 30 cm wide.
		UStaticMeshComponent* Tyre = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Tyre_%s"), WheelNames[WheelIndex]));
		Tyre->SetupAttachment(Hub);
		Tyre->SetRelativeLocation(TyreOffsetInHubSpace);
		Tyre->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		Tyre->SetRelativeScale3D(FVector(TyreRadiusCm / 50.f, TyreRadiusCm / 50.f, 0.3f));
		Tyre->ComponentTags.Add(TEXT("Tyre"));
		Tyres.Add(Tyre);
	}

	BodyPaintMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyPaint"));
	BodyPaintMesh->SetupAttachment(CarRoot);
	BodyPaintMesh->ComponentTags.Add(TEXT("Paint"));

	TrimMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trim"));
	TrimMesh->SetupAttachment(CarRoot);
	TrimMesh->ComponentTags.Add(TEXT("Trim"));

	GlassMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glass"));
	GlassMesh->SetupAttachment(CarRoot);

	HoodAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("HoodAnchor"));
	HoodAnchor->SetupAttachment(CarRoot);
	HoodAnchor->SetRelativeLocation(FVector(150.f, 0.f, 110.f));
	HoodAnchor->ComponentTags.Add(TEXT("CalloutAnchor.Hood"));

	HarvestedPart = CreateDefaultSubobject<UChildActorComponent>(TEXT("HarvestedPart"));
	HarvestedPart->SetupAttachment(CarRoot);
	HarvestedPart->SetChildActorClass(AStaticMeshActor::StaticClass());
}
