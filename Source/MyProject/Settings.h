#pragma once

#include "CoreMinimal.h"
#include "aws/core/Region.h"
#include "aws/core/utils/memory/stl/AWSString.h"


static const FString MarkerAPIEndpoint = "https://0flacbdme3.execute-api.ap-southeast-2.amazonaws.com/dev/";

static const char* MojexaSpacesAwsRegion = Aws::Region::AP_SOUTHEAST_2;
static const Aws::String AWSAccessKeyId = "AKIA6BPFQACFZTW2QD4K";
static const Aws::String AWSSecretKey = "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";
static const Aws::String DynamoDBStreamsARN = "arn:aws:dynamodb:ap-southeast-2:965238325387:table/mojexa-markers/stream/2022-03-21T07:34:06.673";
static const Aws::String DynamoDBTableName = "markers";

inline Aws::String FStringToAwsString(const FString& String)
{
	return Aws::String(TCHAR_TO_UTF8(*String));
}

// DynamoDB attribute names
static const FString PartitionKeyAttributeName = "device_id";
static const FString SortKeyAttributeName = "created_timestamp";
static const FString PositionXAttributeName = "longitude";
static const FString PositionYAttributeName = "latitude";
static const FString PositionZAttributeName = "elevation";
static const FString MarkerTypeAttributeName = "marker_type";

static const Aws::String PartitionKeyAttributeNameAws = FStringToAwsString("device_id");
static const Aws::String SortKeyAttributeNameAws = FStringToAwsString("created_timestamp");
static const Aws::String PositionXAttributeNameAws = FStringToAwsString("longitude");
static const Aws::String PositionYAttributeNameAws = FStringToAwsString("latitude");
static const Aws::String PositionZAttributeNameAws = FStringToAwsString("elevation");
static const Aws::String MarkerTypeAttributeNameAws = FStringToAwsString("marker_type");

static const bool UseDynamoDBLocal = true;
static const Aws::String DynamoDBLocalEndpoint = "http://localhost:8000";