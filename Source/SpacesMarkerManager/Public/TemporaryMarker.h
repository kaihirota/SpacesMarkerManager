// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocationMarker.h"
#include "TemporaryMarker.generated.h"

UCLASS(BlueprintType, Blueprintable)
class SPACESMARKERMANAGER_API ATemporaryMarker : public ALocationMarker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATemporaryMarker();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	FColor TemporaryMarkerColor = FColor::Red;

	/* Initial Counter value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	float InitialCounter = 1000.0f;

	/* Current Counter value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	int Counter = InitialCounter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	int CounterTickSize = 1;

	/* Maximum scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	float MaxScale = 2.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Spaces")
	void IncrementCounter(int Count);

	UFUNCTION(BlueprintCallable, Category="Spaces")
	void DecrementCounter(int Count);
};
