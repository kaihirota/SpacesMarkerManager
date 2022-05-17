#include "MarkerManager.h"

#include "DynamicMarker.h"
#include "Settings.h"
#include "TemporaryMarker.h"
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/core/client/ClientConfiguration.h"
#include "aws/core/http/standard/StandardHttpRequest.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodb/model/DeleteItemRequest.h"
#include "aws/dynamodb/model/PutItemRequest.h"
#include "aws/dynamodb/model/QueryRequest.h"
#include "aws/dynamodb/model/ScanRequest.h"
#include "aws/dynamodbstreams/model/DescribeStreamRequest.h"
#include "aws/dynamodbstreams/model/GetRecordsRequest.h"
#include "aws/dynamodbstreams/model/GetShardIteratorRequest.h"
#include "aws/dynamodbstreams/model/ListStreamsRequest.h"
#include "Misc/DefaultValueHelper.h"


void UMarkerManager::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("Game instance initializing"));
	Super::Init();
	UE_LOG(LogTemp, Warning, TEXT("Initializing AWS SDK..."));
	Aws::InitAPI(Aws::SDKOptions());

	const Aws::Auth::AWSCredentials Credentials = Aws::Auth::AWSCredentials(AWSAccessKeyId, AWSSecretKey);
	Aws::Client::ClientConfiguration Config = Aws::Client::ClientConfiguration();
	Config.region = SpacesAwsRegion;
	if (UseDynamoDBLocal) Config.endpointOverride = DynamoDBLocalEndpoint;

	DynamoClient = new Aws::DynamoDB::DynamoDBClient(Credentials, Config);
	UE_LOG(LogTemp, Warning, TEXT("DynamoDB client ready"));

	StreamsClient = new Aws::DynamoDBStreams::DynamoDBStreamsClient(Credentials, Config);
	UE_LOG(LogTemp, Warning, TEXT("DynamoDB Streams ready"));

	UE_LOG(LogTemp, Warning, TEXT("Initialized AWS SDK."));
	UE_LOG(LogTemp, Warning, TEXT("Initialized MarkerManager Subsystem."));
}

void UMarkerManager::Shutdown()
{
	Super::Shutdown();
	Aws::ShutdownAPI(Aws::SDKOptions());
	UE_LOG(LogTemp, Warning, TEXT("Game instance shutdown complete"));
}

void UMarkerManager::DynamoDBStreamsListen()
{
	if (Listening)
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UMarkerManager::DynamoDBStreamsListenOnce,
		                                          PollingInterval, true, 0.0f);
	}
	Listening = !Listening;
}

void UMarkerManager::DynamoDBStreamsListenOnce()
{
	const TArray<FAwsString> Streams = GetStreams(
		FAwsString::FromFString(*DynamoDBTableName));
	if (Streams.Num() > 0)
	{
		const FAwsString StreamArn = Streams[0];
		const TArray<FAwsString> Shards = GetShards(StreamArn);
		if (Shards.Num() > 0)
		{
			IterateShard(StreamArn, Shards[0],
			             Aws::DynamoDBStreams::Model::ShardIteratorType::LATEST, NULL);
		}
	}
}

TArray<FAwsString> UMarkerManager::GetStreams(const FAwsString TableName)
{
	UE_LOG(LogTemp, Warning, TEXT("Fetching DynamoDB Streams for table: %s"), *TableName.Fstring);
	Aws::DynamoDBStreams::Model::ListStreamsOutcome ListStreamsOutcome = StreamsClient->ListStreams(
		Aws::DynamoDBStreams::Model::ListStreamsRequest().WithTableName(TableName.AwsString));
	
	TArray<FAwsString> StreamArns;
	if (ListStreamsOutcome.IsSuccess())
	{
		const Aws::DynamoDBStreams::Model::ListStreamsResult ListStreamsResult = ListStreamsOutcome.GetResultWithOwnership();
		Aws::Vector<Aws::DynamoDBStreams::Model::Stream> Streams = ListStreamsResult.GetStreams();
		for (Aws::DynamoDBStreams::Model::Stream Stream : Streams)
		{
			StreamArns.Add(FAwsString::FromAwsString(Stream.GetStreamArn()));
		} 
		LastEvaluatedStreamArn = ListStreamsResult.GetLastEvaluatedStreamArn();
		UE_LOG(LogTemp, Warning, TEXT("Found %d DynamoDB Streams for table %s"), Streams.size(), *TableName.Fstring);
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("ListStreams error: %s"), *FString(ListStreamsOutcome.GetError().GetMessage().c_str()));
	}
	return StreamArns;
}


