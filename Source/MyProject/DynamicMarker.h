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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	FColor DynamicMarkerColor = FColor::Purple;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	int idx = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	float InterpolationsPerSecond = 1000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	TArray<FLocationTs> HistoryArr; // heap

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FString ToString() const;

	UFUNCTION(BlueprintCallable, Category="Spaces")
	void AddLocationTs(const FLocationTs Location);
	
	FString ToJsonString() const;
	
	TSharedRef<FJsonObject> ToJsonObject() const;
};
