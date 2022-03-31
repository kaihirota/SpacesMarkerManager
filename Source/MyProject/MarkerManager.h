// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
#include "aws/dynamodbstreams/model/DescribeStreamRequest.h"
#include "aws/dynamodbstreams/model/GetRecordsRequest.h"
#include "aws/dynamodbstreams/model/GetShardIteratorRequest.h"
#include "MarkerManager.generated.h"

UCLASS()
class MYPROJECT_API AMarkerManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMarkerManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBoxComponent* SpawnVolume;

	UFUNCTION(BlueprintCallable)
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID, const bool Temporary);
	ALocationMarker* CreateMarker(FVector SpawnLocation);
	ALocationMarker* CreateMarker(FVector SpawnLocation, bool Temporary);

	ALocationMarker* CreateMarkerFromJsonObject(const FJsonObject* JsonObject);

	UFUNCTION(BlueprintCallable)
	bool CreateMarkerInDB(ALocationMarker* Marker);
	
	UFUNCTION(BlueprintCallable)
	void GetMarkersFromDB();

	UFUNCTION(BlueprintCallable)
	void DeleteMarker(ALocationMarker* Marker, const bool DeleteFromDB = true);

	UFUNCTION(BlueprintCallable)
	void DeleteMarkers(const bool DeleteFromDB = true);
	
	UFUNCTION(BlueprintCallable)
	void DeleteSelectedMarkers(bool DeleteFromDB = true);
	
	UFUNCTION(BlueprintCallable)
	void DeleteMarkerFromDB(ALocationMarker* Marker);

	// UPROPERTY(BlueprintReadOnly)
	// TArray<ALocationMarker*> Selected;

	// TMultiMap<FString, ALocationMarker*> AllMarkers; // Device ID
	TMap<uint32, ALocationMarker*> AllMarkers; // hash

	void GetDynamoDBStreamsShardIteratorAsync(
		Aws::DynamoDBStreams::Model::GetShardIteratorResult ShardIteratorResult,
		bool& Success,
		Aws::DynamoDBStreams::DynamoDBStreamsErrors& ErrorType,
		FString& ErrorMsg,
		const FLatentActionInfo& LatentInfo);
	
	// void GetDynamoDBStreamsRecordsAsync(
	// 	Aws::DynamoDBStreams::DynamoDBStreamsErrors &ErrorType,
	// 	FString &ErrorMsg,
	// 	const FLatentActionInfo &LatentInfo);

	Aws::DynamoDBStreams::DynamoDBStreamsClient* ClientRef;
	Aws::DynamoDBStreams::Model::DescribeStreamRequest DescribeStreamRequest;
	Aws::DynamoDBStreams::Model::DescribeStreamOutcome DescribeStreamOutcome;
	Aws::DynamoDBStreams::Model::DescribeStreamResult DescribeStreamResult;
	Aws::DynamoDBStreams::Model::GetShardIteratorRequest ShardIteratorRequest;
	Aws::DynamoDBStreams::Model::GetShardIteratorOutcome ShardIteratorOutcome;
	Aws::DynamoDBStreams::Model::GetShardIteratorResult ShardIteratorResult;
	Aws::Vector<Aws::DynamoDBStreams::Model::Shard> Shards;
	Aws::DynamoDBStreams::Model::GetRecordsRequest GetRecordsRequest;
	Aws::DynamoDBStreams::Model::GetRecordsOutcome GetRecordsOutcome;
	Aws::DynamoDBStreams::Model::GetRecordsResult GetRecordsResult;
	bool ShardIteratorRequestSuccess = false;
	Aws::DynamoDBStreams::DynamoDBStreamsErrors AErrorType;
	FString AErrorMsg;
	const FLatentActionInfo ALatentInfo;
	bool Completed = false;
	Aws::String ShardIterator;
	Aws::String ShardId;

protected:
	void OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
};