TArray<FAwsString> UMarkerManager::GetShards(const FAwsString StreamArn) const
{
	UE_LOG(LogTemp, Warning, TEXT("Stream ARN %s"), *StreamArn.Fstring);
	Aws::DynamoDBStreams::Model::DescribeStreamOutcome DescribeStreamOutcome = StreamsClient->DescribeStream(
		Aws::DynamoDBStreams::Model::DescribeStreamRequest().WithStreamArn(StreamArn.AwsString));

	TArray<FAwsString> ShardIds;
	if (DescribeStreamOutcome.IsSuccess())
	{
		const Aws::DynamoDBStreams::Model::DescribeStreamResult DescribeStreamResult = DescribeStreamOutcome.GetResultWithOwnership();
		Aws::Vector<Aws::DynamoDBStreams::Model::Shard> Shards = DescribeStreamResult.GetStreamDescription().GetShards();
		for (Aws::DynamoDBStreams::Model::Shard Shard : Shards)
		{
			ShardIds.Add(FAwsString::FromAwsString(Shard.GetShardId()));
		} 
		UE_LOG(LogTemp, Warning, TEXT("Found %d shards for Stream ARN %s"), Shards.size(), *StreamArn.Fstring);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DescribeStreams error: %s"), *FString(DescribeStreamOutcome.GetError().GetMessage().c_str()));
	}
	return ShardIds;
}

void UMarkerManager::DynamoDBStreamsReplay(FString TableName)
{
	TArray<FAwsString> Streams;
	if (TableName == "") Streams = GetStreams(FAwsString::FromAwsString(DynamoDBTableNameAws));
	else Streams = GetStreams(FAwsString::FromFString(TableName));
	for (const auto Stream : Streams)
	{
		ScanStream(Stream, FDateTime::Now() - FTimespan::FromHours(24.0));
	}
}

void UMarkerManager::ScanStream(const FAwsString StreamArn, const FDateTime TReplayStartFrom)
{
	const TArray<FAwsString> Shards = GetShards(StreamArn);
	for (auto Shard : Shards)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shard %s"), *Shard.Fstring);
		IterateShard(StreamArn, Shard, Aws::DynamoDBStreams::Model::ShardIteratorType::TRIM_HORIZON, TReplayStartFrom);
	}
}

void UMarkerManager::IterateShard(
	const FAwsString StreamArn,
	const FAwsString ShardId,
	const FDynamoDBStreamShardIteratorType ShardIteratorType,
	const FDateTime TReplayStartFrom)
{
	if (ShardIterator == "")
	{
		UE_LOG(LogTemp, Warning, TEXT("ShardIterator not created. Creating it now."));
		Aws::DynamoDBStreams::Model::GetShardIteratorOutcome GetShardIteratorOutcome = StreamsClient->GetShardIterator(
			Aws::DynamoDBStreams::Model::GetShardIteratorRequest()
			.WithStreamArn(StreamArn.AwsString)
			.WithShardId(ShardId.AwsString)
			.WithShardIteratorType(ShardIteratorType.Value));
		if (GetShardIteratorOutcome.IsSuccess())
		{
			ShardIterator = FString(GetShardIteratorOutcome.GetResult().GetShardIterator().c_str());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Error: Could not create ShardIterator"));
		}
		return;
	}

	int ProcessedRecordCount = 0;
	int ShardPageCount = 0;
	do
	{
		UE_LOG(LogTemp, Warning, TEXT("Shard Iterator %s"), *ShardIterator);
		Aws::DynamoDBStreams::Model::GetRecordsOutcome GetRecordsOutcome = StreamsClient->GetRecords(
			Aws::DynamoDBStreams::Model::GetRecordsRequest().WithShardIterator(
				Aws::String(TCHAR_TO_UTF8(*ShardIterator))));
		if (GetRecordsOutcome.IsSuccess())
		{
			Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records = GetRecordsOutcome.GetResult().GetRecords();
			ProcessDynamoDBStreamRecords(Records, TReplayStartFrom);
			ProcessedRecordCount += Records.size();
			ShardPageCount++;
			ShardIterator = FString(GetRecordsOutcome.GetResult().GetNextShardIterator().c_str());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GetRecords error: %s"),
			       *FString(GetRecordsOutcome.GetError().GetMessage().c_str()));
			break;
		}
		UE_LOG(LogTemp, Warning, TEXT("Processing complete. Processed %d events from %d pages"), ProcessedRecordCount,
		       ShardPageCount);
	}
	while (ShardIterator != "" && NumberOfEmptyShards <= NumberOfEmptyShardsLimit);
}

