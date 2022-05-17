// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;


public class SpacesMarkerManager : ModuleRules
{
	public SpacesMarkerManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HTTP", "Json", "JsonUtilities", "AWSCoreLibrary", "DynamoDBClientLibrary", "DynamoDBStreamsClientLibrary"}); 
		PrivateDependencyModuleNames.AddRange(new string[] { "JsonUtilities", "CesiumRuntime"});
	}
}
