#pragma once

#include "CoreMinimal.h"
#include "Settings.h"
#include "aws/dynamodbstreams/model/ShardIteratorType.h"
#include "LocationTs.generated.h"

USTRUCT(BlueprintType)
struct FDynamoDBStreamShardIteratorType
{
	GENERATED_BODY()

	FDynamoDBStreamShardIteratorType()
	{
		Value = Aws::DynamoDBStreams::Model::ShardIteratorType::TRIM_HORIZON;
	}

	FDynamoDBStreamShardIteratorType(const Aws::DynamoDBStreams::Model::ShardIteratorType Type)
	{
		Value = Type;
	}

	Aws::DynamoDBStreams::Model::ShardIteratorType Value;
};

USTRUCT(BlueprintType)
struct FAwsString
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Spaces")
	FString Fstring;

	Aws::String AwsString;

	FAwsString()
	{
		
	}

	static FAwsString FromAwsString(Aws::String AwsStr)
	{
		FAwsString Str;
		Str.AwsString = AwsStr;
		Str.Fstring = *FString(AwsStr.c_str());
		return Str;
	}

	static FAwsString FromFString(FString FInputStr)
	{
		FAwsString Str;
		Str.Fstring = FInputStr;
		Str.AwsString = Aws::String(TCHAR_TO_UTF8(*FInputStr));
		return Str;
	}
};


USTRUCT(BlueprintType)
struct FLocationTs
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	FDateTime Timestamp;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	FVector UECoordinate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces")
	FVector Wgs84Coordinate;

	FLocationTs()
	{
	}

	FLocationTs(const FDateTime DateTime, const FVector Wgs84Location, const FVector InGameLocation)
	{
		Timestamp = DateTime;
		Wgs84Coordinate = Wgs84Location;
		UECoordinate = InGameLocation;
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
		Args.Add(FStringFormatArg(Timestamp.ToIso8601()));
		return FString::Format(TEXT("(UE={0}, Wgs84={1}, Timestamp='{2}')"), Args);
	}
};
