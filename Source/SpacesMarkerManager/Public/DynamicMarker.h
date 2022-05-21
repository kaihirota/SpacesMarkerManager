// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TemporaryMarker.h"
#include "DynamicMarker.generated.h"


UCLASS(BlueprintType, Blueprintable)
class SPACESMARKERMANAGER_API ADynamicMarker : public ATemporaryMarker
{
	GENERATED_BODY()

public:
	ADynamicMarker();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	FColor DynamicMarkerColor = FColor::Purple;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	int idx = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spaces")
	float InterpolationsPerSecond = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	TArray<FLocationTs> HistoryArr; // heap

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual TSharedRef<FJsonObject> ToJsonObject() const override;
	virtual FString ToString() const override;
	virtual FString ToJsonString() const override;

	UFUNCTION(BlueprintCallable, Category="Spaces")
	void AddLocationTs(const FLocationTs Location);
};
