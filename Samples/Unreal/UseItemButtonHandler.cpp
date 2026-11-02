#include "UseItemButtonHandler.h"

UUseItemButtonHandler::UUseItemButtonHandler()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UUseItemButtonHandler::BeginPlay()
{
    Super::BeginPlay();
}

void UUseItemButtonHandler::InitializeHandler(UBlsquiCurrentUser* InCurrentUser, UBlsquiSDKComponent* InSDKComponent)
{
    CurrentUser = InCurrentUser;
    SDKComponent = InSDKComponent;

    if (CurrentUser.IsValid())
    {
        CurrentUser->OnItemUseRequested.RemoveDynamic(this, &UUseItemButtonHandler::HandleItemUseRequested);
        CurrentUser->OnItemUseRequested.AddDynamic(this, &UUseItemButtonHandler::HandleItemUseRequested);
    }

    if (SDKComponent.IsValid())
    {
        SDKComponent->OnTransactionCompleted.RemoveDynamic(this, &UUseItemButtonHandler::HandleTransactionCompleted);
        SDKComponent->OnTransactionCompleted.AddDynamic(this, &UUseItemButtonHandler::HandleTransactionCompleted);
    }
}

void UUseItemButtonHandler::OnUseItemButtonClicked(const FString& ItemId)
{
    if (CurrentUser.IsValid())
    {
        CurrentUser->RequestUseItem(ItemId);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UseItemButtonHandler] CurrentUser reference is invalid."));
    }
}

void UUseItemButtonHandler::HandleItemUseRequested(const FItemUseRequestPayload& Payload)
{
    if (!SDKComponent.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[UseItemButtonHandler] SDKComponent reference is invalid."));
        return;
    }

    FBlsquiTxOptions Options;
    Options.bIsTestnet = true;
    Options.FlixId = Payload.FlixId;
    Options.Args.Add(TEXT("itemId"), Payload.ItemId);
    Options.Args.Add(TEXT("serialNumber"), FString::FromInt(Payload.SerialNumber));

    SDKComponent->RequestTransaction(Options);
}

void UUseItemButtonHandler::HandleTransactionCompleted(const FTxResult& Result)
{
    FString Title = TEXT("BLSQUI ITEM TRANSACTION");
    FString Status = Result.Status;
    bool bSuccess = (Result.Status == TEXT("SEALED"));

    FString Details;
    if (bSuccess)
    {
        Details = FString::Printf(
            TEXT("• Transaction ID:\n  %s\n\n• Payer: %s\n• Merchant: %s\n• Nonce: %s"),
            *Result.TxId,
            *Result.Payer,
            *Result.To,
            *Result.Nonce
        );
    }
    else
    {
        Details = FString::Printf(
            TEXT("• Error: %s\n\n• Details:\n%s"),
            Result.Error.IsEmpty() ? TEXT("Item transaction rejected or aborted.") : *Result.Error,
            Result.ErrorMessage.IsEmpty() ? TEXT("No runtime error provided.") : *Result.ErrorMessage
        );
    }

    OnItemTxResultModalReady.Broadcast(Title, Status, Details, bSuccess);
}
