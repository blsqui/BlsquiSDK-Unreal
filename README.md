# Blsqui SDK for Unreal Engine 5

The official Unreal Engine C++ plugin for **Blsqui** — enabling seamless Flow blockchain payments, Passkey wallet authentication, and microtransactions in Unreal Engine games.

## Features
- **Passkey & WebAuthn Ready:** Launches the system browser directly to the Blsqui Gateway for one-touch biometric signing.
- **Edge Polling Engine:** Asynchronous, non-blocking HTTP polling against Blsqui edge nodes until on-chain finality without freezing game threads or UI.
- **FLIX Native:** Built-in support for Flow Interaction Templates with arbitrary Cadence script arguments.
- **Clean C++ Architecture:** Zero forced UI assets or heavy Blueprint dependencies, designed for direct integration into production game loops.

## 📦 Installation & Setup

1. Download or clone this repository.
2. Copy the `BlsquiSDK` folder into your Unreal Engine project's `Plugins/` directory:
```text
YourProject/
└── Plugins/
    └── BlsquiSDK/
```
3. Add `"BlsquiSDK"` to the `PublicDependencyModuleNames` array inside your game's `.Build.cs`:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "BlsquiSDK" });
```
4. Regenerate project files (right-click `.uproject` -> Generate Visual Studio project files) and compile your solution.
5. In the Unreal Editor, open Edit -> Plugins and ensure Blsqui Flow Esports Payment SDK is enabled.


## Quick Start Example
Call `BlsquiSDK.request_transaction()` from any button or event handler:

```cpp
#include "YourPlayerController.h"
#include "BlsquiSDKComponent.h"

void AYourPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Attach or locate the Blsqui component
    UBlsquiSDKComponent* Blsqui = FindComponentByClass<UBlsquiSDKComponent>();
    if (!Blsqui)
    {
        Blsqui = NewObject<UBlsquiSDKComponent>(this, TEXT("BlsquiSDKComponent"));
        Blsqui->RegisterComponent();
    }

    // Bind event callback
    Blsqui->OnTransactionCompleted.AddDynamic(this, &AYourPlayerController::HandlePaymentCompleted);
}

void AYourPlayerController::PurchaseTournamentEntry()
{
    if (UBlsquiSDKComponent* Blsqui = FindComponentByClass<UBlsquiSDKComponent>())
    {
        FBlsquiTxOptions Options;
        Options.bIsTestnet = true;
        Options.bVerbose = true;
        Options.FlixId = TEXT("7d9d4b154547d7f6ec95e8b95741ed84663592d8c0016dbc4b28b6f9bf435ba5");
        Options.Args.Add(TEXT("to"), TEXT("0xa090f900023d6d34")); // Merchant / deposit address

        Blsqui->RequestTransaction(Options);
    }
}

void AYourPlayerController::HandlePaymentCompleted(const FTxResult& Result)
{
    if (Result.Status == TEXT("SEALED"))
    {
        UE_LOG(LogTemp, Log, TEXT("🎉 Payment succeeded! TX ID: %s"), *Result.TxId);
        UE_LOG(LogTemp, Log, TEXT("Payer: %s"), *Result.Payer);
        UE_LOG(LogTemp, Log, TEXT("Amount Paid: %s %s"), *Result.Amount, *Result.Token);
        // Unlock in-game item or grant access here
    }
    else
    {
        FString ErrorDetail = !Result.ErrorMessage.IsEmpty() ? Result.ErrorMessage : Result.Error;
        UE_LOG(LogTemp, Warning, TEXT("Payment unfinished. Status: %s | Error: %s"), *Result.Status, *ErrorDetail);
    }
}
```

## Configuration Reference
### `FBlsquiTxOptions`
Pass this struct into `Blsqui->RequestTransaction(Options)`:
| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `bIsTestnet` | `bool` | `true` | When `true`, connects to Flow Testnet (`lab.blsqui.net`). When `false`, connects to Flow Mainnet (`wallet.blsqui.net`). |
| `FlixId` | `FString` | Default Fee Template | The FLIX (Flow Interaction Template) identifier. Defaults to the standard 10 FLOW entry template. |
| `Args` | `TMap<FString, FString>` | `empty` | Key-value arguments passed into the Cadence interaction. (e.g. `{"to": "0x..."}`). |
| `bVerbose` | `bool` | `false` | When `true`, logs detailed polling requests and HTTP status codes to the Output Log. |

---

### `FTxResult`
Broadcast via `OnTransactionCompleted`:

```cpp
struct FTxResult
{
    FString Status;        // "SEALED" | "FAILED" | "EXPIRED" | "TIMEOUT" | "CANCELED"
    FString TxId;          // Flow blockchain transaction ID (SEALED only)
    FString Nonce;         // Cryptographic tracking nonce
    FString Payer;         // Flow account address that signed the transaction
    FString To;            // Recipient account address
    FString Amount;        // Executed payment amount (Cadence UFix64 string)
    FString Token;         // Token identifier (e.g., "FlowToken")
    FString Error;         // Client-side error description
    FString ErrorMessage;  // Detailed Cadence runtime error message from the node
};
```

### Transaction Status Definitions

 - `SEALED`: The transaction was executed and finalized on the Flow blockchain.

 - `FAILED`: The transaction failed during on-chain execution (see errorMessage).

 - `EXPIRED`: The transaction expired on-chain before execution.

 - `TIMEOUT`: Polling exceeded the maximum duration (default: 300 seconds).

 - `CANCELED`: Polling was stopped manually via `Blsqui->CancelTransaction()`.


## Requirements
- Unreal Engine **5.0** or higher
- Standard C++ project setup (Visual Studio 2022, Xcode, or JetBrains Rider)
- Active internet connection

## License
MIT License - see [LICENSE](LICENSE) for details.
