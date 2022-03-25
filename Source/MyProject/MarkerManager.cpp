#include "MarkerManager.h"
#include "Components/BoxComponent.h"
#include "LocationMarker.h"
#include "JsonObjectConverter.h"
#include "HTTP.h"
#include "TemporaryMarker.h"
#include "Settings.h"

// Sets default values
AMarkerManager::AMarkerManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	SpawnVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnVolume"));
	SpawnVolume->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

// Called when the game starts or when spawned
void AMarkerManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMarkerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

ALocationMarker* AMarkerManager::CreateMarkerFromJsonObject(const FJsonObject* JsonObject)
{
	const FDateTime Timestamp = FDateTime::FromUnixTimestamp(JsonObject->GetNumberField("created_timestamp"));
	return CreateMarker(FVector(
		JsonObject->GetNumberField("longitude"),
		JsonObject->GetNumberField("latitude"),
		JsonObject->GetNumberField("elevation")),
		Timestamp,
		JsonObject->GetStringField("device_id"));
}

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation)
{
	return CreateMarker(SpawnLocation, false);
}

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation, const bool Temporary)
{
	return CreateMarker(SpawnLocation, FDateTime::Now(), FString("UE"), Temporary);
}

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID)
{
	return CreateMarker(SpawnLocation, Timestamp, DeviceID, false);
}

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID, const bool Temporary)
{
	ALocationMarker* Marker = nullptr;
	if (!AllMarkers.Contains(GetTypeHash(SpawnLocation, Timestamp, DeviceID)))
	{
		AActor* MarkerActor;
		if (Temporary) MarkerActor = GetWorld()->SpawnActor<ATemporaryMarker>(SpawnLocation, FRotator::ZeroRotator);
		else MarkerActor = GetWorld()->SpawnActor<ALocationMarker>(SpawnLocation, FRotator::ZeroRotator);
		Marker = Cast<ALocationMarker>(MarkerActor);
		if (Marker != nullptr)
		{
			// TODO convert UE coord to WGS84
			Marker->Coordinate = SpawnLocation;
			Marker->Timestamp = Timestamp;
			Marker->DeviceID = DeviceID;

			UE_LOG(LogTemp, Warning, TEXT("Created %s - %s"), *Marker->GetName() , *Marker->ToString());
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Marker->ToJsonString());
		}

		// AllMarkers.Add(Marker->DeviceID, Marker);
		AllMarkers.Add(GetTypeHash(Marker), Marker);
		UE_LOG(LogTemp, Warning, TEXT("%d data points stored"), AllMarkers.Num());
		
		CreateMarkerInDB(Marker);
		// for(const auto &Pair: AllMarkers)
		// {
		// 	UE_LOG(LogTemp, Warning, TEXT("key: %s value: %s"), *Pair.Key, *Pair.Value->ToString());
		// }
	}
	return Marker;
}

bool AMarkerManager::CreateMarkerInDB(ALocationMarker* Marker)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (Http != nullptr)
	{
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		// Request->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetMarkersResponse);
		Request->SetURL(MarkerAPIEndpoint);
		Request->SetVerb("POST");
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		Request->SetHeader("Content-Type", TEXT("application/json"));
		Request->SetHeader(TEXT("Accepts"), TEXT("application/json"));
		Request->SetContentAsString(Marker->ToJsonString());
		Request->ProcessRequest();
		UE_LOG(LogTemp, Warning, TEXT("Created POST Markers request successfully."));
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("Failed to create POST request"));
	return false;
}

void AMarkerManager::GetMarkersFromDB()
{
	FHttpModule* Http = &FHttpModule::Get();
	if (Http != nullptr)
	{
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		Request->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetMarkersResponse);
		Request->SetURL(MarkerAPIEndpoint);
		Request->SetVerb("GET");
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		Request->SetHeader("Content-Type", "application/json");
		Request->ProcessRequest();
		UE_LOG(LogTemp, Log, TEXT("Created GET Markers request successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create GET request"));
	}
}

void AMarkerManager::OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	TSharedPtr<FJsonObject> JsonObject;
	if(Response->GetResponseCode() == 200 && bWasSuccessful)
	{
		const FString ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
		if(FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TArray<TSharedPtr<FJsonValue>> MarkerArray = JsonObject->GetArrayField("data");
			UE_LOG(LogTemp, Warning, TEXT("Fetch Success: %d items"), MarkerArray.Num());
			for(const TSharedPtr<FJsonValue>& MarkerValue : MarkerArray)
			{
				// how to determine if new marker should be spawned, or not?
				// no marker with same device id, time, and coordinates can co exist
				const ALocationMarker* Marker = CreateMarkerFromJsonObject(MarkerValue->AsObject().Get());
				if (Marker != nullptr) UE_LOG(LogTemp, Log, TEXT("Created Marker: %s"), *Marker->ToString());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Error %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
	}
}

void AMarkerManager::DeleteMarker(ALocationMarker* Marker, bool DeleteFromDB)
{
	if (Marker != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Destroying: %s - %s"), *Marker->GetName(), *Marker->ToString());
		Marker->Destroy();

		if (DeleteFromDB)
		{
			DeleteMarkerFromDB(Marker);
		} 
		AllMarkers.FindAndRemoveChecked(GetTypeHash(Marker));
	}
}

void AMarkerManager::DeleteMarkers(const bool DeleteFromDB)
{
	for(const auto &Pair: AllMarkers)
	{
		ALocationMarker* Marker = Pair.Value;
		if (Marker != nullptr)
		{
			DeleteMarker(Marker, DeleteFromDB);
		}
	}
}

void AMarkerManager::DeleteSelectedMarkers(bool DeleteFromDB)
{
	for (auto It = AllMarkers.CreateIterator(); It; ++It)
	{
		ALocationMarker* Marker = It.Value();
		if (Marker != nullptr)
		{
			if (Marker->Selected)
			{
				UE_LOG(LogTemp, Warning, TEXT("Destroying: %s - %s"), *Marker->GetName(), *Marker->ToString());
				DeleteMarkerFromDB(Marker);
				if (DeleteFromDB) Marker->Destroy();
				It.RemoveCurrent();
			} 
		}
	}
}

void AMarkerManager::DeleteMarkerFromDB(ALocationMarker* Marker)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (Http != nullptr)
	{
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
		Request->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnDeleteMarkersResponse);
		Request->SetURL(MarkerAPIEndpoint);
		Request->SetVerb("DELETE");
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		Request->SetHeader("Content-Type", "application/json");
		Request->SetContentAsString(Marker->ToJsonString());
		Request->ProcessRequest();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to delete marker: %s"), *Marker->ToJsonString());
	}
}

void AMarkerManager::OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if(Response->GetResponseCode() != 200 || !bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not delete marker - HTTP %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("SUCCESS: Delete marker - HTTP %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
	}
}