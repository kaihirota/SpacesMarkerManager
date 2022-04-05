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
	SphereComp->InitSphereRadius(200.0f);
	SphereComp->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	if (SphereMeshAsset.Succeeded())
	{
		StaticMeshComp->SetStaticMesh(SphereMeshAsset.Object);
		StaticMeshComp->SetSimulatePhysics(false);
		StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		StaticMeshComp->SetCollisionProfileName("Marker");
		StaticMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		StaticMeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	};

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveGreen(TEXT("Material'/Game/FirstPerson/EmissiveGreen.EmissiveGreen'"));
	if (EmissiveGreen.Succeeded())
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(EmissiveGreen.Object, nullptr);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FColor::Green);
		DynamicMaterial->SetCastShadowAsMasked(false);
		StaticMeshComp->SetMaterial(0, DynamicMaterial);
		SetColor(BaseColor);
	}
}

void ALocationMarker::ToggleSelection()
{
	Selected = !Selected;
	if (Selected) SetColor(FColor::Red);
	else SetColor(BaseColor);
	UE_LOG(LogTemp, Log, TEXT("%s: %s - %s"), Selected ? TEXT("Selected") : TEXT("Unselected"), *GetName(), *ToString());
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

void ALocationMarker::SetOpacity(const float Val) const
{
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Val);
}

float ALocationMarker::GetOpacity() const
{
	float Opacity;
	DynamicMaterial->GetScalarParameterValue(TEXT("Opacity"), Opacity);
	return Opacity; 
}

FString ALocationMarker::ToString() const
{
	TArray< FStringFormatArg > Args;
	if (!DeviceID.IsEmpty()) Args.Add(FStringFormatArg(DeviceID));
	else Args.Add(FStringFormatArg(FString("")));
	Args.Add(FStringFormatArg(Coordinate.ToString()));
	Args.Add(FStringFormatArg(Timestamp.ToIso8601()));
	return FString::Format( TEXT( "LocationMarker(DeviceID='{0}' Coordinate=({1}), Timestamp='{2}')" ), Args);
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
	JsonObject->SetStringField(SortKeyAttributeName, FString::FromInt(Timestamp.ToUnixTimestamp()));
	JsonObject->SetStringField(PositionXAttributeName, FString::SanitizeFloat(this->Coordinate.X));
	JsonObject->SetStringField(PositionYAttributeName, FString::SanitizeFloat(this->Coordinate.Y));
	JsonObject->SetStringField(PositionZAttributeName, FString::SanitizeFloat(this->Coordinate.Z));
	return JsonObject;
}

// Called when the game starts or when spawned
void ALocationMarker::BeginPlay()
{
	Super::BeginPlay();
}

void ALocationMarker::BeginDestroy()
{
	Super::BeginDestroy();
}

// Called every frame
void ALocationMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}
