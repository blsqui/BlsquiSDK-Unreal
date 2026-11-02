#include "ShopButtonHandler.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/Button.h"

UShopButtonHandler::UShopButtonHandler()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UShopButtonHandler::BeginPlay()
{
    Super::BeginPlay();

    // Cache the SDK component on the owner actor or pawn
    if (AActor* Owner = GetOwner())
    {
        BlsquiSDKComp = Owner->FindComponentByClass<UBlsquiSDKComponent>();
        if (BlsquiSDKComp)
        {
            BlsquiSDKComp->OnTransactionSealed.AddDynamic(this, &UShopButtonHandler::HandleTransactionSealed);
        }
    }
}

void UShopButtonHandler::ExecuteShopPurchase()
{
    if (!BlsquiSDKComp)
    {
        if (AActor* Owner = GetOwner())
        {
            BlsquiSDKComp = Owner->FindComponentByClass<UBlsquiSDKComponent>();
        }
    }

    if (!BlsquiSDKComp)
    {
        UE_LOG(LogTemp, Error, TEXT("[Shop] UBlsquiSDKComponent not found on owner actor."));
        return;
    }

    // Display UI feedback
    if (ShopDialog)
    {
        if (TitleText)
        {
            TitleText->SetText(FText::FromString(TEXT("Blsqui In-Game Shop")));
        }
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("Authorizing transaction in Web3 wallet...")));
        }
        if (CancelButton)
        {
            CancelButton->SetVisibility(ESlateVisibility::Visible);
        }
        ShopDialog->SetVisibility(ESlateVisibility::Visible);
    }

    // Build transaction options
    FBlsquiTxOptions Options;
    Options.bIsTestnet = bIsTestnet;
    Options.bVerbose = true;
    Options.FlixId = FlixId;

    if (!MerchantAddress.IsEmpty())
    {
        Options.Args.Add(TEXT("to"), MerchantAddress);
    }

    UE_LOG(LogTemp, Log, TEXT("[Shop] Requesting transaction with FLIX: %s"), *FlixId);

    // Trigger wallet interaction modal/tab
    BlsquiSDKComp->RequestTransaction(Options);
}

void UShopButtonHandler::HandleTransactionSealed(const FString& TxId, bool bSuccess)
{
    if (!StatusText) return;

    if (bSuccess)
    {
        FString Msg = FString::Printf(TEXT("Purchase Successful!\nTxID: %s"), *TxId);
        StatusText->SetText(FText::FromString(Msg));
        UE_LOG(LogTemp, Log, TEXT("[Shop] Transaction sealed successfully: %s"), *TxId);
    }
    else
    {
        StatusText->SetText(FText::FromString(TEXT("Transaction canceled or failed.")));
        UE_LOG(LogTemp, Warning, TEXT("[Shop] Transaction rejected or failed: %s"), *TxId);
    }
}

void UShopButtonHandler::OnCloseShopDialog()
{
    if (ShopDialog)
    {
        ShopDialog->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UShopButtonHandler::FormatTxResultForModal(
    const FTxResult& Result, 
    FString& OutTitle, 
    FString& OutStatus, 
    FString& OutDetails, 
    bool& bOutSuccess)
{
    OutTitle = TEXT("BLSQUI SHOP TRANSACTION");
    OutStatus = Result.Status;
    bOutSuccess = (Result.Status == TEXT("SEALED"));

    if (bOutSuccess)
    {
        OutDetails = FString::Printf(
            TEXT("• Transaction ID:\n  %s\n\n• Payer: %s\n• Merchant: %s\n• Amount: %s %s\n• Network Fee: %s FlowToken\n• Nonce: %s"),
            *Result.TxId,
            *Result.Payer,
            *Result.To,
            *Result.Amount,
            Result.Token.IsEmpty() ? TEXT("FlowToken") : *Result.Token,
            Result.TxFee.IsEmpty() ? TEXT("0.00001000") : *Result.TxFee,
            *Result.Nonce
        );
    }
    else
    {
        OutDetails = FString::Printf(
            TEXT("• Error: %s\n\n• Details:\n%s"),
            Result.Error.IsEmpty() ? TEXT("Transaction was rejected or aborted.") : *Result.Error,
            Result.ErrorMessage.IsEmpty() ? TEXT("No runtime error returned.") : *Result.ErrorMessage
        );
    }
}