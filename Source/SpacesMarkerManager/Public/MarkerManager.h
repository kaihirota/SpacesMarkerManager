#pragma once

#include "CoreMinimal.h"
#include "CesiumGeoreference.h"
#include "LocationMarker.h"
#include "Utils.h"
#include "LocationTs.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "MarkerManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMarkerManager, Display, All);

class ALocationMarker;

UCLASS(Blueprintable, BlueprintType)
class SPACESMARKERMANAGER_API UMarkerManager : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Spaces|MarkerManager")
	int NumberOfEmptyShards = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Spaces|MarkerManager")
	int NumberOfEmptyShardsLimit = 5;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Spaces|MarkerManager")
	bool Listening = false;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Spaces|MarkerManager")
	FString ShardIterator = "";

	/* Length between each successive call to DynamoDBStreamsListen() */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Spaces|MarkerManager")
	double PollingInterval = 2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spaces|MarkerManager")
	FTimerHandle TimerHandle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spaces|MarkerManager")
	ACesiumGeoreference* Georeference;

protected:
	// Maps from DeviceID to LocationMarker
	TMap<FString, ALocationMarker*> SpawnedLocationMarkers;
	
	Aws::DynamoDB::DynamoDBClient* DynamoClient;
	Aws::DynamoDBStreams::DynamoDBStreamsClient* DynamoDBStreamsClient;

	// DynamoDB Streams
	Aws::String LastEvaluatedShardId;
	Aws::String LastEvaluatedSequenceNumber;
	Aws::String LastEvaluatedStreamArn;

	virtual void Init() override;
	virtual void Shutdown() override;
	
public:

	/**
	* Spawn a marker of specified type, initialized with the provided parameters.
	* Using this function to spawn location markers will save a references in SpawnedLocationMarkers.
	* @param LocationTs
	* @param MarkerType
	* @param DeviceID
	* @returns Marker [ALocationMarker] 
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	ALocationMarker* SpawnAndInitializeMarker(const FLocationTs LocationTs, const ELocationMarkerType MarkerType, const FString DeviceID);

	/****************   DynamoDB   ******************/

	/**
	* Given a reference to a LocationMarker instance, insert it into DynamoDB table.
	* This function was written with the assumption that within the game,
	* only Static Markers and Temporary Markers are created by the user, and
	* temporary markers created in the game do not need to be stored in DynamoDB.
	* The reason for this assumption is that this game is mainly the consumer of data
	* as opposed to being a producer.
	* @param Marker
	* @returns Success [bool] True if insert succeeded, else False.
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	bool CreateMarkerInDB(const ALocationMarker* Marker) const;

	/**
	* Fetch all markers from DynamoDB and spawn them in the world.
	* Caution: Because this method retrieves all rows from DynamoDB table,
	* this is the most expensive function. It's recommended to use a local
	* DynamoDB instance to not accumulate charges.
	* @param StaticMarkersOnly [bool] If set to true, only static markers will be spawned.
	* Otherwise, static and dynamic markers are spawned.
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	void GetAllMarkersFromDynamoDB(const bool StaticMarkersOnly = true);

	/**
	* Given a DeviceID, query DynamoDB for the last known location.
	* If the last known location from DynamoDB is the same as LastKnownTimestamp, zero vector will be returned
	* @param DeviceID
	* @param LastKnownTimestamp
	* @returns LatestLocation [FVector]
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	FVector GetLatestRecord(const FString DeviceID, const FDateTime LastKnownTimestamp);

	/**
	* Destroy all the spawned markers that are currently selected.
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	void DestroySelectedMarkers();

	/**
	* Delete a single marker with matching attributes from SpawnedLocationMarkers.
	* If DeleteFromDB is true, the marker will also be deleted from DynamoDB.
	* This method is delegated to location markers, and called automatically
	* in BeginDestroy(). If DeleteFromDB is False, the marker will be destroyed
	* and removed from SpawnedLocationMarkers, but not deleted from DynamoDB.
	* @param DeviceID [FString]
	* @param Timestamp [FDateTime]
	* @param DeleteFromDB [bool]
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	void DestroyMarker(const FString DeviceID, const FDateTime Timestamp, const bool DeleteFromDB);

	/**
	 * Deletes a marker with matching attributes from DynamoDB.
	 * Cannot delete a record without both parameters, since they are both keys.
	 * @param DeviceID
	 * @param Timestamp
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	bool DeleteMarkerFromDynamoDB(const FString DeviceID, const FDateTime Timestamp) const;

	/****************   DynamoDB Streams   ******************/

	/**
	 * Start polling DynamoDB Streams at interval specified by PollingInterval.
	 * Call this method to begin / end listening to the Streams.
	 * Internally it uses DynamoDBStreamsListenOnce().
	 * Table name must be configured in Settings.h.
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	void DynamoDBStreamsListen();

	/**
	 * This function is called repeatedly to poll DynamoDB Streams for latest events.
	 * At each invocation, it obtains a list of all the DynamoDB Streams
	 * associated with the configured DynamoDB table. If there is one or more stream,
	 * it will iterate through the first stream found.
	 * Uses DynamoDB Stream LATEST iterator type.
	 **/
	// UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	void DynamoDBStreamsListenOnce();

	/**
	 * Given a DynamoDB table, which may be associated with one or more streams,
	 * replay all the insert events in the last 24 hours.
	 * Internally this calls ScanStreams() on each Stream associated with a given table.
	 * @param TableName
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	void DynamoDBStreamsReplay(FString TableName);

	/**
	 * @param TableName
	 * @returns List of stream ARNs for the given DynamoDB table.
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
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
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	void ScanStream(const FAwsString StreamArn, const FDateTime TReplayStartFrom);

	/**
	 * Given a stream ARN, get a list of shards in the stream.
	 * @param StreamArn
	 **/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	TArray<FAwsString> GetShards(const FAwsString StreamArn) const;

	/**
	* Given a shard, create shard iterator with the specified ShardIteratorType
	* and use that to iterate through the shard.
	* @param StreamArn
	* @param ShardId
	* @param ShardIteratorType
	* @param TReplayStartFrom For TRIM_HORIZON shard iterator, this can be specified as a filter to skip the oldest records.
	*/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	void IterateShard(const FAwsString StreamArn,
					  const FAwsString ShardId,
	                  const FDynamoDBStreamShardIteratorType ShardIteratorType,
	                  const FDateTime TReplayStartFrom);

	void ProcessDynamoDBStreamRecords(Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records,
	                                  FDateTime TReplayStartFrom);


	/**
	* Given timestamp and lon, lat, elevation in WGS84, return a wrapper object
	* that contains WGS84, UE, ECEF coordinates, and the timestamp.
	* If CesiumGeoReference is active and UseCesiumGeoreference is set to True in Settings.h file,
	* this function will convert the given coordinates from Wgs84 Lon, Lat, Elevation to UE coordinate
	* and store both of them in FLocationTs.
	* @param Timestamp [FDateTime]
	* @param Lon [double]
	* @param Lat [double]
	* @param Elev [double]
	* @return FLocationTs
	*/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	FLocationTs WrapLocationTs(const FDateTime Timestamp, const double Lon, const double Lat, const double Elev) const;
	FLocationTs WrapLocationTs(FDateTime Timestamp, FVector Coordinate) const;

	/**
	* Return a list of location markers currently existing in the UE World.
	* @return TArray<ALocationMarker*>
	*/
	UFUNCTION(BlueprintCallable, Category="Spaces|MarkerManager")
	TArray<ALocationMarker*> GetActiveMarkers() const;
};
