// Fill out your copyright notice in the Description page of Project Settings.


#include "TemporaryMarker.h"


// Sets default values
ATemporaryMarker::ATemporaryMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetColor(FColor::Cyan);
}

// Called when the game starts or when spawned
void ATemporaryMarker::BeginPlay()
{
	Super::BeginPlay();
	
	// Call RepeatingFunction once per second, starting two seconds from now.
	// GetWorldTimerManager().SetTimer(TimerHandle, this, &ATemporaryMarker::RepeatingFunction, 0.1f, true, 0.0f);
}

// Called every frame
void ATemporaryMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	DecrementCounter(1);
	float Scale = Counter / InitialCounter;
	if (Scale > 2.0f)
	{
		Scale = 2.0f;
	}
	SetActorRelativeScale3D(FVector(Scale, Scale, Scale));
	// UE_LOG(LogTemp, Warning, TEXT("TemporaryMarker %s counter %d"), *GetName() , Counter);
	
	if (Counter <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Destroying %s - %s"), *GetName(), *ToString());
		Destroy();
	}
}

void ATemporaryMarker::SetOpacity(const float Opacity) const
{
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
}

void ATemporaryMarker::IncrementCounter(const int Count)
{
	Counter += Count;
}

void ATemporaryMarker::IncrementCounter()
{
	IncrementCounter(InitialCounter / 5);
}

void ATemporaryMarker::DecrementCounter(const int Count)
{
	Counter -= Count;
}

void ATemporaryMarker::DecrementCounter()
{
	DecrementCounter(1);
}

void ATemporaryMarker::RepeatingFunction()
{
	DecrementCounter(1);
}