// Fill out your copyright notice in the Description page of Project Settings.


#include "MojexaSpacesMarkerManager.h"

#include "DynamicMarker.h"
#include "HttpModule.h"
#include "Settings.h"
#include "TemporaryMarker.h"
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/core/client/ClientConfiguration.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodb/model/PutItemRequest.h"
#include "aws/dynamodb/model/QueryRequest.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DefaultValueHelper.h"

UMojexaSpacesMarkerManager::UMojexaSpacesMarkerManager(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
}

void UMojexaSpacesMarkerManager::Init()
{
	Super::Init();
	UE_LOG(LogTemp, Warning, TEXT("Game instance initializing"));
	Aws::InitAPI(Aws::SDKOptions());
	const Aws::Auth::AWSCredentials Credentials = Aws::Auth::AWSCredentials(AWSAccessKeyId, AWSSecretKey);
	Aws::Client::ClientConfiguration Config = Aws::Client::ClientConfiguration();
	Config.region = MojexaSpacesAwsRegion;

	if (UseDynamoDBLocal) Config.endpointOverride = DynamoDBLocalEndpoint;
	ClientRef = new Aws::DynamoDB::DynamoDBClient(Credentials, Config);
	UE_LOG(LogTemp, Warning, TEXT("Game instance initialized"));
}

void UMojexaSpacesMarkerManager::Shutdown()
{
	Super::Shutdown();
	Aws::ShutdownAPI(Aws::SDKOptions());
	UE_LOG(LogTemp, Warning, TEXT("Game instance shutdown complete"));
}


/*
 * Given a Device ID, fetch the latest location recorded strictly
 * after the LastKnownTimestamp
 */
FVector UMojexaSpacesMarkerManager::GetLatestRecord(FString TargetDeviceID, FDateTime LastKnownTimestamp)
{
	Aws::DynamoDB::Model::QueryRequest Request;
	Request.SetTableName(DynamoDBTableName);
	Request.SetKeyConditionExpression(PartitionKeyAttributeNameAws + " = :valueToMatch");

	// Set Expression AttributeValues
	Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> AttributeValues;
	Aws::String PartitionKeyVal = Aws::String(TCHAR_TO_UTF8(*TargetDeviceID));
	AttributeValues.emplace(":valueToMatch", PartitionKeyVal);
	Request.SetExpressionAttributeValues(AttributeValues);
	Request.SetScanIndexForward(false);
	Request.SetLimit(1);

	// Perform Query operation
	const Aws::DynamoDB::Model::QueryOutcome& result = ClientRef->Query(Request);
	if (result.IsSuccess())
	{
		// Reference the retrieved items
		for (const auto& Item : result.GetResult().GetItems())
		{
			// get timestamp data
			Aws::DynamoDB::Model::AttributeValue TimestampValue = Item.at(SortKeyAttributeNameAws);
			FDateTime Timestamp = FDateTime::FromUnixTimestamp(std::atoi(TimestampValue.GetS().c_str()));
			UE_LOG(LogTemp, Warning, TEXT("Timestamp: %d"), Timestamp.ToUnixTimestamp());

			if (Timestamp > LastKnownTimestamp)
			{
				// get location data
				float lon, lat, elev;
				FDefaultValueHelper::ParseFloat(FString(Item.at(PositionXAttributeNameAws).GetS().c_str()), lon);
				FDefaultValueHelper::ParseFloat(FString(Item.at(PositionYAttributeNameAws).GetS().c_str()), lat);
				FDefaultValueHelper::ParseFloat(FString(Item.at(PositionYAttributeNameAws).GetS().c_str()), elev);
				return FVector(lon, lat, elev);
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Failed to query items: %s"), *FString(result.GetError().GetMessage().c_str()));
	return FVector::ZeroVector;
}

ALocationMarker* UMojexaSpacesMarkerManager::CreateMarkerFromJsonObject(const FJsonObject* JsonObject)
{
	const FDateTime Timestamp = FDateTime::FromUnixTimestamp(
		FCString::Atoi(*JsonObject->GetStringField(SortKeyAttributeName)));
	FString MarkerTypeStr;
	ELocationMarkerType MarkerType = ELocationMarkerType::Static;

	if (JsonObject->TryGetStringField(MarkerTypeAttributeName, MarkerTypeStr))
	{
		UE_LOG(LogTemp, Warning, TEXT("Marker type Field: %s"), *MarkerTypeStr);
		if (MarkerTypeStr == FString("dynamic"))
		{
			MarkerType = ELocationMarkerType::Dynamic;
		}
	}
	return CreateMarker(FVector(
		                    JsonObject->GetNumberField(PositionXAttributeName),
		                    JsonObject->GetNumberField(PositionYAttributeName),
		                    JsonObject->GetNumberField(PositionZAttributeName)),
	                    Timestamp,
	                    JsonObject->GetStringField(PartitionKeyAttributeName),
	                    MarkerType);
}

ALocationMarker* UMojexaSpacesMarkerManager::CreateMarker(const FVector SpawnLocation,
                                                          const ELocationMarkerType MarkerType)
{
	return CreateMarker(SpawnLocation, FDateTime::Now(), FString("UE"), MarkerType);
}

ALocationMarker* UMojexaSpacesMarkerManager::CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp,
                                                          const FString DeviceID, const ELocationMarkerType MarkerType)
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

			UE_LOG(LogTemp, Warning, TEXT("Created %s - %s"), *Marker->GetName(), *Marker->ToString());
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Marker->ToJsonString());
		}

		AllMarkers.Add(GetTypeHash(Marker), Marker);
		UE_LOG(LogTemp, Warning, TEXT("%d data points stored"), AllMarkers.Num());
	}
	return Marker;
}

