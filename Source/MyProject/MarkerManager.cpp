#include "MarkerManager.h"

#include <cstdlib>

#include "DynamicMarker.h"
#include "Components/BoxComponent.h"
#include "LocationMarker.h"
#include "JsonObjectConverter.h"
#include "HTTP.h"
#include "TemporaryMarker.h"
#include "Settings.h"
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/dynamodb/model/GetItemRequest.h"
#include "aws/dynamodb/model/QueryRequest.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "Misc/DefaultValueHelper.h"


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
	
	InitAPI(Aws::SDKOptions());
	Aws::Auth::AWSCredentials Creadentials = Aws::Auth::AWSCredentials(AWSAccessKeyId, AWSSecretKey);
	Aws::Client::ClientConfiguration Config = Aws::Client::ClientConfiguration();
	Config.region = REGION;
	ClientRef = new Aws::DynamoDB::DynamoDBClient(Creadentials, Config);
	
	// GetWorld()->GetTimerManager()
	// 	.SetTimer(TimerHandle, this, &AMarkerManager::RepeatingFunction, 1.0f, true, 1.0f);
}

// void AMarkerManager::RepeatingFunction()
// {
// 	for(const auto &Pair: AllMarkers)
// 	{
// 		FVector LatestLoc = GetLatestRecord(Pair.Value->DeviceID, Pair.Value->Timestamp);
// 		if (LatestLoc != FVector::ZeroVector && LatestLoc != Pair.Value->Coordinate)
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("new: %s old: %s"), *LatestLoc.ToString(), *Pair.Value->Coordinate.ToString());
// 			Pair.Value->SetActorLocation(LatestLoc, true, nullptr, ETeleportType::None);
// 		}
// 	}
// }

// Called every frame
void AMarkerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FVector AMarkerManager::GetLatestRecord(FString TargetDeviceID, FDateTime LastKnownTimestamp)
{
	Aws::DynamoDB::Model::QueryRequest Request;
	Request.SetTableName(DynamoDBTableName);

	// Set query key condition expression
	Aws::String PartitionKeyName = Aws::String("device_id");
	Aws::String PartitionKeyVal = Aws::String(TCHAR_TO_UTF8(*TargetDeviceID));
	Request.SetKeyConditionExpression(PartitionKeyName + "= :valueToMatch");

	// Set Expression AttributeValues
	Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> AttributeValues;
	AttributeValues.emplace(":valueToMatch", PartitionKeyVal);
	Request.SetExpressionAttributeValues(AttributeValues);

	FVector LatestLoc = FVector::ZeroVector;

	// Perform Query operation
	const Aws::DynamoDB::Model::QueryOutcome& result = ClientRef->Query(Request);
	if (result.IsSuccess())
	{
		// Reference the retrieved items
		const Aws::Vector<Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>>& items = result.GetResult().GetItems();
		UE_LOG(LogTemp, Warning, TEXT("Number of items retrieved from Query: %d"), items.size());
		
		FDateTime LatestRecordTimestamp = LastKnownTimestamp;
		
		for(const auto &item: items)
		{
			// get name
			const FString DeviceID = item.at("device_id").GetS().c_str();
			
			// get timestamp data
			Aws::DynamoDB::Model::AttributeValue TimestampValue = item.at("created_timestamp");
			FDateTime Timestamp = FDateTime::FromUnixTimestamp(std::atoi(TimestampValue.GetS().c_str()));
			UE_LOG(LogTemp, Warning, TEXT("Timestamp: %d"), Timestamp.ToUnixTimestamp());
			
			// get location data
			float lon, lat, elev;
			FDefaultValueHelper::ParseFloat(FString(item.at("longitude").GetS().c_str()), lon);
			FDefaultValueHelper::ParseFloat(FString(item.at("latitude").GetS().c_str()), lat);
			FDefaultValueHelper::ParseFloat(FString(item.at("elevation").GetS().c_str()), elev);
			const FVector CurrLocation = FVector(lon, lat, elev);
			UE_LOG(LogTemp, Warning, TEXT("Device ID: %s Location: %s"), *DeviceID, *CurrLocation.ToString());
			
			// hold on to the latest location
			UE_LOG(LogTemp, Warning, TEXT("%s %s"), *Timestamp.ToIso8601(), *LatestRecordTimestamp.ToIso8601());
			if (Timestamp > LatestRecordTimestamp)
			{
				UE_LOG(LogTemp, Warning, TEXT("AAAAA"));
				LatestLoc = CurrLocation;
			};
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to query items: %s"), *FString(result.GetError().GetMessage().c_str()));
	}
	return LatestLoc;
}

ALocationMarker* AMarkerManager::CreateMarkerFromJsonObject(const FJsonObject* JsonObject)
{
	const FDateTime Timestamp = FDateTime::FromUnixTimestamp(FCString::Atoi(*JsonObject->GetStringField("created_timestamp")));
	FString MarkerTypeStr;
	ELocationMarkerType MarkerType = ELocationMarkerType::Static;
	
	if (JsonObject->TryGetStringField("marker_type", MarkerTypeStr))
	{
		UE_LOG(LogTemp, Warning, TEXT("Marker type Field: %s"), *MarkerTypeStr);
		if (MarkerTypeStr == FString("dynamic"))
		{
			MarkerType = ELocationMarkerType::Dynamic;
		}
	}
	return CreateMarker(FVector(
		JsonObject->GetNumberField("longitude"),
		JsonObject->GetNumberField("latitude"),
		JsonObject->GetNumberField("elevation")),
		Timestamp,
		JsonObject->GetStringField("device_id"),
		MarkerType);
}

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation, const ELocationMarkerType MarkerType)
{
	return CreateMarker(SpawnLocation, FDateTime::Now(), FString("UE"), MarkerType);
}

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID, const ELocationMarkerType MarkerType)
{
	ALocationMarker* Marker = nullptr;
	if (!AllMarkers.Contains(GetTypeHash(SpawnLocation, Timestamp, DeviceID)))
	{
		AActor* MarkerActor;

		switch (MarkerType)
		{
			case ELocationMarkerType::Static:
				MarkerActor = GetWorld()->SpawnActor<ALocationMarker>(SpawnLocation, FRotator::ZeroRotator);
				break;
			case ELocationMarkerType::Temporary:
				MarkerActor = GetWorld()->SpawnActor<ATemporaryMarker>(SpawnLocation, FRotator::ZeroRotator);
				break;
			case ELocationMarkerType::Dynamic:
				MarkerActor = GetWorld()->SpawnActor<ADynamicMarker>(SpawnLocation, FRotator::ZeroRotator);
				ADynamicMarker* DynamicMarker = Cast<ADynamicMarker>(MarkerActor);
				DynamicMarker->ManagerDelegate.BindUFunction(this, FName("GetLatestRecord"));
				break;
		}
		
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
		
		AllMarkers.Add(GetTypeHash(Marker), Marker);
		UE_LOG(LogTemp, Warning, TEXT("%d data points stored"), AllMarkers.Num());
		
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
				ALocationMarker* Marker = CreateMarkerFromJsonObject(MarkerValue->AsObject().Get());
				if (Marker != nullptr) UE_LOG(LogTemp, Log, TEXT("Created Marker: %s %s"), *Marker->GetName(), *Marker->ToString());
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