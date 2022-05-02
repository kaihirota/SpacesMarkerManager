// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicMarker.h"
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
	NextLocation = GetActorLocation();
}

void ADynamicMarker::EnqueueLocation(const FVector Location)
{
	CoordinateQueue.Enqueue(Location);
}

// Called every frame
void ADynamicMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetActorLocation() == NextLocation)
	{
		if (CoordinateQueue.Dequeue(NextLocation))
		{
			UE_LOG(LogTemp, Warning, TEXT("Dynamic Marker %s at %s, next stop %s"), *DeviceID, *GetActorLocation().ToString(), *NextLocation.ToString());
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Dynamic Marker %s reached final destination %s"), *DeviceID, *GetActorLocation().ToString());
		}
	} else
	{
		Step = FMath::VInterpConstantTo(
			GetActorLocation(),
			NextLocation,
			DeltaTime,
			InterpolationsPerSecond);
		SetActorLocation(Step, true, nullptr, ETeleportType::None);
		UE_LOG(LogTemp, Warning, TEXT("Dynamic Marker %s %s %s"), *DeviceID, *Step.ToString(), *NextLocation.ToString());
	}
}