/*
 * Create new entry in DynamoDB from the given Location Marker using the aws-sdk DynamoDB client
 */
bool UMojexaSpacesMarkerManager::CreateMarkerInDB(const ALocationMarker* Marker)
{
	Aws::DynamoDB::Model::PutItemRequest Request;
	Request.SetTableName(DynamoDBTableName);

	Aws::DynamoDB::Model::AttributeValue PartitionKeyValue;
	PartitionKeyValue.SetS(Aws::String(TCHAR_TO_UTF8(*Marker->DeviceID)));
	Request.AddItem(PartitionKeyAttributeNameAws, PartitionKeyValue);

	Aws::DynamoDB::Model::AttributeValue SortKeyValue;
	SortKeyValue.SetS(Aws::String(TCHAR_TO_UTF8(*FString::FromInt(Marker->Timestamp.ToUnixTimestamp()))));
	Request.AddItem(SortKeyAttributeNameAws, SortKeyValue);

	Aws::DynamoDB::Model::AttributeValue Lon, Lat, Elev;
	Lon.SetS(Aws::String(TCHAR_TO_UTF8(*FString::SanitizeFloat(Marker->Coordinate.X))));
	Request.AddItem(PositionXAttributeNameAws, Lon);

	Lat.SetS(Aws::String(TCHAR_TO_UTF8(*FString::SanitizeFloat(Marker->Coordinate.Y))));
	Request.AddItem(PositionYAttributeNameAws, Lat);

	Elev.SetS(Aws::String(TCHAR_TO_UTF8(*FString::SanitizeFloat(Marker->Coordinate.Z))));
	Request.AddItem(PositionZAttributeNameAws, Elev);

	Aws::DynamoDB::Model::PutItemOutcome Outcome = ClientRef->PutItem(Request);
	if (Outcome.IsSuccess())
	{
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("Put item failed: %s"), *FString(Outcome.GetError().GetMessage().c_str()));
	return false;
}


bool UMojexaSpacesMarkerManager::CreateMarkerInDBByAPI(ALocationMarker* Marker)
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

void UMojexaSpacesMarkerManager::GetMarkersFromDB()
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

void UMojexaSpacesMarkerManager::OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                      bool bWasSuccessful)
{
	TSharedPtr<FJsonObject> JsonObjArr;
	if (Response->GetResponseCode() == 200 && bWasSuccessful)
	{
		const FString ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
		if (FJsonSerializer::Deserialize(Reader, JsonObjArr))
		{
			TArray<TSharedPtr<FJsonValue>> MarkerArray = JsonObjArr->GetArrayField("data");
			UE_LOG(LogTemp, Warning, TEXT("Fetch Success: %d items"), MarkerArray.Num());
			// for dynamic markers, spawn the last location
			TMap<FString, FJsonObject*> DynamicMarkers;

			for (const TSharedPtr<FJsonValue>& MarkerValue : MarkerArray)
			{
				// how to determine if new marker should be spawned, or not?
				// no marker with same device id, time, and coordinates can co exist
				FJsonObject* JsonObject = MarkerValue->AsObject().Get();
				if (JsonObject->HasField(MarkerTypeAttributeName) && JsonObject->HasField(PartitionKeyAttributeName) &&
					JsonObject->HasField(SortKeyAttributeName))
				{
					FString MarkerType = JsonObject->GetStringField(MarkerTypeAttributeName); // == FString("dynamic")
					FString DeviceID = JsonObject->GetStringField(PartitionKeyAttributeName);

					FString TimestampStr = JsonObject->GetStringField(SortKeyAttributeName);
					FDateTime Timestamp = FDateTime::FromUnixTimestamp(FCString::Atoi(*TimestampStr));
					UE_LOG(LogTemp, Log, TEXT("Timestamp: %s"), *Timestamp.ToString());
					DynamicMarkers.Add(DeviceID, JsonObject);
				}
				else
				{
					const ALocationMarker* Marker = CreateMarkerFromJsonObject(JsonObject);
					if (Marker != nullptr) UE_LOG(LogTemp, Log, TEXT("Created Marker: %s %s"), *Marker->GetName(),
					                              *Marker->ToString());
				}
			}

			for (const auto DMarker : DynamicMarkers)
			{
				const ALocationMarker* Marker = CreateMarkerFromJsonObject(DMarker.Value);
				if (Marker != nullptr) UE_LOG(LogTemp, Log, TEXT("Created Dynamic Marker: %s %s"), *Marker->GetName(),
				                              *Marker->ToString());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Error %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
	}
}

void UMojexaSpacesMarkerManager::DeleteMarker(ALocationMarker* Marker, bool DeleteFromDB)
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

void UMojexaSpacesMarkerManager::DeleteMarkers(const bool DeleteFromDB)
{
	for (const auto& Pair : AllMarkers)
	{
		ALocationMarker* Marker = Pair.Value;
		if (Marker != nullptr)
		{
			DeleteMarker(Marker, DeleteFromDB);
		}
	}
}

void UMojexaSpacesMarkerManager::DeleteSelectedMarkers(bool DeleteFromDB)
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

void UMojexaSpacesMarkerManager::DeleteMarkerFromDB(ALocationMarker* Marker)
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

void UMojexaSpacesMarkerManager::OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                         bool bWasSuccessful)
{
	if (Response->GetResponseCode() != 200 || !bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not delete marker - HTTP %d: %s"), Response->GetResponseCode(),
		       *Response->GetContentAsString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SUCCESS: Delete marker - HTTP %d: %s"), Response->GetResponseCode(),
		       *Response->GetContentAsString());
	}
}
