using System.IO;
using UnrealBuildTool;

public class BlsquiSDK : ModuleRules
{
    public BlsquiSDK(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        string ThirdPartyPath = Path.Combine(ModuleDirectory, "ThirdParty", "libdatachannel");

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Public"),
                Path.Combine(ThirdPartyPath, "include")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Private")
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "HTTP",
                "Json",
                "JsonUtilities",
                "Projects"
            }
        );

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string DylibPath = Path.Combine(ThirdPartyPath, "lib", "Mac", "libdatachannel.dylib");
            if (File.Exists(DylibPath))
            {
                PublicAdditionalLibraries.Add(DylibPath);
                RuntimeDependencies.Add(DylibPath);
            }

            PublicFrameworks.AddRange(
                new string[] {
                    "Security",
                    "CoreFoundation"
                }
            );
        }
    }
}
