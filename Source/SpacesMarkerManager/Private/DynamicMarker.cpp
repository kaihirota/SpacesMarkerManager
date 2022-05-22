// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicMarker.h"

#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogDynamicMarker);

// Sets default values
ADynamicMarker::ADynamicMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Super::MarkerType = ELocationMarkerType::Dynamic;
	Super::BaseColor = DynamicMarkerColor;
	SetColor(BaseColor);
}

// Called when the game starts or when spawned
void ADynamicMarker::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		SetReplicates(true);
		SetReplicateMovement(true);
	}
	if (LocationTs.UECoordinate == FVector::ZeroVector)
	{
		LocationTs.UECoordinate = GetActorLocation();
	}
}

void ADynamicMarker::Tick(const float DeltaTime)
{
	if (GetActorLocation() == LocationTs.UECoordinate)
	{
		if ((idx >= 0) && (idx < HistoryArr.Num()))
		{
			LocationTs = HistoryArr[idx];
			UE_LOG(LogDynamicMarker, Display,
				TEXT("Dynamic Marker %s at %s, next stop %s"),
				*DeviceID,
				*GetActorLocation().ToString(),
				*LocationTs.ToString());
			if (DeltaTime > 0) idx++;
			else if (DeltaTime < 0) idx--;
			this->Counter = static_cast<int>(this->InitialCounter);
		} else
		{
			Super::Tick(DeltaTime);
		}
	} else
	{
		const FVector Step = FMath::VInterpConstantTo(
			GetActorLocation(),
			LocationTs.UECoordinate,
			DeltaTime,
			InterpolationsPerSecond);
		SetActorLocation(Step, true, nullptr, ETeleportType::None);
	}
}

void ADynamicMarker::AddLocationTs(const FLocationTs Location)
{
	HistoryArr.HeapPush(Location);
}

FString ADynamicMarker::ToString() const
{
	TArray<FStringFormatArg> Args;
	FString History = FString(", History=[");
	for (FLocationTs Record : HistoryArr)
	{
		History.Append(Record.ToString());
		History.Append(", ");
	}
	History.Append("])");
	
	FString Ret = Super::ToString();
	Ret.InsertAt(Ret.Len()-1, History);
	return Ret;
}

FString ADynamicMarker::ToJsonString() const
{
	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ToJsonObject(), Writer);
	return OutputString;
}

TSharedRef<FJsonObject> ADynamicMarker::ToJsonObject() const
{
	const TSharedRef<FJsonObject> JsonObject = Super::ToJsonObject();
	TArray<TSharedPtr<FJsonValue>> History;
	for (FLocationTs Record : HistoryArr)
	{
		TSharedRef<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
		FJsonObjectConverter::UStructToJsonObject(FLocationTs::StaticStruct(), &Record, JsonObj, 0, 0);
		History.Add(MakeShareable(new FJsonValueObject(JsonObj)));
	}
	JsonObject->SetArrayField("history", History);
	return JsonObject;
}