void UMarkerManager::ProcessDynamoDBStreamRecords(
	const Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records,
	const FDateTime TReplayStartFrom)
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

			if (CreatedDateTime >= TReplayStartFrom)
			{
				if (JsonView.ValueExists(PartitionKeyAttributeNameAws) && JsonView.ValueExists(SortKeyAttributeNameAws))
				{
					// type
					ELocationMarkerType MarkerType = ELocationMarkerType::Static;;
					if (JsonView.KeyExists(MarkerTypeAttributeNameAws) && JsonView.ValueExists(MarkerTypeAttributeNameAws))
					{
						FString MarkerTypeStr = FString(JsonView.GetObject(MarkerTypeAttributeNameAws).GetString("S").c_str());
						if (MarkerTypeStr == "dynamic") MarkerType = ELocationMarkerType::Dynamic;
						else if (MarkerTypeStr == "temporary") MarkerType = ELocationMarkerType::Temporary;
					}

					FString DeviceID = FString(JsonView.GetObject(PartitionKeyAttributeNameAws).GetString("S").c_str());
					FString TimestampStr = FString(JsonView.GetObject(SortKeyAttributeNameAws).GetString("S").c_str());
					const FDateTime Timestamp = FDateTime::FromUnixTimestamp(FCString::Atoi(*TimestampStr));
					const double Lon = FCString::Atod(*FString(JsonView.GetObject(PositionXAttributeNameAws).GetString("N").c_str()));
					const double Lat = FCString::Atod(*FString(JsonView.GetObject(PositionYAttributeNameAws).GetString("N").c_str()));
					const double Elev = FCString::Atod(*FString(JsonView.GetObject(PositionZAttributeNameAws).GetString("N").c_str()));
					const FVector Coordinate = FVector(Lon, Lat, Elev);
					const FLocationTs LocationTs = FLocationTs(Timestamp, Coordinate);
					
					ALocationMarker* Marker;
					if (MarkerType == ELocationMarkerType::Dynamic && LocationMarkers.Contains(DeviceID))
					{
						// dynamic marker with matching device id already exists
						Marker = *LocationMarkers.Find(DeviceID);
						if (ADynamicMarker* DynamicMarker = Cast<ADynamicMarker>(Marker))
						{
							// pass the new data to the marker
							DynamicMarker->AddLocationTs(LocationTs);
							UE_LOG(LogTemp, Warning, TEXT("Added new location %s for Dynamic marker %s"),
								*LocationTs.ToString(),
								*Marker->ToString());
						}
					} else
					{
						// spawn dynamic marker only if AllMarkers doesn't contain the device ID
						Marker = CreateMarker(LocationTs, MarkerType, DeviceID);
						UE_LOG(LogTemp, Warning, TEXT("Created Dynamic Marker: %s %s"), *Marker->GetName(), *Marker->ToString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("GetRecords error - Record does not have a device_id or created_timestamp"));
					break;
				}
			}
		}
	}
}

