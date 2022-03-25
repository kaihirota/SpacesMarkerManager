// Fill out your copyright notice in the Description page of Project Settings.


#include "TemporaryMarker.h"


// Sets default values
ATemporaryMarker::ATemporaryMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetColor(FColor::Red);
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
	Counter--;
	const float Scale = Counter / InitialCounter;
	if (Counter <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Destroying %s - %s"), *GetName(), *ToString());
		Destroy();
	}
	else SetActorRelativeScale3D(FVector(Scale, Scale, Scale));
	// else SetOpacity(Counter / 1000.0f);
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
