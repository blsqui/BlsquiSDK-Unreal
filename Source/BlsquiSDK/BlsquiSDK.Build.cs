using UnrealBuildTool;

public class BlsquiSDK : ModuleRules
{
    public BlsquiSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "HTTP",          // Needed for Local Loopback & Cloudflare Worker requests
                "Json",          // Needed for parsing transaction status responses
                "JsonUtilities",
                "UMG",           // Needed for displaying UI Dialogs
                "Slate",
                "SlateCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // Put any private internal module dependencies here if needed
            }
        );
    }
}