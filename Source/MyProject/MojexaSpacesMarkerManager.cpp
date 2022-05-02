// Fill out your copyright notice in the Description page of Project Settings.


#include "MojexaSpacesMarkerManager.h"

#include "DynamicMarker.h"
#include "HttpModule.h"
#include "Settings.h"
#include "TemporaryMarker.h"
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/core/client/ClientConfiguration.h"
#include "aws/core/http/standard/StandardHttpRequest.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodb/model/PutItemRequest.h"
#include "aws/dynamodb/model/QueryRequest.h"
#include "aws/dynamodb/model/ScanRequest.h"
#include "aws/dynamodbstreams/model/DescribeStreamRequest.h"
#include "aws/dynamodbstreams/model/GetRecordsRequest.h"
#include "aws/dynamodbstreams/model/GetShardIteratorRequest.h"
#include "aws/dynamodbstreams/model/ListStreamsRequest.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DefaultValueHelper.h"

UMojexaSpacesMarkerManager::UMojexaSpacesMarkerManager(
	const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
}

void UMojexaSpacesMarkerManager::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("Game instance initializing"));
	Super::Init();
	Aws::InitAPI(Aws::SDKOptions());

	const Aws::Auth::AWSCredentials Credentials = Aws::Auth::AWSCredentials(AWSAccessKeyId, AWSSecretKey);
	Aws::Client::ClientConfiguration Config = Aws::Client::ClientConfiguration();
	Config.region = MojexaSpacesAwsRegion;
	if (UseDynamoDBLocal) Config.endpointOverride = DynamoDBLocalEndpoint; 

	DynamoClient = new Aws::DynamoDB::DynamoDBClient(Credentials, Config);
	UE_LOG(LogTemp, Warning, TEXT("DynamoDB client ready"));
	
	StreamsClient = new Aws::DynamoDBStreams::DynamoDBStreamsClient(Credentials, Config);
	UE_LOG(LogTemp, Warning, TEXT("DynamoDB Streams ready"));

	UE_LOG(LogTemp, Warning, TEXT("Game instance initialized"));
}

/**
 * @ Do a "replay" of the inserts in the last 24 hours
 * Start reading at the last (untrimmed) stream record,
 * which is the oldest record in the shard.
 * In DynamoDB Streams, there is a 24 hour limit on data retention.
 * Stream records whose age exceeds this limit are subject to
 * removal (trimming) from the stream.
 */
void UMojexaSpacesMarkerManager::DynamoDBStreamsReplay(FDateTime TReplayStartFrom)
{
	UE_LOG(LogTemp, Warning, TEXT("Finding Streams of DynamoDB table %s"), *FString(DynamoDBTableName.c_str()));
	Aws::DynamoDBStreams::Model::ListStreamsOutcome ListStreamsOutcome = StreamsClient->ListStreams(
		Aws::DynamoDBStreams::Model::ListStreamsRequest().WithTableName(DynamoDBTableName));
	if (ListStreamsOutcome.IsSuccess())
	{
		const Aws::DynamoDBStreams::Model::ListStreamsResult ListStreamsResult = ListStreamsOutcome.GetResult();
		const Aws::Vector<Aws::DynamoDBStreams::Model::Stream> Streams = ListStreamsResult.GetStreams();
		if (Streams.size() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ListStreams error: no streams were found"));
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Found %d Streams of DynamoDB table %s"), Streams.size(), *FString(DynamoDBTableName.c_str()));
			for (auto Stream : Streams)
			{
				SingleStreamReplay(Stream, TReplayStartFrom);
			} 
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("ListStreams error: %s"), *FString(ListStreamsOutcome.GetError().GetMessage().c_str()));
	}
}

