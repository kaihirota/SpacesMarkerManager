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
	PreviousLocation = GetActorLocation();
	NextLocation = GetActorLocation();

	GetWorld()->GetTimerManager()
	          .SetTimer(TimerHandle, this, &ADynamicMarker::RepeatingFunction, 1.0f, true, 1.0f);
}

void ADynamicMarker::UpdateLocation(const FVector Location)
{
	PreviousLocation = GetActorLocation();
	this->NextLocation = Location;
}

// Called every frame
void ADynamicMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (PreviousLocation != NextLocation)
	{
		Step = FMath::VInterpConstantTo(GetActorLocation(), this->NextLocation, DeltaTime, InterpolationsPerSecond);
		SetActorLocation(Step, true, nullptr, ETeleportType::None);
	}
}

void ADynamicMarker::RepeatingFunction()
{
	if (ManagerDelegate.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("Requesting new location for : %s at %s"), *this->ToString(),
		       *GetActorLocation().ToString());
		const FVector NewLocation = ManagerDelegate.Execute(this->DeviceID, this->Timestamp);
		if (NewLocation != FVector::ZeroVector)
		{
			UE_LOG(LogTemp, Warning, TEXT("Marker: %s Current loc: %s Next Location: %s"), *this->ToString(),
			       *GetActorLocation().ToString(), *NextLocation.ToString());
			if (NewLocation != GetActorLocation())
			{
				// SetActorLocation(NextLocation, true, nullptr, ETeleportType::None);
				UpdateLocation(NewLocation);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Location not updated for marker %s: Current loc: %s"), *this->ToString(),
			       *GetActorLocation().ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No reference to MarkerManager found"));
	}
}
