// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocationMarker.h"
#include "TemporaryMarker.generated.h"

UCLASS()
class MYPROJECT_API ATemporaryMarker : public ALocationMarker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATemporaryMarker();

	/* Initial Counter */
	UPROPERTY(EditAnywhere)
	float InitialCounter = 1000.0f;
	const FColor BaseColor = FColor::Red;

	/* Counter */
	UPROPERTY(EditAnywhere)
	int Counter = InitialCounter;

	FTimerHandle TimerHandle;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void SetOpacity(float Opacity) const;
	void IncrementCounter(int Count);
	void IncrementCounter();
	void DecrementCounter(int Count);
	void DecrementCounter();
	void RepeatingFunction();
};
