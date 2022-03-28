// Copyright(c) Splash Damage. All rights reserved.
using UnrealBuildTool;

public class FutureExtensionsTest : ModuleRules
{
	public FutureExtensionsTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "Automatron",
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
			"FutureExtensions"
		});
	}
}
