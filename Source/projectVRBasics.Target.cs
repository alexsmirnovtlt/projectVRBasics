// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class projectVRBasicsTarget : TargetRules
{
	public projectVRBasicsTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "projectVRBasics" } );

		ProjectDefinitions.Add("UE4_PROJECT_STEAMSHIPPINGID=480");
	}
}
