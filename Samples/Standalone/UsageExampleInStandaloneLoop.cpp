int main()
{
    StandaloneUserData ActiveUser;
    ActiveUser.Address = "0xcabd5cc48a013e7f";
    ActiveUser.Network = "testnet";
    ActiveUser.bIsLoggedIn = true;

    HubButtonHandlerStandalone HubHandler;

    // Simulating player pressing HUB button in UI
    HubHandler.OnClickHub(ActiveUser);

    return 0;
}