void UMojexaSpacesMarkerManager::SingleStreamReplay(const Aws::DynamoDBStreams::Model::Stream& Stream, FDateTime TReplayStartFrom)
{
	Aws::DynamoDBStreams::Model::DescribeStreamOutcome DescribeStreamOutcome;
	UE_LOG(LogTemp, Warning, TEXT("Replaying Stream ARN %s"), *FString(Stream.GetStreamArn().c_str()));
	DescribeStreamOutcome = StreamsClient->DescribeStream(
	Aws::DynamoDBStreams::Model::DescribeStreamRequest().WithStreamArn(Stream.GetStreamArn()));
	if (DescribeStreamOutcome.IsSuccess())
	{
		Aws::DynamoDBStreams::Model::DescribeStreamResult DescribeStreamResult;
		DescribeStreamResult = DescribeStreamOutcome.GetResultWithOwnership();
		Aws::Vector<Aws::DynamoDBStreams::Model::Shard> Shards = DescribeStreamResult.GetStreamDescription().GetShards();
		UE_LOG(LogTemp, Warning, TEXT("Found %d shards"), Shards.size());

		for (auto Shard : Shards)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shard %s"), *FString(Shard.GetShardId().c_str()));
			if(Shard.ShardIdHasBeenSet())
			{
				Aws::DynamoDBStreams::Model::GetShardIteratorRequest ShardIteratorRequest;
				Aws::DynamoDBStreams::Model::GetShardIteratorOutcome ShardIteratorOutcome;
				Aws::DynamoDBStreams::Model::GetShardIteratorResult ShardIteratorResult;

				// get iterator for the given shard id
				ShardIteratorRequest = Aws::DynamoDBStreams::Model::GetShardIteratorRequest()
				                       .WithStreamArn(Stream.GetStreamArn())
				                       .WithShardId(Shard.GetShardId())
				                       .WithShardIteratorType(Aws::DynamoDBStreams::Model::ShardIteratorType::TRIM_HORIZON);

				ShardIteratorOutcome = StreamsClient->GetShardIterator(ShardIteratorRequest);
				if (ShardIteratorOutcome.IsSuccess())
				{
					IterateShard(ShardIteratorOutcome.GetResultWithOwnership(), TReplayStartFrom);
				}
			}
		}
		LastEvaluatedShardId = DescribeStreamResult.GetStreamDescription().GetLastEvaluatedShardId();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DescribeStreams error: %s"),
		       *FString(DescribeStreamOutcome.GetError().GetMessage().c_str()));
	}
}

