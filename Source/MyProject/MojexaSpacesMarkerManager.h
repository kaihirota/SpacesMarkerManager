// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "aws/dynamodbstreams/DynamoDBStreamsClient.h"
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
private:
	GENERATED_BODY()

protected:
	void OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
public:
	UMojexaSpacesMarkerManager(const FObjectInitializer& ObjectInitializer);
	virtual void Init() override;
	void IterateStreams();
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable)
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const ELocationMarkerType MarkerType);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID,
	                              const ELocationMarkerType MarkerType);
	ALocationMarker* CreateMarkerFromJsonObject(const FJsonObject* JsonObject);

	UFUNCTION(BlueprintCallable)
	bool CreateMarkerInDB(const ALocationMarker* Marker);

	UFUNCTION(BlueprintCallable)
	bool CreateMarkerInDBByAPI(ALocationMarker* Marker);

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
