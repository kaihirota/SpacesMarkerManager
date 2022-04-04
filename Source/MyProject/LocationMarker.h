// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.generated.h"

UCLASS()
// class SPACES_ACT_API ALocationMarker : public AActor
class MYPROJECT_API ALocationMarker : public AActor
{
public:
	GENERATED_BODY()
	
	// Sets default values for this actor's properties
	ALocationMarker();
	void ToggleSelection();

	/* Static Mesh Component */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComp;

	/* Sphere Component (not static mesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USphereComponent* SphereComp;

	/* Emissive material */
	UPROPERTY(EditDefaultsOnly, Category=Default)
	UMaterialInterface* EmissiveMatInterface;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector Coordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Wgs84Coordinate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime Timestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString DeviceID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool Selected;

	UFUNCTION(BlueprintCallable)
	void SetColor(const FLinearColor Color) const;
	FLinearColor GetColor() const;
	void SetOpacity(float Val) const;
	float GetOpacity() const;

	UFUNCTION(BlueprintCallable)
	FString ToString() const;
	
	FString ToJsonString() const;

	TSharedRef<FJsonObject> ToJsonObject() const;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called before destroying the object. 
	// This is called immediately upon deciding to destroy the object, to allow the object to begin an asynchronous cleanup process.
    virtual void BeginDestroy() override;
};

inline bool operator== (const ALocationMarker& Marker1, const ALocationMarker& Marker2)
{
	return (Marker1.DeviceID == Marker2.DeviceID &&
			Marker1.Coordinate == Marker1.Coordinate &&
			Marker1.Timestamp == Marker2.Timestamp);
}

inline uint32 GetTypeHash(const ALocationMarker& Thing)
{
	FString s = Thing.ToJsonString();
	const uint32 Hash = FCrc::StrCrc32(*s, sizeof(s));
	return Hash;
}

inline uint32 GetTypeHash(const FVector Coordinate, const FDateTime Timestamp, const FString DeviceID)
{
	const TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if (!DeviceID.IsEmpty()) JsonObject->SetStringField("device_id", DeviceID);
	else JsonObject->SetStringField("device_id", FString(""));
	JsonObject->SetNumberField("created_timestamp", Timestamp.ToUnixTimestamp());
	JsonObject->SetStringField("longitude", FString::SanitizeFloat(Coordinate.X));
	JsonObject->SetStringField("latitude", FString::SanitizeFloat(Coordinate.Y));
	JsonObject->SetStringField("elevation", FString::SanitizeFloat(Coordinate.Z));

	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject, Writer);
	const uint32 Hash = FCrc::StrCrc32(*OutputString, sizeof(OutputString));
	return Hash;
}