﻿#include "TemporaryMarker.h"


DEFINE_LOG_CATEGORY(LogTemporaryMarker);

ATemporaryMarker::ATemporaryMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Super::ClassName = TemporaryMarkerName;
	Super::BaseColor = TemporaryMarkerColor;
	SetColor(BaseColor);
}

// Called when the game starts or when spawned
void ATemporaryMarker::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATemporaryMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime > 0) DecrementCounter(CounterTickSize);
	else IncrementCounter(CounterTickSize);

	const float Scale = FGenericPlatformMath::Min(MaxScale, Counter / InitialCounter);
	SetActorRelativeScale3D(FVector(Scale, Scale, Scale));

	if (Counter <= 0) Destroy();
}

void ATemporaryMarker::IncrementCounter(const int Count)
{
	Counter += Count;
}

void ATemporaryMarker::DecrementCounter(const int Count)
{
	Counter -= Count;
}