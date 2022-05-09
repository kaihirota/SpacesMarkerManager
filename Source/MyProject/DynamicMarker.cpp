// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicMarker.h"

#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ADynamicMarker::ADynamicMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
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
	LocationTs.Coordinate = GetActorLocation();
}

void ADynamicMarker::AddLocationTs(const FLocationTs Location)
{
	HistoryArr.HeapPush(Location);
}

// Called every frame
void ADynamicMarker::Tick(const float DeltaTime)
{
	if (GetActorLocation() == LocationTs.Coordinate)
	{
		if ((idx >= 0) && (idx < HistoryArr.Num()))
		{
			LocationTs = HistoryArr[idx];
			UE_LOG(LogTemp, Warning,
				TEXT("Dynamic Marker %s at %s, next stop %s"),
				*DeviceID,
				*GetActorLocation().ToString(),
				*LocationTs.Coordinate.ToString());
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
			LocationTs.Coordinate,
			DeltaTime,
			InterpolationsPerSecond);
		SetActorLocation(Step, true, nullptr, ETeleportType::None);
	}
}

FString ADynamicMarker::ToString() const
{
	TArray<FStringFormatArg> Args;
	if (!DeviceID.IsEmpty()) Args.Add(FStringFormatArg(DeviceID));
	else Args.Add(FStringFormatArg(FString("")));
	FString Ret = FString::Format(TEXT("LocationMarker(DeviceID='{0}', History=["), Args);
	
	for (FLocationTs Record : HistoryArr)
	{
		Args.Add(Record.ToString());
	}
	Ret.Append("])");
	return Ret;
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

FString ADynamicMarker::ToJsonString() const
{
	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ToJsonObject(), Writer);
	return OutputString;
}