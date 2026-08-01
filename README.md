# Blsqui SDK for Unreal Engine 5

The official Unreal Engine 5 C++ plugin for **Blsqui** — enabling seamless Flow blockchain local loopback authentication and in-game transaction dialogs.

## Features
- **Local Loopback Auth:** Secure, browser-based Flow wallet authentication via local loopback listener.
- **Cloudflare Worker Backbone:** High-throughput, edge-accelerated transaction status polling via Cloudflare Workers.
- **Pre-built UMG Dialogs:** Ready-to-use Blueprint UI widgets (`WBP_LoadingDialog`, `WBP_PaymentDialog`) for payment confirmation.
- **Blueprint Assignable Delegates:** Native C++ delegates (`OnTransactionCompleted`, `OnTransactionFailed`) for clean Event Graph wiring.

## 📦 Quick Start
1. Clone or download this repository.
2. Copy the `BlsquiSDK` folder into your Unreal project's `Plugins/` directory (e.g., `YourProject/Plugins/BlsquiSDK`).
3. Re-generate project files and compile your project in Xcode / Visual Studio.
4. Enable **Blsqui SDK** in **Edit -> Plugins**.
5. Add the `BlsquiSDKComponent` to your player character or game mode actor!

## Requirements
- Unreal Engine **5.0** or higher
- C++ or Blueprint Project setup

## License
MIT License - see [LICENSE](LICENSE) for details.