auto UMarkerManager::GetLatestRecord(const FString DeviceID, const FDateTime LastKnownTimestamp) -> FVector
{
	Aws::DynamoDB::Model::QueryRequest Request;
	Request.SetTableName(DynamoDBTableNameAws);
	Request.SetKeyConditionExpression(PartitionKeyAttributeNameAws + " = :valueToMatch");

	// Set Expression AttributeValues
	Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> AttributeValues;
	Aws::String PartitionKeyVal = Aws::String(TCHAR_TO_UTF8(*DeviceID));
	AttributeValues.emplace(":valueToMatch", PartitionKeyVal);
	Request.SetExpressionAttributeValues(AttributeValues);
	Request.SetScanIndexForward(false);
	Request.SetLimit(1);

	// Perform Query operation
	const Aws::DynamoDB::Model::QueryOutcome& Result = DynamoClient->Query(Request);
	if (Result.IsSuccess())
	{
		// Reference the retrieved items
		for (const auto& Item : Result.GetResult().GetItems())
		{
			// get timestamp data
			Aws::DynamoDB::Model::AttributeValue TimestampValue = Item.at(SortKeyAttributeNameAws);
			FDateTime Timestamp = FDateTime::FromUnixTimestamp(std::atoi(TimestampValue.GetS().c_str()));
			UE_LOG(LogTemp, Warning, TEXT("Timestamp: %s"), *Timestamp.ToIso8601());

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
	UE_LOG(LogTemp, Warning, TEXT("Failed to query items: %s"), *FString(Result.GetError().GetMessage().c_str()));
	return FVector::ZeroVector;
}

ALocationMarker* UMarkerManager::CreateMarker(const FLocationTs LocationTs, const ELocationMarkerType MarkerType, const FString DeviceID)
{
	AActor* MarkerActor;

	switch (MarkerType)
	{
		case ELocationMarkerType::Static:
			MarkerActor = GetWorld()->SpawnActor<ALocationMarker>(LocationTs.Coordinate, FRotator::ZeroRotator);
			break;
		case ELocationMarkerType::Temporary:
			MarkerActor = GetWorld()->SpawnActor<ATemporaryMarker>(LocationTs.Coordinate, FRotator::ZeroRotator);
			break;
		case ELocationMarkerType::Dynamic:
			MarkerActor = GetWorld()->SpawnActor<ADynamicMarker>(LocationTs.Coordinate, FRotator::ZeroRotator);
			break;
		default:
			return nullptr;
	}

	if (MarkerActor != nullptr)
	{
		ALocationMarker* Marker = Cast<ALocationMarker>(MarkerActor);
		if (Marker != nullptr)
		{
			Marker->LocationTs = LocationTs;
			Marker->DeviceID = DeviceID;

			UE_LOG(LogTemp, Warning, TEXT("Created %s - %s"), *Marker->GetName(), *Marker->ToString());
			// UE_LOG(LogTemp, Warning, TEXT("%s"), *Marker->ToJsonString());
		}

		LocationMarkers.Add(DeviceID, Marker);
		UE_LOG(LogTemp, Warning, TEXT("%d data points stored"), LocationMarkers.Num());
		return Marker;
	}
	UE_LOG(LogTemp, Warning, TEXT("Error: Could not cast LocationMarker to desired type."));
	return nullptr;
}

bool UMarkerManager::CreateMarkerInDB(const ALocationMarker* Marker)
{
	Aws::DynamoDB::Model::PutItemRequest Request;
	Request.SetTableName(DynamoDBTableNameAws);

	Aws::DynamoDB::Model::AttributeValue PartitionKeyValue;
	PartitionKeyValue.SetS(Aws::String(TCHAR_TO_UTF8(*Marker->DeviceID)));
	Request.AddItem(PartitionKeyAttributeNameAws, PartitionKeyValue);

	Aws::DynamoDB::Model::AttributeValue SortKeyValue;
	SortKeyValue.SetS(Aws::String(TCHAR_TO_UTF8(*FString::FromInt(Marker->LocationTs.Timestamp.ToUnixTimestamp()))));
	Request.AddItem(SortKeyAttributeNameAws, SortKeyValue);

	Aws::DynamoDB::Model::AttributeValue Lon, Lat, Elev;
	Lon.SetS(Aws::String(TCHAR_TO_UTF8(*FString::SanitizeFloat(Marker->LocationTs.Coordinate.X))));
	Request.AddItem(PositionXAttributeNameAws, Lon);

	Lat.SetS(Aws::String(TCHAR_TO_UTF8(*FString::SanitizeFloat(Marker->LocationTs.Coordinate.Y))));
	Request.AddItem(PositionYAttributeNameAws, Lat);

	Elev.SetS(Aws::String(TCHAR_TO_UTF8(*FString::SanitizeFloat(Marker->LocationTs.Coordinate.Z))));
	Request.AddItem(PositionZAttributeNameAws, Elev);

	const Aws::DynamoDB::Model::PutItemOutcome Outcome = DynamoClient->PutItem(Request);
	if (Outcome.IsSuccess())
	{
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("Put item failed: %s"), *FString(Outcome.GetError().GetMessage().c_str()));
	return false;
}

void UMarkerManager::GetAllMarkersFromDynamoDB()
{
	Aws::DynamoDB::Model::ScanRequest Request;
	Request.SetTableName(DynamoDBTableNameAws);
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
			}
			else
			{
				const FString MarkerTypeStr = FString(Pairs.at(MarkerTypeAttributeNameAws).GetS().c_str());
				if (MarkerTypeStr.Equals(FString("dynamic"))) MarkerType = ELocationMarkerType::Dynamic;
				else MarkerType = ELocationMarkerType::Static;
			}
			
			Aws::DynamoDB::Model::AttributeValue TimestampValue = Pairs.at(SortKeyAttributeNameAws);
			FDateTime Timestamp = FDateTime::FromUnixTimestamp(std::atoi(TimestampValue.GetS().c_str()));
			
			float Lon, Lat, Elev;
			FDefaultValueHelper::ParseFloat(FString(Pairs.at(PositionXAttributeNameAws).GetN().c_str()), Lon);
			FDefaultValueHelper::ParseFloat(FString(Pairs.at(PositionYAttributeNameAws).GetN().c_str()), Lat);
			FDefaultValueHelper::ParseFloat(FString(Pairs.at(PositionZAttributeNameAws).GetN().c_str()), Elev);
			const FVector Coordinate = FVector(Lon, Lat, Elev);
			const FLocationTs LocationTs = FLocationTs(Timestamp, Coordinate);
					
			ALocationMarker* Marker;
			if (MarkerType == ELocationMarkerType::Dynamic && LocationMarkers.Contains(DeviceID))
			{
				// dynamic marker with matching device id already exists
				Marker = *LocationMarkers.Find(DeviceID);
				if (ADynamicMarker* DynamicMarker = Cast<ADynamicMarker>(Marker))
				{
					// pass the new data to the marker
					DynamicMarker->AddLocationTs(LocationTs);
					UE_LOG(LogTemp, Warning, TEXT("Added new location %s for Dynamic marker %s"),
						*LocationTs.ToString(),
						*Marker->ToString());
				}
			} else
			{
				// spawn dynamic marker only if AllMarkers doesn't contain the device ID
				Marker = CreateMarker(LocationTs, MarkerType, DeviceID);
				UE_LOG(LogTemp, Warning, TEXT("Created Dynamic Marker: %s %s"), *Marker->GetName(), *Marker->ToString());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DynamoDB Scan Request failed: %s"),
		       *FString(Outcome.GetError().GetMessage().c_str()));
	}
}

