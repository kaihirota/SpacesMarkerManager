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
	
	const FColor BaseColor = FColor::Purple;
	
	UPROPERTY(BlueprintReadWrite)
	float PollIntervalInSeconds = 5.0f;
	
	UPROPERTY(BlueprintReadWrite)
	float InitialDelay = 0.5f;
	
	const float InterpolationsPerSecond = 1000.0f;
	TQueue<FVector> CoordinateQueue;
	FVector NextLocation;
	FVector Step;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION()
	void EnqueueLocation(FVector Location);
};
