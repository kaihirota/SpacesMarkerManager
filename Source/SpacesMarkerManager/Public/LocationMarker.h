// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "CesiumGlobeAnchorComponent.h"
#include "LocationTs.h"
#include "Settings.h"
#include "GameFramework/Actor.h"
#include "LocationMarker.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogLocationMarker, Display, All);

/**
 * Static: Location & size are fixed after being initialized
 * Temporary: Size can be set as a function of time
 * Dynamic: Location & size can be changed
 */
UENUM(BlueprintType, Category="Spaces|Marker")
enum class ELocationMarkerType : uint8
{
	Static		UMETA(DisplayName = StaticMarkerName),
	Temporary	UMETA(DisplayName = TemporaryMarkerName),
	Dynamic		UMETA(DisplayName = DynamicMarkerName)
};


UCLASS(BlueprintType, Blueprintable)
class SPACESMARKERMANAGER_API ALocationMarker : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALocationMarker();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	ELocationMarkerType MarkerType = ELocationMarkerType::Static;

	// appearance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker")
	FColor BaseColor = FColor::Turquoise;

	/* Static Mesh Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	UStaticMeshComponent* StaticMeshComp;

	/* Sphere Component (not static mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	class USphereComponent* SphereComp;

	/* Emissive material */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	UMaterialInterface* EmissiveMatInterface;

	/* Emissive material instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker")
	UMaterialInstanceDynamic* DynamicMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker")
	UCesiumGlobeAnchorComponent* CesiumGlobeAnchor;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Spaces|Marker")
	float DefaultRadius = 200.0f;
	
	// properties
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	FString DeviceID;
	
	/* Location and timestamp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	FLocationTs LocationTs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spaces|Marker")
	bool Selected = false;

	/* When DeleteFromDBOnDestroy is True, the marker will be deleted from the database upon destruction*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spaces|Marker")
	bool DeleteFromDBOnDestroy = false;

	DECLARE_DELEGATE_ThreeParams(FLocationMarkerOnDelete, FString, FDateTime, bool);
	FLocationMarkerOnDelete MarkerOnDelete;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	/* Called before destroying the object.
	 * This is called immediately upon deciding to destroy the object,
	 * to allow the object to begin an asynchronous cleanup process.
	 */ 
	virtual void BeginDestroy() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	/**
	* Select or unselect this marker
	* @returns Selected [bool] The value of the selected state 
	**/
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	bool ToggleSelection();
	
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	FLinearColor GetColor() const;
	
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	void SetColor(const FLinearColor Color) const;

	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	float GetOpacity() const;
	
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	void SetOpacity(float OpacityVal) const;
	
	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	virtual FString ToString() const;

	UFUNCTION(BlueprintCallable, Category="Spaces|Marker")
	virtual FString ToJsonString() const;

	virtual TSharedRef<FJsonObject> ToJsonObject() const;
};

inline bool operator==(const ALocationMarker& Marker1, const ALocationMarker& Marker2)
{
	return (Marker1.DeviceID == Marker2.DeviceID &&
		Marker1.LocationTs.UECoordinate == Marker1.LocationTs.UECoordinate &&
		Marker1.LocationTs.Timestamp == Marker2.LocationTs.Timestamp);
}

// inline uint32 GetTypeHash(const ALocationMarker& Thing)
// {
// 	FString s = Thing.ToJsonString();
// 	const uint32 Hash = FCrc::StrCrc32(*s, sizeof(s));
// 	return Hash;
// }
//
// inline uint32 GetTypeHash(const FVector Coordinate, const FDateTime Timestamp, const FString DeviceID)
// {
// 	const TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
//
// 	if (!DeviceID.IsEmpty()) JsonObject->SetStringField(PartitionKeyAttributeName, DeviceID);
// 	else JsonObject->SetStringField(PartitionKeyAttributeName, FString(""));
// 	JsonObject->SetStringField(SortKeyAttributeName, FString::FromInt(Timestamp.ToUnixTimestamp()));
// 	JsonObject->SetStringField(PositionXAttributeName, FString::SanitizeFloat(Coordinate.X));
// 	JsonObject->SetStringField(PositionYAttributeName, FString::SanitizeFloat(Coordinate.Y));
// 	JsonObject->SetStringField(PositionZAttributeName, FString::SanitizeFloat(Coordinate.Z));
// 	FString OutputString;
// 	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
// 	FJsonSerializer::Serialize(JsonObject, Writer);
// 	const uint32 Hash = FCrc::StrCrc32(*OutputString, sizeof(OutputString));
// 	return Hash;
// }
