// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LocationMarker.h"
#include "TemporaryMarker.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTemporaryMarker, Display, All);

UCLASS(BlueprintType, Blueprintable)
class SPACESMARKERMANAGER_API ATemporaryMarker : public ALocationMarker
{
	GENERATED_BODY()

public:
	ATemporaryMarker();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker|Temporary")
	FColor TemporaryMarkerColor = FColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker|Temporary")
	float DefaultLifeSpan = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker|Temporary")
	float MaxScale = 2.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
