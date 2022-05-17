#pragma once

#include "CoreMinimal.h"
#include "LocationMarkerType.h"
#include "LocationTs.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "MarkerManager.generated.h"

class ALocationMarker;
UCLASS(Blueprintable, BlueprintType)
class SPACESMARKERMANAGER_API UMarkerManager : public UGameInstance
{
	GENERATED_BODY()

public:
	// Maps from DeviceID to LocationMarker
	TMap<FString, ALocationMarker*> LocationMarkers;
	Aws::DynamoDB::DynamoDBClient* DynamoClient;
	Aws::DynamoDBStreams::DynamoDBStreamsClient* StreamsClient;

	// DynamoDB Streams
	Aws::String LastEvaluatedShardId;
	Aws::String LastEvaluatedSequenceNumber;
	Aws::String LastEvaluatedStreamArn;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spaces")
	FString ShardIterator = "";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Spaces")
	int NumberOfEmptyShards = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spaces")
	int NumberOfEmptyShardsLimit = 5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spaces")
	bool Listening = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spaces")
	FTimerHandle TimerHandle;

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/**
	* Spawn a marker with the provided parameters and save a references in LocationMarkers
	* @param LocationTs
	* @param MarkerType
	* @param DeviceID
	* @returns Marker [ALocationMarker] 
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	ALocationMarker* CreateMarker(const FLocationTs LocationTs, const ELocationMarkerType MarkerType, const FString DeviceID);

	/****************   DynamoDB   ******************/

	/**
	* Given a reference to a LocationMarker instance, insert it into DynamoDB table.
	* @param Marker
	* @returns Success [bool] True if insert succeeded, else False.
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	bool CreateMarkerInDB(const ALocationMarker* Marker);

	/**
	* Fetch all markers from DynamoDB and spawn them in the world.
	* For DynamicMarkers, only one marker will be spawned for the most recent known location.
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void GetAllMarkersFromDynamoDB();
	
	/**
	* Given a DeviceID, query DynamoDB for the last known location.
	* If the last known location from DynamoDB is the same as LastKnownTimestamp, zero vector will be returned
	* @param DeviceID
	* @param LastKnownTimestamp
	* @returns LatestLocation [FVector]
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	FVector GetLatestRecord(const FString DeviceID, const FDateTime LastKnownTimestamp);

	/**
	* Delete all the spawned markers that are currently selected.
	* If DeleteFromDB is true, the marker will also be deleted from DynamoDB
	* @param DeleteFromDB [bool] Default set to false.
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void DeleteSelectedMarkers(const bool DeleteFromDB = true);

	/**
	 * Deletes a marker with matching attributes.
	 * @param DeviceID
	 * @param Timestamp
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	bool DeleteMarkerFromDynamoDB(FString DeviceID, FDateTime Timestamp);

	/****************   DynamoDB Streams   ******************/

	/**
	 * Start polling DynamoDB Streams at some specified interval.
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void DynamoDBStreamsListen();

	/**
	 * Function called repeatedly to poll DynamoDB Streams for latest events.
	 * At each invocation, it obtains a list of all the DynamoDB Streams
	 * associated with the configured DynamoDB table. If there is one or more stream,
	 * it will iterate through the first stream found.
	 * Uses DynamoDB Stream LATEST iterator type.
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void DynamoDBStreamsListenOnce();

	/**
	 * Given a DynamoDB table, which may be associated with one or more streams, replay the events in the last 24 hours.
	 * @param TableName
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void DynamoDBStreamsReplay(FString TableName);

	/**
	 * @param TableName
	 * @returns List of stream ARNs for all the associated streams.
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	TArray<FAwsString> GetStreams(const FAwsString TableName);

	/**
	* Do a "replay" of the inserts in the last 24 hours for a single stream.
	* Start reading at the last (untrimmed) stream record,
	* which is the oldest record in the shard.
	* In DynamoDB Streams, there is a 24 hour limit on data retention.
	* Stream records whose age exceeds this limit are subject to
	* removal (trimming) from the stream.
	* @param StreamArn
	* @param TReplayStartFrom Oldest threshold timestamp from which to start the replay from.
\	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void ScanStream(const FAwsString StreamArn, const FDateTime TReplayStartFrom);

	/**
	 * Given a stream ARN, get a list of ID for the shards in the stream.
	 * @param StreamArn
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	TArray<FAwsString> GetShards(const FAwsString StreamArn) const;

	/**
	* Given a shard, create shard iterator with the specified ShardIteratorType
	* and use that to iterate through the shard.
	* @param StreamArn
	* @param ShardId
	* @param ShardIteratorType
	* @param TReplayStartFrom For TRIM_HORIZON shard iterator, this can be specified as a filter to skip the oldest records.
	*/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void IterateShard(const FAwsString StreamArn,
					  const FAwsString ShardId,
	                  const FDynamoDBStreamShardIteratorType ShardIteratorType,
	                  const FDateTime TReplayStartFrom);

	void ProcessDynamoDBStreamRecords(Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records,
	                                  FDateTime TReplayStartFrom);
protected:
	
};
