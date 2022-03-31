#include "MarkerManager.h"

#include "Components/BoxComponent.h"
#include "LocationMarker.h"
#include "JsonObjectConverter.h"
#include "HTTP.h"
#include "TemporaryMarker.h"
#include "Settings.h"
#include "aws/core/Aws.h"
#include "aws/core/auth/AWSCredentials.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "aws/dynamodbstreams/model/GetShardIteratorRequest.h"

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
	ClientRef = new Aws::DynamoDBStreams::DynamoDBStreamsClient(Creadentials, Config);

	DescribeStreamRequest = Aws::DynamoDBStreams::Model::DescribeStreamRequest().WithStreamArn(DynamoDBStreamsARN);
	DescribeStreamOutcome = ClientRef->DescribeStream(DescribeStreamRequest);

	if(DescribeStreamOutcome.IsSuccess())
	{
		DescribeStreamResult = DescribeStreamOutcome.GetResultWithOwnership();
		Shards = DescribeStreamResult.GetStreamDescription().GetShards();
		UE_LOG(LogTemp, Warning, TEXT("Found %d shards"), Shards.size());
		
		for (auto Shard : Shards)
		{
			UE_LOG(LogTemp, Warning, TEXT("shard %s"), *FString(Shard.GetShardId().c_str()));
			
			// get iterator for the given shard id
			ShardIteratorRequest = Aws::DynamoDBStreams::Model::GetShardIteratorRequest()
				.WithShardId(Shard.GetShardId())
				.WithStreamArn(DynamoDBStreamsARN)
				.WithShardIteratorType(Aws::DynamoDBStreams::Model::ShardIteratorType::LATEST);

			if (ShardIteratorRequest.ShardIteratorTypeHasBeenSet() && ShardIteratorRequest.StreamArnHasBeenSet())
			{
				ShardIteratorOutcome = ClientRef->GetShardIterator(ShardIteratorRequest);
				if (ShardIteratorOutcome.IsSuccess())
				{
					ShardIteratorResult = ShardIteratorOutcome.GetResult();
					ShardIterator = ShardIteratorResult.GetShardIterator();

					// get record from the shard using the iterator
					if (!ShardIterator.empty())
					{
						GetRecordsRequest = Aws::DynamoDBStreams::Model::GetRecordsRequest()
									.WithShardIterator(ShardIterator)
									.WithLimit(10);
						GetRecordsOutcome = ClientRef->GetRecords(GetRecordsRequest);
						if (GetRecordsOutcome.IsSuccess())
						{
							GetRecordsResult = GetRecordsOutcome.GetResult();
							Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records = GetRecordsResult.GetRecords();
							UE_LOG(LogTemp, Warning, TEXT("Found %d records using sharditerator %s"), Records.size(), *FString(ShardIterator.c_str()));
							for (auto Record : Records)
							{
								Aws::Utils::Json::JsonValue JsonVal = Record.Jsonize();
								Aws::String JsonStr = Aws::String();
								JsonVal.AsString(JsonStr);
								UE_LOG(LogTemp, Warning, TEXT("Record: %s"), *FString(JsonStr.c_str()));						
							}
							// ShardIterator = GetRecordsResult.GetNextShardIterator();
						} else
						{
							UE_LOG(LogTemp, Warning, TEXT("GetRecords error: %s"), *FString(GetRecordsOutcome.GetError().GetMessage().c_str()));
							ShardIterator = "";
						}
					}
				}
				// GetDynamoDBStreamsShardIteratorAsync(ShardIteratorResult, ShardIteratorRequestSuccess, AErrorType, AErrorMsg, ALatentInfo);
			}
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Something went wrong %s"), *FString(DescribeStreamOutcome.GetError().GetMessage().c_str()));
	}
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

// void AMarkerManager::GetDynamoDBStreamsShardIteratorAsync(
// 	Aws::DynamoDBStreams::Model::GetShardIteratorResult AShardIteratorResult,
// 	bool &Success,
// 	Aws::DynamoDBStreams::DynamoDBStreamsErrors &ErrorType,
// 	FString &ErrorMsg,
// 	const FLatentActionInfo &LatentInfo)
// {
// 	ShardIteratorRequest = Aws::DynamoDBStreams::Model::GetShardIteratorRequest()
// 			.WithStreamArn(DynamoDBStreamsARN)
// 			.WithShardIteratorType(Aws::DynamoDBStreams::Model::ShardIteratorType::TRIM_HORIZON);
// 	
// 	if (ClientRef != nullptr && ClientRef)
// 	{
// 		ClientRef->GetShardIteratorAsync(
// 			ShardIteratorRequest, [&Success, &AShardIteratorResult, &ErrorType, &ErrorMsg, this](
// 				const Aws::DynamoDBStreams::DynamoDBStreamsClient *awsDynamoDBStreamsClient,
// 				const Aws::DynamoDBStreams::Model::GetShardIteratorRequest &awsGetShardIteratorRequest,
// 				const Aws::DynamoDBStreams::Model::GetShardIteratorOutcome &awsGetShardIteratorOutcome,
// 				const std::shared_ptr<const Aws::Client::AsyncCallerContext> &awsAsyncCallerContext
// 			) mutable -> void {
// 				Success = awsGetShardIteratorOutcome.IsSuccess();
// 				if (Success) {
// 					AShardIteratorResult = awsGetShardIteratorOutcome.GetResult();
// 					ShardIterator = AShardIteratorResult.GetShardIterator();
// 					GetRecordsRequest = Aws::DynamoDBStreams::Model::GetRecordsRequest()
// 						.WithShardIterator(ShardIterator)
// 						.WithLimit(10);
// 					while (!ShardIterator.empty())
// 					{
// 						GetRecordsOutcome = ClientRef->GetRecords(GetRecordsRequest);
// 						if (GetRecordsOutcome.IsSuccess())
// 						{
// 							const Aws::DynamoDBStreams::Model::GetRecordsResult Result = GetRecordsOutcome.GetResult();
// 							const Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records = Result.GetRecords();
// 							UE_LOG(LogTemp, Warning, TEXT("%d records found in shard %s"), Records.size(), *FString(ShardIterator.c_str()));
// 							
// 							for (auto Record : Records)
// 							{
// 								Aws::Utils::Json::JsonValue JsonVal = Record.Jsonize();
// 								Aws::String JsonStr = Aws::String();
// 								JsonVal.AsString(JsonStr);
// 								UE_LOG(LogTemp, Warning, TEXT("Record: %s"), *FString(JsonStr.c_str()));						
// 							}
// 							ShardIterator = Result.GetNextShardIterator();
// 						} else
// 						{
// 							UE_LOG(LogTemp, Warning, TEXT("GetRecords error: %s"), *FString(GetRecordsOutcome.GetError().GetMessage().c_str()));
// 						}
// 					}
// 				} else
// 				{
// 					ErrorType = awsGetShardIteratorOutcome.GetError().GetErrorType();
// 					ErrorMsg = ("GetShardIterator error: " + awsGetShardIteratorOutcome.GetError().GetMessage()).c_str();
// 					UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(ErrorMsg));
// 					Completed = true;
// 				}
// 			},
// 			std::make_shared<Aws::Client::AsyncCallerContext>(std::to_string(LatentInfo.UUID).c_str())
// 			);
// 	}
// 	
// }

// void AMarkerManager::GetDynamoDBStreamsRecordsAsync(
// 	Aws::DynamoDBStreams::DynamoDBStreamsErrors &ErrorType,
// 	FString &ErrorMsg,
// 	const FLatentActionInfo &LatentInfo)
// {
// 	bool GetRecordsAsyncSuccess = false;
// 	Aws::DynamoDBStreams::Model::GetRecordsResult Result;
// 	ClientRef->GetRecordsAsync(
// 		GetRecordsRequest, [&GetRecordsAsyncSuccess, &Result, &ErrorType, &ErrorMsg, this](
// 			const Aws::DynamoDBStreams::DynamoDBStreamsClient *awsDynamoDBStreamsClient,
// 			const Aws::DynamoDBStreams::Model::GetRecordsRequest &RecordsRequest,
// 			const Aws::DynamoDBStreams::Model::GetRecordsOutcome &RecordsOutcome,
// 			const std::shared_ptr<const Aws::Client::AsyncCallerContext> &awsAsyncCallerContext
// 		) mutable -> void {
// 			GetRecordsAsyncSuccess = RecordsOutcome.IsSuccess();
//
// 			if (GetRecordsAsyncSuccess) {
// 				const Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records = RecordsOutcome.GetResult().GetRecords();
// 				UE_LOG(LogTemp, Warning, TEXT("%d records found in shard"), Records.size());
// 				for (auto Record : Records)
// 				{
// 					Aws::Utils::Json::JsonValue JsonVal = Record.Jsonize();
// 					Aws::String JsonStr = Aws::String();
// 					JsonVal.AsString(JsonStr);
// 					UE_LOG(LogTemp, Warning, TEXT("Record: %s"), *FString(JsonStr.c_str()));						
// 				}
// 			} else
// 			{
// 				ErrorType = RecordsOutcome.GetError().GetErrorType();
// 				ErrorMsg = ("GetRecords error: " + RecordsOutcome.GetError().GetMessage()).c_str();
// 				UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(ErrorMsg));
// 			}
//
// 		},
// 		std::make_shared<Aws::Client::AsyncCallerContext>(std::to_string(LatentInfo.UUID).c_str()));
// }

ALocationMarker* AMarkerManager::CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID, const bool Temporary)
{
	// if (GetRecordsRequest.ShardIteratorHasBeenSet())
	// {
	// 	GetDynamoDBStreamsRecordsAsync(AErrorType, AErrorMsg, ALatentInfo);
	// }
	
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