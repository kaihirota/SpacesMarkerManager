// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TemporaryMarker.h"
#include "DynamicMarker.generated.h"

UCLASS()
class MYPROJECT_API ADynamicMarker : public ATemporaryMarker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADynamicMarker();
	const FColor BaseColor = FColor::Purple;
	
	int idx = 0;
	const float InterpolationsPerSecond = 1000.0f;
	TArray<FLocationTs> HistoryArr; // heap

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	FString ToString() const;

	UFUNCTION()
	void AddLocationTs(const FLocationTs Location);
	
	TSharedRef<FJsonObject> ToJsonObject() const;
	FString ToJsonString() const;
};
