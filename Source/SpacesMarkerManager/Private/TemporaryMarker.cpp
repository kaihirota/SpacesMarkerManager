#include "TemporaryMarker.h"


// Sets default values
ATemporaryMarker::ATemporaryMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetColor(DefaultColor);
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
	
	float Scale = Counter / InitialCounter;
	if (Scale > 2.0f)
	{
		Scale = 2.0f;
	}
	SetActorRelativeScale3D(FVector(Scale, Scale, Scale));

	if (Counter <= 0)
	{
		Destroy();
	}
}

void ATemporaryMarker::IncrementCounter(const int Count)
{
	Counter += Count;
}

void ATemporaryMarker::DecrementCounter(const int Count)
{
	Counter -= Count;
}