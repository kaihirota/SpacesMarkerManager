#pragma once

#include "CoreMinimal.h"
#include "Settings.h"
#include "LocationTs.generated.h"


USTRUCT(BlueprintType)
struct FLocationTs
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MojexaSpaces")
	FDateTime Timestamp;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MojexaSpaces")
	FVector Coordinate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MojexaSpaces")
	FVector Wgs84Coordinate;

	FLocationTs()
	{
		Coordinate = FVector::ZeroVector;
		Timestamp = FDateTime::Now();
	}
	
	friend bool operator < (const FLocationTs& x, const FLocationTs& y)
	{
		return x.Timestamp < y.Timestamp;
	}

	FString ToString() const
	{
		TArray<FStringFormatArg> Args;
		Args.Add(FStringFormatArg(Coordinate.ToString()));
		Args.Add(FStringFormatArg(Timestamp.ToIso8601()));
		return FString::Format(TEXT("({0}, '{1}')"), Args);
	}

	static FLocationTs FromLocationTimestamp(const FDateTime Timestamp, const FVector Coordinate)
	{
		FLocationTs Location;
		Location.Timestamp = Timestamp;
		Location.Coordinate = Coordinate;
		return Location;
	}
};
