// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "Interfaces/IHttpRequest.h"
#include "MqttUtilities/Private/Linux/MqttClientImpl.h"
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
	int NumberOfEmptyShards = 0;
	int NumberOfEmptyShardsLimit = 5;
private:
	GENERATED_BODY()

protected:
	void OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
public:
	UMojexaSpacesMarkerManager(const FObjectInitializer& ObjectInitializer);
	virtual void Init() override;
	virtual void Shutdown() override;
	
	void handle_signal(int s);
	static void connect_callback(mosquitto* mosq, void* obj, int result);
	static void message_callback(mosquitto* mosq, void* obj, const mosquitto_message* message);
	static void BeginSubscribe();
	void IterateStreams();
	void Subscribe();
	static int ResultCode;
	// static int RunStatus;
	
	void DynamoDBStreamsReplay(FDateTime TReplayStartFrom);
	void SingleStreamReplay(const Aws::DynamoDBStreams::Model::Stream& Stream, FDateTime TReplayStartFrom);
	void IterateShard(const Aws::DynamoDBStreams::Model::GetShardIteratorResult& GetShardIteratorResult, FDateTime TReplayStartFrom);
	void ProcessDynamoDBStreamRecords(Aws::Vector<Aws::DynamoDBStreams::Model::Record> Records, FDateTime TReplayStartFrom);

	UFUNCTION(BlueprintCallable)
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const ELocationMarkerType MarkerType);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID);
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
	void DeleteMarkerFromDB(ALocationMarker* Marker);
};