void UMojexaSpacesMarkerManager::IterateShard(
	const Aws::DynamoDBStreams::Model::GetShardIteratorResult& GetShardIteratorResult, FDateTime TReplayStartFrom)
{
	Aws::String ShardIterator = GetShardIteratorResult.GetShardIterator();
	int processedRecordCount = 0;
	int shardPageCount = 0;
	while (ShardIterator != "" && NumberOfEmptyShards <= NumberOfEmptyShardsLimit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shard Iterator %s"), *FString(ShardIterator.c_str()));
						
		Aws::DynamoDBStreams::Model::GetRecordsOutcome GetRecordsOutcome;
		GetRecordsOutcome = StreamsClient->GetRecords(Aws::DynamoDBStreams::Model::GetRecordsRequest()
			.WithShardIterator(ShardIterator));
		if (GetRecordsOutcome.IsSuccess())
		{
			Aws::DynamoDBStreams::Model::GetRecordsResult GetRecordsResult = GetRecordsOutcome.GetResult();
			Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records = GetRecordsResult.GetRecords();
			ProcessDynamoDBStreamRecords(Records, TReplayStartFrom);
			processedRecordCount += Records.size();
			if (ShardIterator == GetRecordsResult.GetNextShardIterator()) break;
			ShardIterator = GetRecordsResult.GetNextShardIterator();
			shardPageCount++;
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("GetRecords error: %s"), *FString(GetRecordsOutcome.GetError().GetMessage().c_str()));
			break;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Processing complete. Processed %d events from %d pages"), processedRecordCount, shardPageCount);
}

void UMojexaSpacesMarkerManager::ProcessDynamoDBStreamRecords(Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records, FDateTime TReplayStartFrom)
{
	UE_LOG(LogTemp, Warning, TEXT("Found %d records"), Records.size());
	if (Records.size() == 0)
	{
		NumberOfEmptyShards++;
		return;
	}
	NumberOfEmptyShards = 0;
	for (auto Record : Records)
	{
		if (Record.GetEventName() == Aws::DynamoDBStreams::Model::OperationType::INSERT)
		{
			Aws::Utils::Json::JsonValue JsonValue = Record.Jsonize();
			Aws::Utils::Json::JsonView JsonView = JsonValue.View().GetObject("dynamodb").GetObject("NewImage");

			LastEvaluatedSequenceNumber = JsonValue.View().GetObject("dynamodb").GetString("SequenceNumber");
			const int CreationDateTime = JsonValue.View().GetObject("dynamodb").GetInteger("ApproximateCreationDateTime");
			FDateTime CreatedDateTime = FDateTime::FromUnixTimestamp(CreationDateTime);

			UE_LOG(LogTemp, Warning, TEXT("INSERT: EventID=%s Time:%s Region=%s SeqNum=%s"),
				*FString(Record.GetEventID().c_str()),
				*CreatedDateTime.ToIso8601(),
				*FString(Record.GetAwsRegion().c_str()),
				*FString(LastEvaluatedSequenceNumber.c_str()));
			
			if (CreatedDateTime >= TReplayStartFrom)
			{
				if (JsonView.ValueExists("device_id") && JsonView.ValueExists("created_timestamp"))
				{
					// type
					FString MarkerTypeStr;
					if (JsonView.KeyExists("marker_type") && JsonView.ValueExists("marker_type"))
					{
						MarkerTypeStr = FString(JsonView.GetObject("marker_type").GetString("S").c_str()); 
					} else
					{
						MarkerTypeStr = "static";
					}
					
					ELocationMarkerType MarkerType;

					if (MarkerTypeStr == "dynamic") MarkerType = ELocationMarkerType::Dynamic;
					else if (MarkerTypeStr == "temporary") MarkerType = ELocationMarkerType::Temporary;
					else MarkerType = ELocationMarkerType::Static;

					FString DeviceID = FString(JsonView.GetObject("device_id").GetString("S").c_str());
					FString Timestamp = FString(JsonView.GetObject("created_timestamp").GetString("S").c_str());

					double Lon = FCString::Atod(*FString(JsonView.GetObject("longitude").GetString("N").c_str()));
					double Lat = FCString::Atod(*FString(JsonView.GetObject("latitude").GetString("N").c_str()));
					double Elev = FCString::Atod(*FString(JsonView.GetObject("elevation").GetString("N").c_str()));

					UE_LOG(LogTemp, Warning,
						TEXT("device_id: %s, created_timestamp: %s, marker_type: %s, lon: %s, lat: %s, elev: %s"),
						*DeviceID,
						*Timestamp,
						*MarkerTypeStr,
						*FString::SanitizeFloat(Lon),
						*FString::SanitizeFloat(Lat),
						*FString::SanitizeFloat(Elev));

					// convert to json object
					FJsonObject* JsonObject = new FJsonObject;
					JsonObject->SetStringField(PartitionKeyAttributeName, DeviceID);
					JsonObject->SetStringField(SortKeyAttributeName, Timestamp);
					JsonObject->SetNumberField(PositionXAttributeName, Lon);
					JsonObject->SetNumberField(PositionYAttributeName, Lat);
					JsonObject->SetNumberField(PositionZAttributeName, Elev);

					if (MarkerType == ELocationMarkerType::Dynamic)
					{
						JsonObject->SetStringField(MarkerTypeAttributeName, "dynamic");
						FString DynamicMarkerDeviceID = JsonObject->GetStringField(PartitionKeyAttributeName);
						if (!AllMarkers.Contains(DynamicMarkerDeviceID))
						{
							// spawn only if AllMarkers doesn't contain the device ID
							ALocationMarker* CreatedMarker = CreateMarkerFromJsonObject(JsonObject);
							if (CreatedMarker != nullptr)
							{
								UE_LOG(LogTemp, Warning, TEXT("Created Dynamic Marker: %s %s"), *CreatedMarker->GetName(),
							   *CreatedMarker->ToString());
							}
						} else
						{
							// find and move the existing dynamic marker
							ALocationMarker* DynamicMarker = *AllMarkers.Find(DynamicMarkerDeviceID);
							UE_LOG(LogTemp, Warning, TEXT("Found Dynamic Marker: %s"), *DynamicMarker->ToString());
						}
					} else if (MarkerType == ELocationMarkerType::Temporary)
					{
						JsonObject->SetStringField(MarkerTypeAttributeName, "temporary");
						CreateMarkerFromJsonObject(JsonObject);
					}
					else
					{
						JsonObject->SetStringField(MarkerTypeAttributeName, "static");
						CreateMarkerFromJsonObject(JsonObject);
					}
				} else
				{
					UE_LOG(LogTemp, Warning, TEXT("GetRecords error - Record does not have a device_id or created_timestamp"));
					break;
				}
			}
		}
	}
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
	const Aws::DynamoDB::Model::QueryOutcome& result = DynamoClient->Query(Request);
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
				FDefaultValueHelper::ParseFloat(FString(Item.at(PositionXAttributeNameAws).GetN().c_str()), lon);
				FDefaultValueHelper::ParseFloat(FString(Item.at(PositionYAttributeNameAws).GetN().c_str()), lat);
				FDefaultValueHelper::ParseFloat(FString(Item.at(PositionZAttributeNameAws).GetN().c_str()), elev);
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

	ALocationMarker* Marker = Cast<ALocationMarker>(MarkerActor);
	if (Marker != nullptr)
	{
		Marker->Coordinate = SpawnLocation;
		Marker->Timestamp = Timestamp;
		Marker->DeviceID = DeviceID;

		UE_LOG(LogTemp, Warning, TEXT("Created %s - %s"), *Marker->GetName(), *Marker->ToString());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Marker->ToJsonString());
	}

	AllMarkers.Add(DeviceID, Marker);
	UE_LOG(LogTemp, Warning, TEXT("%d data points stored"), AllMarkers.Num());
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

	Aws::DynamoDB::Model::PutItemOutcome Outcome = DynamoClient->PutItem(Request);
	if (Outcome.IsSuccess())
	{
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("Put item failed: %s"), *FString(Outcome.GetError().GetMessage().c_str()));
	return false;
}

