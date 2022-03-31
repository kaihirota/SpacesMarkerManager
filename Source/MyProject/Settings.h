#pragma once

#include "CoreMinimal.h"
#include "aws/core/Region.h"
#include "aws/core/utils/memory/stl/AWSString.h"


const char* REGION = Aws::Region::AP_SOUTHEAST_2;
const FString MarkerAPIEndpoint = "https://0flacbdme3.execute-api.ap-southeast-2.amazonaws.com/dev/";
const Aws::String DynamoDBStreamsARN = "arn:aws:dynamodb:ap-southeast-2:965238325387:table/mojexa-markers/stream/2022-03-21T07:34:06.673";
const Aws::String AWSAccessKeyId = "AKIA6BPFQACFZTW2QD4K";
const Aws::String AWSSecretKey = "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";
