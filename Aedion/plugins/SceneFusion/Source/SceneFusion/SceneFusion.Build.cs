using UnrealBuildTool;
using System.IO;

public class SceneFusion : ModuleRules
{
    public SceneFusion(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(new string[] {
            "SceneFusion/Private",
            "SceneFusion/Private/UI",
            "SceneFusion/Private/Web"
        });

        PublicIncludePaths.AddRange(new string[] {
            "SceneFusion/Public"
        });

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate", "SlateCore", "EditorStyle", "Kismet", "LevelEditor", "Projects", "Paper2D",
            "Http", "Json", "JsonUtilities", "AppFramework", "RawMesh", "VREditor", "UnrealEd",
            "Landscape"
        });

        // Include SceneFusionAPI files
        string path = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/SceneFusionAPI"));
        PublicAdditionalLibraries.AddRange(new string[] {
            Path.Combine(path, "Libraries/SceneFusionAPI.lib"),
            Path.Combine(path, "Libraries/KSCommon.lib"),
            Path.Combine(path, "Libraries/KSNetworking.lib"),
            Path.Combine(path, "Libraries/KSLZMA.lib"),
            Path.Combine(path, "Libraries/Reactor.lib")
        });

        PublicIncludePaths.Add(Path.Combine(path, "Includes"));
    }
}