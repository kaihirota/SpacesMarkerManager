// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "aws/dynamodbstreams/model/ShardIteratorType.h"
#include "Interfaces/IHttpRequest.h"
#include "MojexaSpacesMarkerManager.generated.h"

UENUM()
enum class ELocationMarkerType : uint8
{
	Static,
	Temporary,
	Dynamic
};

class ALocationMarker;
/**
 * 
 */
UCLASS()
class MYPROJECT_API UMojexaSpacesMarkerManager : public UGameInstance
{
public:
	TMap<FString, ALocationMarker*> AllMarkers;

	Aws::DynamoDB::DynamoDBClient* DynamoClient;
	Aws::DynamoDBStreams::DynamoDBStreamsClient* StreamsClient;
	Aws::String LastEvaluatedShardId;
	Aws::String LastEvaluatedSequenceNumber;
	Aws::String LastEvaluatedStreamArn;
	FString ShardIterator = "";
	int NumberOfEmptyShards = 0;
	int NumberOfEmptyShardsLimit = 5;
	bool Success = true;
private:
	GENERATED_BODY()

protected:
	void OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	static void OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
public:
	UMojexaSpacesMarkerManager(const FObjectInitializer& ObjectInitializer);
	virtual void Init() override;
	Aws::Vector<Aws::DynamoDBStreams::Model::Stream> GetStreams(Aws::String TableName);
	Aws::Vector<Aws::DynamoDBStreams::Model::Shard> GetShards(Aws::String StreamArn) const;
	void ScanStream(const Aws::DynamoDBStreams::Model::Stream& Stream, Aws::DynamoDBStreams::Model::ShardIteratorType ShardIteratorType, FDateTime TReplayStartFrom);
	void IterateShard(const Aws::String StreamArn, const Aws::DynamoDBStreams::Model::Shard Shard, Aws::DynamoDBStreams::Model::ShardIteratorType ShardIteratorType, FDateTime TReplayStartFrom);
	void ProcessDynamoDBStreamRecords(Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records, FDateTime TReplayStartFrom);
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable)
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const ELocationMarkerType MarkerType);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID,
	                              const ELocationMarkerType MarkerType);
	ALocationMarker* CreateMarkerFromJsonObject(const FJsonObject* JsonObject);

	UFUNCTION(BlueprintCallable)
	bool CreateMarkerInDB(const ALocationMarker* Marker);

	UFUNCTION(BlueprintCallable)
	void GetMarkersFromDB();

	UFUNCTION(BlueprintCallable)
	FVector GetLatestRecord(FString TargetDeviceID, FDateTime LastKnownTimestamp);

	UFUNCTION(BlueprintCallable)
	void DeleteMarker(ALocationMarker* Marker, const bool DeleteFromDB = true);

	UFUNCTION(BlueprintCallable)
	void DeleteMarkers(const bool DeleteFromDB = true);

	UFUNCTION(BlueprintCallable)
	void DeleteSelectedMarkers(bool DeleteFromDB = true);

	UFUNCTION(BlueprintCallable)
	bool DeleteMarkerFromDB(ALocationMarker* Marker);
};
