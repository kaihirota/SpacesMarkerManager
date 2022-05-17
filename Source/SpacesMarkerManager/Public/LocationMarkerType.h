#pragma once

#include "CoreMinimal.h"
#include "LocationMarkerType.generated.h"

/**
 * Static: Location & size are fixed after being initialized
 * Temporary: Size can be set as a function of time
 * Dynamic: Location & size can be changed
 */
UENUM()
enum class ELocationMarkerType : uint8
{
	Static,
	Temporary,
	Dynamic
};