void UMarkerManager::DeleteSelectedMarkers(const bool DeleteFromDB)
{
	for (auto It = LocationMarkers.CreateIterator(); It; ++It)
	{
		ALocationMarker* Marker = It.Value();
		if (Marker != nullptr)
		{
			if (Marker->Selected)
			{
				UE_LOG(LogTemp, Warning, TEXT("Destroying: %s - %s"), *Marker->GetName(), *Marker->ToString());
				DeleteMarkerFromDynamoDB(Marker->DeviceID, Marker->LocationTs.Timestamp);
				if (DeleteFromDB) Marker->Destroy();
				It.RemoveCurrent();
			}
		}
	}
}

auto UMarkerManager::DeleteMarkerFromDynamoDB(const FString DeviceID, const FDateTime Timestamp) -> bool
{
	Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue> AttributeValues;
	Aws::DynamoDB::Model::AttributeValue PartitionKey;
	PartitionKey.SetS(Aws::String(TCHAR_TO_UTF8(*DeviceID)));
	Aws::DynamoDB::Model::AttributeValue SortKey;
	SortKey.SetS(Aws::String(TCHAR_TO_UTF8(*FString::FromInt(Timestamp.ToUnixTimestamp()))));
	// TODO does not work when using global constants for some reason, data type mismatch even if using Aws::String
	AttributeValues.emplace("device_id", PartitionKey);
	AttributeValues.emplace("created_timestamp", SortKey);

	const Aws::DynamoDB::Model::DeleteItemRequest Request = Aws::DynamoDB::Model::DeleteItemRequest()
	                                                        .WithTableName(DynamoDBTableNameAws)
	                                                        .WithKey(AttributeValues);
	const Aws::DynamoDB::Model::DeleteItemOutcome Outcome = DynamoClient->DeleteItem(Request);
	return Outcome.IsSuccess();
}
