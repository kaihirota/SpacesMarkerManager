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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="MojexaSpaces")
	FColor DefaultColor = FColor::Red;

	/* Initial Counter value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MojexaSpaces")
	float InitialCounter = 1000.0f;

	/* Current Counter value */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MojexaSpaces")
	int Counter = InitialCounter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MojexaSpaces")
	int CounterTickSize = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MojexaSpaces")
	FTimerHandle TimerHandle;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="MojexaSpaces")
	void IncrementCounter(int Count);

	UFUNCTION(BlueprintCallable, Category="MojexaSpaces")
	void DecrementCounter(int Count);
};
