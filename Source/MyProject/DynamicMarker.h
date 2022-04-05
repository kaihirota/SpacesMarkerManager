// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TemporaryMarker.h"
#include "DynamicMarker.generated.h"

UCLASS()
class MYPROJECT_API ADynamicMarker : public ALocationMarker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADynamicMarker();

	FTimerHandle TimerHandle;

	const float InterpolationsPerSecond = 2000.0f;

	const FColor BaseColor = FColor::Purple;

	// TSharedRef<AMarkerManager> MarkerManager;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void RepeatingFunction();

	DECLARE_DELEGATE_RetVal_TwoParams(FVector, FMarkerManagerDelegate, FString, FDateTime)
	FMarkerManagerDelegate ManagerDelegate;

	UFUNCTION()
	void UpdateLocation(FVector Location);

	FVector NextLocation;
	FVector PreviousLocation;
	FVector Step;

	// UTimelineComponent* platformTimeline;
	//
	// //Delegate signature for the function which will handle our Finished event.
	// FOnTimelineEvent TimelineFinishedEvent;
	//
	// UFUNCTION()
	// void TimelineFinishedFunction();
	//


	// FOnTimelineFloat InterpFunction;
	// FTimeline Timeline;

	// DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FVector, FMarkerManagerDelegate, FString, FDateTime );
};
