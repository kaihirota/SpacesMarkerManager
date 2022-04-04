// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.h"
#include "aws/dynamodb/DynamoDBClient.h"
#include "Components/TimelineComponent.h"
#include "MarkerManager.generated.h"

UENUM()
enum class ELocationMarkerType : uint8
{
	Static,
	Temporary,
	Dynamic
};

UCLASS()
class MYPROJECT_API AMarkerManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMarkerManager();

	FTimerHandle TimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBoxComponent* SpawnVolume;

	UFUNCTION(BlueprintCallable)
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const ELocationMarkerType MarkerType);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID);
	ALocationMarker* CreateMarker(const FVector SpawnLocation, const FDateTime Timestamp, const FString DeviceID, const ELocationMarkerType MarkerType);
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
	TMap<uint32, ALocationMarker*> AllMarkers; // hash
	
	Aws::DynamoDB::DynamoDBClient* ClientRef;

	UFUNCTION(BlueprintCallable)
	FVector GetLatestRecord(FString DeviceID, FDateTime Timestamp);

protected:
	void OnGetMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnDeleteMarkersResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void RepeatingFunction();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
};