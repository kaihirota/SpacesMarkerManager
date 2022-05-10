#pragma once

#include "CoreMinimal.h"
#include "aws/core/Region.h"
#include "aws/core/utils/memory/stl/AWSString.h"


static const Aws::String AWSAccessKeyId = "AKIA6BPFQACFZTW2QD4K";
static const Aws::String AWSSecretKey = "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";
static const Aws::String DynamoDBLocalEndpoint = "http://localhost:8000";
static const char* SpacesAwsRegion = Aws::Region::AP_SOUTHEAST_2;
static const bool UseDynamoDBLocal = true;
static const double PollingInterval = 2.0f;

inline Aws::String FStringToAwsString(const FString& String)
{
	return Aws::String(TCHAR_TO_UTF8(*String));
}

inline FString AwsStringToFString(const Aws::String& String)
{
	return FString(String.c_str());
}

// DynamoDB attribute names
static const FString DynamoDBTableName = "mojexa-markers";
static const FString PartitionKeyAttributeName = "device_id";
static const FString SortKeyAttributeName = "created_timestamp";
static const FString PositionXAttributeName = "longitude";
static const FString PositionYAttributeName = "latitude";
static const FString PositionZAttributeName = "elevation";
static const FString MarkerTypeAttributeName = "marker_type";

static const Aws::String DynamoDBTableNameAws = FStringToAwsString(DynamoDBTableName);
static const Aws::String PartitionKeyAttributeNameAws = FStringToAwsString(PartitionKeyAttributeName);
static const Aws::String SortKeyAttributeNameAws = FStringToAwsString(SortKeyAttributeName);
static const Aws::String PositionXAttributeNameAws = FStringToAwsString(PositionXAttributeName);
static const Aws::String PositionYAttributeNameAws = FStringToAwsString(PositionYAttributeName);
static const Aws::String PositionZAttributeNameAws = FStringToAwsString(PositionZAttributeName);
static const Aws::String MarkerTypeAttributeNameAws = FStringToAwsString(MarkerTypeAttributeName);

