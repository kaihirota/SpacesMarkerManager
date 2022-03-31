// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject : ModuleRules
{
	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "HTTP", "Json", "JsonUtilities", "AWSCoreLibrary", "DynamoDBClientLibrary", "DynamoDBStreamsClientLibrary"});
		PrivateDependencyModuleNames.AddRange(new string[] { "JsonUtilities" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	}
}
