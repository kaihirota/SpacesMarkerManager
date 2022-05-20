#include "LocationMarker.h"

#include "Settings.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"

ALocationMarker::ALocationMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create mesh components
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = StaticMeshComp;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Marker"));
	SphereComp->InitSphereRadius(DefaultRadius);
	SphereComp->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
		TEXT("StaticMesh'/SpacesMarkerManager/Sphere.Sphere'"));
	if (SphereMeshAsset.Succeeded())
	{
		StaticMeshComp->SetStaticMesh(SphereMeshAsset.Object);
		StaticMeshComp->SetSimulatePhysics(false);
		StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		StaticMeshComp->SetCollisionProfileName("Marker");
		StaticMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMaterialInstance(TEXT("MaterialInstanceConstant'/SpacesMarkerManager/EmissiveMaterial_Inst.EmissiveMaterial_Inst'"));
	if (EmissiveMaterialInstance.Succeeded())
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(EmissiveMaterialInstance.Object, nullptr);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FColor::Green);
		DynamicMaterial->SetCastShadowAsMasked(false);
		StaticMeshComp->SetMaterial(0, DynamicMaterial);
		SetColor(BaseColor);
	}
}

bool ALocationMarker::ToggleSelection()
{
	Selected = !Selected;
	if (Selected) SetColor(FColor::Red);
	else SetColor(BaseColor);
	UE_LOG(LogTemp, Log, TEXT("%s: %s"), Selected ? TEXT("Selected") : TEXT("Unselected"), *ToString());
	return Selected;
}

void ALocationMarker::SetColor(const FLinearColor Color) const
{
	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
}

FLinearColor ALocationMarker::GetColor() const
{
	FLinearColor Color;
	DynamicMaterial->GetVectorParameterValue(TEXT("Color"), Color);
	return Color;
}

void ALocationMarker::SetOpacity(const float OpacityVal) const
{
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), OpacityVal);
}

float ALocationMarker::GetOpacity() const
{
	float Opacity;
	DynamicMaterial->GetScalarParameterValue(TEXT("Opacity"), Opacity);
	return Opacity;
}

FString ALocationMarker::ToString() const
{
	TArray<FStringFormatArg> Args;
	Args.Add(FStringFormatArg(*ClassName));
	if (!DeviceID.IsEmpty()) Args.Add(FStringFormatArg(DeviceID));
	else Args.Add(FStringFormatArg(FString("")));

	Args.Add(FStringFormatArg(LocationTs.UECoordinate.ToString()));
	Args.Add(FStringFormatArg(LocationTs.Timestamp.ToIso8601()));

	if (Selected) Args.Add(FString("True"));
	else Args.Add(FString("False"));
	
	return FString::Format(TEXT("{0}(DeviceID='{1}' Coordinate=({2}), Timestamp='{3}', Selected='{4}')"), Args);
}

FString ALocationMarker::ToJsonString() const
{
	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ToJsonObject(), Writer);
	return OutputString;
}

TSharedRef<FJsonObject> ALocationMarker::ToJsonObject() const
{
	const TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	if (!DeviceID.IsEmpty()) JsonObject->SetStringField(PartitionKeyAttributeName, this->DeviceID);
	else JsonObject->SetStringField(PartitionKeyAttributeName, FString(""));
	JsonObject->SetStringField(SortKeyAttributeName, FString::FromInt(LocationTs.Timestamp.ToUnixTimestamp()));
	JsonObject->SetStringField(PositionXAttributeName, FString::SanitizeFloat(LocationTs.UECoordinate.X));
	JsonObject->SetStringField(PositionYAttributeName, FString::SanitizeFloat(LocationTs.UECoordinate.Y));
	JsonObject->SetStringField(PositionZAttributeName, FString::SanitizeFloat(LocationTs.UECoordinate.Z));
	return JsonObject;
}

// Called when the game starts or when spawned
void ALocationMarker::BeginPlay()
{
	Super::BeginPlay();
}

void ALocationMarker::BeginDestroy()
{
	UE_LOG(LogTemp, Warning, TEXT("BeginDestroy: %s"), *DeviceID);
	Super::BeginDestroy();
	if (MarkerOnDelete.IsBound()) MarkerOnDelete.Execute(DeviceID, LocationTs.Timestamp, DeleteFromDBOnDestroy);
}

// Called every frame
void ALocationMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}
