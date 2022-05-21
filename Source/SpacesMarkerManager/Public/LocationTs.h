#pragma once

#include "CoreMinimal.h"
#include "LocationTs.generated.h"


/*
 * Wrapper class for coordinates at a given point in time.
 * Use MarkerManager.WrapLocationTs to create an instance of this object.
 * Expects input to be in WGS84 coordinate system, and
 * will calculate the other coordinates based on the WGS84.
 * For safety, the attributes in this class are read-only
 * because when one coordinate is changed, the others must also change.
 */
USTRUCT(BlueprintType, meta = (HasNativeMake = "SpacesMarkerManager.MarkerManager.WrapLocationTs"))
struct FLocationTs
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Util")
	FDateTime Timestamp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Util")
	FVector UECoordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Util")
	FVector Wgs84Coordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Util")
	FVector EcefCoordinate;
	
	FLocationTs(const FDateTime DateTime, const FVector InGameLocation, const FVector Wgs84Location, const FVector EcefLocation)
	{
		Timestamp = DateTime;
		UECoordinate = InGameLocation;
		Wgs84Coordinate = Wgs84Location;
		EcefCoordinate = EcefLocation;
	}

	FLocationTs()
	{
	}

	friend bool operator < (const FLocationTs& x, const FLocationTs& y)
	{
		return x.Timestamp < y.Timestamp;
	}

	FString ToString() const
	{
		TArray<FStringFormatArg> Args;
		Args.Add(FStringFormatArg(UECoordinate.ToString()));
		Args.Add(FStringFormatArg(Wgs84Coordinate.ToString()));
		Args.Add(FStringFormatArg(EcefCoordinate.ToString()));
		Args.Add(FStringFormatArg(Timestamp.ToIso8601()));
		return FString::Format(TEXT("(UE={0}, WGS84={1}, ECEF={2}, Timestamp='{3}')"), Args);
	}
};
