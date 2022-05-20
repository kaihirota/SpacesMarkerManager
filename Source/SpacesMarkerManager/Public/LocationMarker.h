// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "CesiumGlobeAnchorComponent.h"
#include "LocationTs.h"
#include "Settings.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.generated.h"


UCLASS(BlueprintType, Blueprintable)
class SPACESMARKERMANAGER_API ALocationMarker : public AActor
{
public:
	GENERATED_BODY()

	// Sets default values for this actor's properties
	ALocationMarker();

	FString ClassName = FString("StaticMarker");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	FColor BaseColor = FColor::Turquoise;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	float DefaultRadius = 200.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	bool DeleteFromDBOnDestroy = false;

	DECLARE_DELEGATE_ThreeParams(FLocationMarkerOnDelete, FString, FDateTime, bool);
	FLocationMarkerOnDelete MarkerOnDelete;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	bool Selected = false;
	
	/* Static Mesh Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	UStaticMeshComponent* StaticMeshComp;

	/* Sphere Component (not static mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	class USphereComponent* SphereComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	UCesiumGlobeAnchorComponent* CesiumGlobeAnchor;

	/* Emissive material */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spaces")
	UMaterialInterface* EmissiveMatInterface;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	UMaterialInstanceDynamic* DynamicMaterial;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	FString DeviceID;
	
	/* Location and timestamp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces")
	FLocationTs LocationTs;

	/**
	* Select or unselect this marker
	* @returns Selected [bool] The value of the selected state 
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces")
	bool ToggleSelection();
	
	UFUNCTION(BlueprintCallable, Category="Spaces")
	FLinearColor GetColor() const;
	
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void SetColor(const FLinearColor Color) const;

	UFUNCTION(BlueprintCallable, Category="Spaces")
	float GetOpacity() const;
	
	UFUNCTION(BlueprintCallable, Category="Spaces")
	void SetOpacity(float OpacityVal) const;
	
	UFUNCTION(BlueprintCallable, Category="Spaces")
	virtual FString ToString() const;

	UFUNCTION(BlueprintCallable, Category="Spaces")
	virtual FString ToJsonString() const;

	virtual TSharedRef<FJsonObject> ToJsonObject() const;

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called before destroying the object. 
	// This is called immediately upon deciding to destroy the object, to allow the object to begin an asynchronous cleanup process.
	virtual void BeginDestroy() override;
};

inline bool operator==(const ALocationMarker& Marker1, const ALocationMarker& Marker2)
{
	return (Marker1.DeviceID == Marker2.DeviceID &&
		Marker1.LocationTs.Wgs84Coordinate == Marker1.LocationTs.Wgs84Coordinate &&
		Marker1.LocationTs.Timestamp == Marker2.LocationTs.Timestamp);
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

	if (!DeviceID.IsEmpty()) JsonObject->SetStringField(PartitionKeyAttributeName, DeviceID);
	else JsonObject->SetStringField(PartitionKeyAttributeName, FString(""));
	JsonObject->SetStringField(SortKeyAttributeName, FString::FromInt(Timestamp.ToUnixTimestamp()));
	JsonObject->SetStringField(PositionXAttributeName, FString::SanitizeFloat(Coordinate.X));
	JsonObject->SetStringField(PositionYAttributeName, FString::SanitizeFloat(Coordinate.Y));
	JsonObject->SetStringField(PositionZAttributeName, FString::SanitizeFloat(Coordinate.Z));
	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject, Writer);
	const uint32 Hash = FCrc::StrCrc32(*OutputString, sizeof(OutputString));
	return Hash;
}
