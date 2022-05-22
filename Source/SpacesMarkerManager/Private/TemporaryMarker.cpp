#include "TemporaryMarker.h"


DEFINE_LOG_CATEGORY(LogTemporaryMarker);

ATemporaryMarker::ATemporaryMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	Super::MarkerType = ELocationMarkerType::Temporary;
	Super::BaseColor = TemporaryMarkerColor;
	Super::DeleteFromDBOnDestroy = false;
	SetColor(BaseColor);
}

// Called when the game starts or when spawned
void ATemporaryMarker::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(DefaultLifeSpan);
}

// Called every frame
void ATemporaryMarker::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetLifeSpan() > 0)
	{
		const float Scale = FGenericPlatformMath::Min(MaxScale, (GetLifeSpan() + DeltaTime) / InitialLifeSpan);
		SetActorRelativeScale3D(FVector(Scale, Scale, Scale));
	}
}