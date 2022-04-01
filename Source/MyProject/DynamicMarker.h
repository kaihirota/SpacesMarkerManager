// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarkerManager.h"
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

	AMarkerManager* MarkerManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// void RepeatingFunction();
	// DECLARE_DELEGATE_RetVal_TwoParams(FVector, FMarkerManagerDelegate, FString, FDateTime)
};
