#include "BlsquiSDKModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

#define LOCTEXT_NAMESPACE "FBlsquiSDKModule"

static void* LibDataChannelHandle = nullptr;

void FBlsquiSDKModule::StartupModule()
{
#if PLATFORM_MAC
    // Resolve absolute path to bundled libdatachannel.dylib
    FString BaseDir = IPluginManager::Get().FindPlugin("BlsquiSDK")->GetBaseDir();
    FString LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/BlsquiSDK/ThirdParty/libdatachannel/lib/Mac/libdatachannel.dylib"));

    LibDataChannelHandle = FPlatformProcess::GetDllHandle(*LibraryPath);
    if (LibDataChannelHandle)
    {
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Successfully loaded libdatachannel.dylib from: %s"), *LibraryPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[BlsquiSDK] Failed to load libdatachannel.dylib from: %s"), *LibraryPath);
    }
#endif
}

void FBlsquiSDKModule::ShutdownModule()
{
#if PLATFORM_MAC
    if (LibDataChannelHandle)
    {
        FPlatformProcess::FreeDllHandle(LibDataChannelHandle);
        LibDataChannelHandle = nullptr;
    }
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlsquiSDKModule, BlsquiSDK)
