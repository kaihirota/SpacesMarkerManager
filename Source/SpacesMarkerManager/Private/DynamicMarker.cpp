// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicMarker.h"

#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogDynamicMarker);

// Sets default values
ADynamicMarker::ADynamicMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.01f;
	Super::MarkerType = ELocationMarkerType::Dynamic;
	Super::BaseColor = DynamicMarkerColor;
	Super::DeleteFromDBOnDestroy = false;
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
	SetLifeSpan(0);
}

void ADynamicMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetActorLocation() == LocationTs.UECoordinate)
	{
		if (idx >= 0 && idx + 1 < HistoryArr.Num())
		{
			if (DeltaTime > 0) idx++;
			else if (DeltaTime < 0 && idx > 0) idx--;
			LocationTs = HistoryArr[idx];
			if (idx + 1 == HistoryArr.Num())
			{
				UE_LOG(LogDynamicMarker, Display, TEXT("DynamicMarker %s reached final location. Will be destroyed in %f seconds."), *DeviceID, DefaultLifeSpan);
				SetLifeSpan(DefaultLifeSpan);
			} 
		}
	}
	else
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
