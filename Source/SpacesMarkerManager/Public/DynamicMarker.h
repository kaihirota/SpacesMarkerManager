// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TemporaryMarker.h"
#include "DynamicMarker.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDynamicMarker, Display, All);

UCLASS(BlueprintType, Blueprintable)
class SPACESMARKERMANAGER_API ADynamicMarker : public ATemporaryMarker
{
	GENERATED_BODY()

public:
	ADynamicMarker();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker|Dynamic")
	FColor DynamicMarkerColor = FColor::Purple;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker|Dynamic")
	int idx = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spaces|Marker|Dynamic")
	float InterpolationsPerSecond = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker|Dynamic")
	TArray<FLocationTs> HistoryArr; // heap

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual TSharedRef<FJsonObject> ToJsonObject() const override;
	virtual FString ToString() const override;
	virtual FString ToJsonString() const override;

	UFUNCTION(BlueprintCallable, Category="Spaces|Marker|Dynamic")
	void AddLocationTs(const FLocationTs Location);
};
