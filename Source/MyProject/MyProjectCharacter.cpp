// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectCharacter.h"

#include "DrawDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "LocationMarker.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "TemporaryMarker.h"

// DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AMyProjectCharacter

AMyProjectCharacter::AMyProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));
}

void AMyProjectCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	MarkerManager = GetWorld()->GetGameInstance<UMojexaSpacesMarkerManager>();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("CreateLocationMarker", IE_Pressed, this,
	                                 &AMyProjectCharacter::CreateLocationMarker);
	PlayerInputComponent->BindAction("CreateTemporaryMarker", IE_Pressed, this,
	                                 &AMyProjectCharacter::CreateTemporaryMarker);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("GetMarkers", IE_Pressed, this, &AMyProjectCharacter::GetMarkers);
	PlayerInputComponent->BindAction("RemoveSelectedMarkers", IE_Pressed, this, &AMyProjectCharacter::RemoveSelectedMarkers);

	PlayerInputComponent->BindAction("DynamoDBStreamsReplay", IE_Pressed, this, &AMyProjectCharacter::DynamoDBStreamsReplay);
	PlayerInputComponent->BindAction("DynamoDBStreamsListen", IE_Pressed, this, &AMyProjectCharacter::DynamoDBStreamsListen);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMyProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMyProjectCharacter::LookUpAtRate);
}

void AMyProjectCharacter::CreateLocationMarker()
{
	FHitResult OutHit;
	FVector Start = GetActorLocation();
	FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	FVector End = (ForwardVector * FVector(1000000.f, 1000000.f, 1000000.f)) + Start;
	FCollisionQueryParams CollisionParams;

	DrawDebugLine(GetWorld(), Start, End, FColor::Emerald, false, 1, 0, 1);

	if (GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, CollisionParams))
	{
		if (OutHit.bBlockingHit)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.f,
					FColor::Emerald,
					FString::Printf(
						TEXT("You are hitting: %s at %s"),
						*OutHit.GetActor()->GetName(),
						*OutHit.ImpactPoint.ToString()
					)
				);
			}
			if (ALocationMarker* Marker = Cast<ALocationMarker>(OutHit.GetActor()))
			{
				Marker->ToggleSelection();
			}
			else if (MarkerManager != nullptr)
			{
				Marker = MarkerManager->CreateMarker(OutHit.ImpactPoint, ELocationMarkerType::Static);
				MarkerManager->CreateMarkerInDB(Marker);
			}
		}
	}
}

void AMyProjectCharacter::CreateTemporaryMarker()
{
	FHitResult OutHit;
	FVector Start = GetActorLocation();
	FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();
	FVector End = ((ForwardVector * FVector(1000000.f, 1000000.f, 1000000.f)) + Start);
	FCollisionQueryParams CollisionParams;

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1, 0, 1);

	if (GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, CollisionParams))
	{
		if (OutHit.bBlockingHit)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.f,
					FColor::Red,
					FString::Printf(
						TEXT("You are hitting: %s at %s"),
						*OutHit.GetActor()->GetName(),
						*OutHit.ImpactPoint.ToString()
					)
				);
			}
			// TODO: Move this if block into CreateMarkerFromUECoordinate?
			// But MarkerManager does not have reference to FHitResult or "OutHit"
			if (ATemporaryMarker* HitMarker = Cast<ATemporaryMarker>(OutHit.GetActor()))
			{
				HitMarker->IncrementCounter(50);
			}
			else
			{
				if (MarkerManager != nullptr)
				{
					MarkerManager->CreateMarker(OutHit.ImpactPoint, ELocationMarkerType::Temporary);
				}
			}
		}
	}
}


void AMyProjectCharacter::GetMarkers()
{
	if (MarkerManager != nullptr)
	{
		MarkerManager->GetMarkersFromDB();
	}
}

void AMyProjectCharacter::RemoveSelectedMarkers()
{
	if (MarkerManager != nullptr)
	{
		MarkerManager->DeleteSelectedMarkers();
	}
}

Aws::Vector<Aws::DynamoDBStreams::Model::Stream> AMyProjectCharacter::GetStreams()
{
	Aws::Vector<Aws::DynamoDBStreams::Model::Stream> Streams;
	if (MarkerManager != nullptr)
	{
		Streams = MarkerManager->GetStreams(DynamoDBTableName);
		if (Streams.size() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ListStreams error: no streams were found"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Found %d Streams of DynamoDB table %s"), Streams.size(),
				   *FString(DynamoDBTableName.c_str()));
		}
	}
	return Streams;
}

void AMyProjectCharacter::DynamoDBStreamsReplay()
{
	const Aws::Vector<Aws::DynamoDBStreams::Model::Stream> Streams = GetStreams();
	for (const auto Stream : Streams)
	{
		MarkerManager->ScanStream(Stream, Aws::DynamoDBStreams::Model::ShardIteratorType::TRIM_HORIZON, FDateTime::Now() - FTimespan::FromHours(24.0));
	}
}

void AMyProjectCharacter::DynamoDBStreamsListen()
{
	if (Listening)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
	} else
	{
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyProjectCharacter::DynamoDBStreamsListen_, PollingInterval, true, 0.0f);
	}
	Listening = !Listening;
}

void AMyProjectCharacter::DynamoDBStreamsListen_()
{
	const Aws::Vector<Aws::DynamoDBStreams::Model::Stream> Streams = GetStreams();
	if (Streams.size() > 0)
	{
		const Aws::DynamoDBStreams::Model::Stream Stream = Streams[0];
		const Aws::Vector<Aws::DynamoDBStreams::Model::Shard> Shards = MarkerManager->GetShards(Stream.GetStreamArn());
		if (Shards.size() > 0)
		{
			MarkerManager->IterateShard(Stream.GetStreamArn(), Shards[0], Aws::DynamoDBStreams::Model::ShardIteratorType::LATEST, NULL);
		}
	}
}

void AMyProjectCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		CreateLocationMarker();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AMyProjectCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AMyProjectCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMyProjectCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMyProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMyProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AMyProjectCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AMyProjectCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AMyProjectCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AMyProjectCharacter::TouchUpdate);
		return true;
	}

	return false;
}
