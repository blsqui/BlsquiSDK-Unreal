#include "Modules/ModuleManager.h"

class FBlsquiSDKModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // Executed when the plugin is loaded into memory
    }

    virtual void ShutdownModule() override
    {
        // Executed when the plugin is unloaded
    }
};

IMPLEMENT_MODULE(FBlsquiSDKModule, BlsquiSDK)