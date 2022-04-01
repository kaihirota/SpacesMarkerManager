// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicMarker.h"

#include "MarkerManager.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ADynamicMarker::ADynamicMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetColor(FColor::Purple);
}

// Called when the game starts or when spawned
void ADynamicMarker::BeginPlay()
{
	Super::BeginPlay();
	// GetWorld()->GetTimerManager()
	// 	.SetTimer(TimerHandle, this, &ADynamicMarker::RepeatingFunction, 1.0f, true, 1.0f);
}

// Called every frame
void ADynamicMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// void ADynamicMarker::RepeatingFunction()
// {
// 	UE_LOG(LogTemp, Warning, TEXT("Hello"));
// 	FMarkerManagerDelegate MarkerManagerDelegate;
// 	
// 	MarkerManagerDelegate.BindRaw(&AMarkerManager::GetLatestRecord, DeviceID, Timestamp);
// 	
// 	AActor* MarkerManagerActor = UGameplayStatics::GetActorOfClass(GetWorld(), ALocationMarker::StaticClass());
// 	MarkerManager = Cast<AMarkerManager>(MarkerManagerActor);
// 	if (MarkerManager != nullptr)
// 	{
// 		FVector LatestLocation = MarkerManager->GetLatestRecord(this->DeviceID, this->Timestamp);
// 		if (LatestLocation != FVector::ZeroVector)
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("Latest Location: %s  Curr Location: %s"), *LatestLocation.ToString(), *GetActorLocation().ToString());
// 			if (LatestLocation != GetActorLocation())
// 			{
// 				SetActorLocation(LatestLocation, true, nullptr, ETeleportType::None);
// 			}
// 		}
// 	} else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("No reference to MarkerManager found"));
// 	}
// }
