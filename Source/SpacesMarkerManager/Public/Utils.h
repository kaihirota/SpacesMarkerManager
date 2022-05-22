#pragma once

#include "CoreMinimal.h"
#include "aws/dynamodbstreams/model/ShardIteratorType.h"
#include "Utils.generated.h"


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

	UPROPERTY(BlueprintReadOnly, Category="Spaces|Util")
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
