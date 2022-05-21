#pragma once

#include "CoreMinimal.h"
#include "aws/dynamodbstreams/model/ShardIteratorType.h"
#include "Utils.generated.h"

/**
 * Static: Location & size are fixed after being initialized
 * Temporary: Size can be set as a function of time
 * Dynamic: Location & size can be changed
 */
UENUM(BlueprintType)
enum class ELocationMarkerType : uint8
{
	Static		UMETA(DisplayName = StaticMarkerName),
	Temporary	UMETA(DisplayName = TemporaryMarkerName),
	Dynamic		UMETA(DisplayName = DynamicMarkerName)
};

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