void UMojexaSpacesMarkerManager::GetMarkersFromDB()
{
	Aws::DynamoDB::Model::ScanRequest Request;
	Request.SetTableName(DynamoDBTableName);
	Aws::DynamoDB::Model::ScanOutcome Outcome = DynamoClient->Scan(Request);
	if (Outcome.IsSuccess())
	{
		Aws::DynamoDB::Model::ScanResult Result = Outcome.GetResult();
		UE_LOG(LogTemp, Warning, TEXT("DynamoDB Scan Request success: %d items"), Result.GetCount());
		TMap<FString, FJsonObject*> DynamicMarkers;
		
		for (auto Pairs : Result.GetItems())
		{
			// device id
			FString DeviceID = AwsStringToFString(Pairs.at(PartitionKeyAttributeNameAws).GetS());
			
			// type
			ELocationMarkerType MarkerType;
			if (Pairs.find(MarkerTypeAttributeNameAws) == Pairs.end())
			{
				MarkerType = ELocationMarkerType::Static;
			} else
			{
				const FString MarkerTypeStr = FString(Pairs.at(MarkerTypeAttributeNameAws).GetS().c_str());
				if (MarkerTypeStr.Equals(FString("dynamic"))) MarkerType = ELocationMarkerType::Dynamic;
				else MarkerType = ELocationMarkerType::Static;
			}

			float lon, lat, elev;
			FDefaultValueHelper::ParseFloat(FString(Pairs.at(PositionXAttributeNameAws).GetN().c_str()), lon);
			FDefaultValueHelper::ParseFloat(FString(Pairs.at(PositionYAttributeNameAws).GetN().c_str()), lat);
			FDefaultValueHelper::ParseFloat(FString(Pairs.at(PositionZAttributeNameAws).GetN().c_str()), elev);

			// convert to json object
			FJsonObject* JsonObject = new FJsonObject;
			JsonObject->SetStringField(PartitionKeyAttributeName, DeviceID);
			JsonObject->SetStringField(SortKeyAttributeName, FString(Pairs.at(SortKeyAttributeNameAws).GetS().c_str()));
			JsonObject->SetNumberField(PositionXAttributeName, lon);
			JsonObject->SetNumberField(PositionYAttributeName, lat);
			JsonObject->SetNumberField(PositionZAttributeName, elev);
			
			if (MarkerType == ELocationMarkerType::Dynamic)
			{
				DynamicMarkers.Add(DeviceID, JsonObject);
				JsonObject->SetStringField(MarkerTypeAttributeName, "dynamic");
			} else
			{
				ALocationMarker* Marker = CreateMarkerFromJsonObject(JsonObject);
			}
		} 
		for (const auto DMarker : DynamicMarkers)
		{
			// spawn only if AllMarkers doesn't contain the device ID
			if (!AllMarkers.Contains(DMarker.Value->GetStringField(PartitionKeyAttributeName)))
			{
				ALocationMarker* CreatedMarker = CreateMarkerFromJsonObject(DMarker.Value);
				if (CreatedMarker != nullptr)
				{
					UE_LOG(LogTemp, Warning, TEXT("Created Dynamic Marker: %s %s"), *CreatedMarker->GetName(),
				   *CreatedMarker->ToString());
				}
			}
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("DynamoDB Scan Request failed: %s"), *FString(Outcome.GetError().GetMessage().c_str()));
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
				if (JsonObject->HasField(MarkerTypeAttributeName))
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
				if (Marker != nullptr)
				{
					UE_LOG(LogTemp, Log, TEXT("Created Dynamic Marker: %s %s"), *Marker->GetName(),
						  *Marker->ToString());
				}
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
		AllMarkers.FindAndRemoveChecked(Marker->DeviceID);
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
		// Request->SetURL(MarkerAPIEndpoint);
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
