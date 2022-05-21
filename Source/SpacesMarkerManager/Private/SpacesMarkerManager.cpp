// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpacesMarkerManager.h"


#define LOCTEXT_NAMESPACE "FSpacesMarkerManager"

void FSpacesMarkerManager::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(MojexaSpaces, Display, TEXT("Starting FSpacesMarkerManagerModule Module..."))
}


void FSpacesMarkerManager::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(MojexaSpaces, Display, TEXT("Shutting down FSpacesMarkerManagerModule Module..."))
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSpacesMarkerManager, SpacesMarkerManager)
