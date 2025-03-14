// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NetworkFYPTarget : TargetRules
{
	public NetworkFYPTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		/* Can't modify the properties and package without source engine */
		//bWithPushModel = true;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("NetworkFYP");
	}
}
