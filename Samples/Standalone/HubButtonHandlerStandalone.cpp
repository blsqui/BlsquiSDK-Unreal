#include "HubButtonHandlerStandalone.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

#if defined(_WIN32)
    #include <windows.h>
    #include <shellapi.h>
#endif

HubButtonHandlerStandalone::HubButtonHandlerStandalone(OpenUrlCallback InUrlOpener)
    : OpenUrlFn(std::move(InUrlOpener))
{
}

std::string HubButtonHandlerStandalone::BuildHubUrl(const StandaloneUserData& CurrentUser) const
{
    std::string BaseUrl = (CurrentUser.Network == "mainnet") ? HubBaseUrlMainnet : HubBaseUrlTestnet;
    std::ostringstream Url;

    Url << BaseUrl << "?app_id=" << AppId;

    if (CurrentUser.bIsLoggedIn && !CurrentUser.Address.empty())
    {
        Url << "&player=" << CurrentUser.Address;
    }

    if (!CurrentUser.Network.empty())
    {
        Url << "&network=" << CurrentUser.Network;
    }

    return Url.str();
}

void HubButtonHandlerStandalone::OnClickHub(const StandaloneUserData& CurrentUser)
{
    std::string FullUrl = BuildHubUrl(CurrentUser);

    std::cout << "[HubStandalone] Opening Hub dashboard: " << FullUrl << std::endl;

    if (OpenUrlFn)
    {
        OpenUrlFn(FullUrl);
    }
    else
    {
        LaunchSystemBrowser(FullUrl);
    }
}

void HubButtonHandlerStandalone::LaunchSystemBrowser(const std::string& Url)
{
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", Url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    std::string Command = "open \"" + Url + "\"";
    std::system(Command.c_str());
#else // Linux / POSIX
    std::string Command = "xdg-open \"" + Url + "\"";
    std::system(Command.c_str());
#endif
}
