#include "LocationMarker.h"

#include "Settings.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"

DEFINE_LOG_CATEGORY(LogLocationMarker);

ALocationMarker::ALocationMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Create mesh components
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = StaticMeshComp;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
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

	CesiumGlobeAnchor = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("CesiumGlobeAnchor"));
}

bool ALocationMarker::ToggleSelection()
{
	Selected = !Selected;
	if (Selected) SetColor(FColor::Red);
	else SetColor(BaseColor);
	UE_LOG(LogLocationMarker, Display, TEXT("%s: %s"), Selected ? TEXT("Selected") : TEXT("Unselected"), *ToString());
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
	Args.Add(FStringFormatArg(GetClass()->GetName()));
	if (!DeviceID.IsEmpty()) Args.Add(FStringFormatArg(DeviceID));
	else Args.Add(FStringFormatArg(FString("")));

	Args.Add(FStringFormatArg(LocationTs.ToString()));

	if (Selected) Args.Add(FString("True"));
	else Args.Add(FString("False"));
	
	return FString::Format(TEXT("{0}(DeviceID='{1}' LocationTs={2}, Selected={3})"), Args);
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
	JsonObject->SetStringField(MarkerTypeAttributeName, UEnum::GetValueAsName(this->MarkerType).ToString());
	return JsonObject;
}

void ALocationMarker::InitializeParams(const FString ParamDeviceID, const FLocationTs ParamLocationTs)
{
	this->DeviceID = ParamDeviceID;
	this->LocationTs = ParamLocationTs;
	this->Initialized = true;
}

void ALocationMarker::BeginPlay()
{
	Super::BeginPlay();
	if (this->Initialized) UE_LOG(LogLocationMarker, Display, TEXT("BeginPlay: %s"), *LocationTs.ToString());
}

void ALocationMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (EndPlayReason == EEndPlayReason::Destroyed && MarkerOnDelete.IsBound()) MarkerOnDelete.Execute(DeviceID, LocationTs.Timestamp, DeleteFromDBOnDestroy);
	UE_LOG(LogLocationMarker, Display, TEXT("EndPlay: %s - %s"), *UEnum::GetValueAsName(EndPlayReason).ToString(), *LocationTs.ToString());
}