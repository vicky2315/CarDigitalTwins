// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CarDigitalTwins : ModuleRules
{
	public CarDigitalTwins(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets code include by folder from the module root, e.g. "MVVM/ViewModelBase.h".
		PublicIncludePaths.Add(ModuleDirectory);

		// DeveloperSettings is public: UTelemetrySettings (a UDeveloperSettings subclass) sits in a public header.
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "DeveloperSettings" });

		// Json + JsonUtilities: UFileTelemetryReceiver parses the recorded trip with FJsonObjectConverter (SPEC.md §2.2).
		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
