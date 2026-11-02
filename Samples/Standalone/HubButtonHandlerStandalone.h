#pragma once

#include <string>
#include <functional>

struct StandaloneUserData
{
    std::string Address;
    std::string Network = "testnet";
    bool bIsLoggedIn = false;
};

class HubButtonHandlerStandalone
{
public:
    using OpenUrlCallback = std::function<void(const std::string& Url)>;

    HubButtonHandlerStandalone(OpenUrlCallback InUrlOpener = nullptr);

    /** Called when the player clicks the HUB button */
    void OnClickHub(const StandaloneUserData& CurrentUser);

    /** Builds the target URL including player parameters */
    std::string BuildHubUrl(const StandaloneUserData& CurrentUser) const;

    // Base endpoints
    std::string HubBaseUrlTestnet = "https://blsqui.net/dashboard";
    std::string HubBaseUrlMainnet = "https://blsqui.net/dashboard";
    std::string AppId = "valkyrie_vanguard";

private:
    OpenUrlCallback OpenUrlFn;

    /** Fallback system launcher if no engine/game callback is provided */
    static void LaunchSystemBrowser(const std::string& Url);
};
