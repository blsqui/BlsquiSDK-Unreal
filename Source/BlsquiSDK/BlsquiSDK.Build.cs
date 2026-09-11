using UnrealBuildTool;

public class BlsquiSDK : ModuleRules
{
    public BlsquiSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Exposed publicly through BlsquiSDKComponent.h
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "HTTP"
            }
        );

        // Used internally inside BlsquiSDKComponent.cpp
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Json",
                "JsonUtilities"
            }
        );
    }
}