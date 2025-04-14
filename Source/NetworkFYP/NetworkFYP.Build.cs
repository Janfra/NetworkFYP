// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NetworkFYP : ModuleRules
{
	public NetworkFYP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", 
			"InputCore", "EnhancedInput", "UMG", "NetCore",
            "OnlineServicesInterface", "OnlineServicesEOSGS", "CoreOnline" });

		/* Sets the game to be in P2P mode instead of dedicated server */
		PrivateDefinitions.Add("P2PMODE=1");
	}
}
