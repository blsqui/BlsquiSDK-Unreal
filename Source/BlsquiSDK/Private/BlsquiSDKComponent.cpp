#include "BlsquiSDKComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Guid.h"
#include "TimerManager.h"

UBlsquiSDKComponent::UBlsquiSDKComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    bIsWaitingForNextPoll = false;
    LastPollTime = 0.0f;
}

void UBlsquiSDKComponent::RequestTransaction(float Amount, const FString& Destination, bool bIsTestnet, bool bVerbose)
{
    if (GetWorld() == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[BlsquiSDK] CRASH AVERTED: GetWorld() is NULL!"));
        return; 
    }

    // 🛠️ AUTO-FIX: Force show the mouse cursor and set input mode to UI/Game 
    // so clicks never get ignored by the viewport background!
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        PC->bShowMouseCursor = true;
        
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
    }

    ActiveBaseUrl = bIsTestnet ? TESTNET_URL : MAINNET_URL;
    ActiveNonce = GenerateClientNonce();
    bActiveVerbose = bVerbose;

    int64 CurrentTime = FDateTime::UtcNow().ToUnixTimestamp();

    // Build payment signer URL safely
    FString FullUrl = FString::Printf(TEXT("%s?nonce=%s&to=%s&price=%.2f&issued_time=%lld"),
        *ActiveBaseUrl, *ActiveNonce, *Destination, Amount, CurrentTime);

    if (bActiveVerbose)
    {
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Launching browser (%s) -> %s"), bIsTestnet ? TEXT("TESTNET") : TEXT("MAINNET"), *FullUrl);
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Nonce: %s"), *ActiveNonce);
    }

    // Open user browser
    FPlatformProcess::LaunchURL(*FullUrl, nullptr, nullptr);

    // Start polling loop
    StartPolling();
}

void UBlsquiSDKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Our custom, pause-proof timer logic
    if (bIsWaitingForNextPoll && GetWorld())
    {
        // GetRealTimeSeconds ignores the game being paused!
        float CurrentRealTime = GetWorld()->GetRealTimeSeconds();
        
        if (CurrentRealTime - LastPollTime >= POLL_INTERVAL_SECONDS)
        {
            // Time is up! Stop waiting and fire the next request
            bIsWaitingForNextPoll = false;
            PollTick();
        }
    }
}

void UBlsquiSDKComponent::StartPolling()
{
    SetComponentTickEnabled(true);
    StartTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
    UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] 🚀 StartPolling called! Initiating first HTTP tick..."));
    
    // Kick off the first request immediately
    bIsWaitingForNextPoll = false;
    PollTick();
}

void UBlsquiSDKComponent::PollTick()
{
    UE_LOG(LogTemp, Log, TEXT("==== [BlsquiSDK] PollTick Executing... ===="));
    UWorld* World = GetWorld();
    if (!World) return;

    // Timeout Check (must use GetRealTimeSeconds here too!)
    if ((World->GetRealTimeSeconds() - StartTime) >= TIMEOUT_SECONDS)
    {
        if (bActiveVerbose)
        {
            UE_LOG(LogTemp, Error, TEXT("[BlsquiSDK] Transaction poll timed out after %.0f seconds."), TIMEOUT_SECONDS);
        }

        FTxResult TimeoutResult;
        TimeoutResult.Status = TEXT("TIMEOUT");
        TimeoutResult.Error = FString::Printf(TEXT("Transaction poll timed out after %.0f seconds"), TIMEOUT_SECONDS);
        CompleteTransaction(TimeoutResult);
        return;
    }

    // Send HTTP GET Request
    FString PollUrl = FString::Printf(TEXT("%s/api/status?nonce=%s"), *ActiveBaseUrl, *ActiveNonce);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb(TEXT("GET"));
    HttpRequest->SetURL(PollUrl);
    HttpRequest->OnProcessRequestComplete().BindUObject(this, &UBlsquiSDKComponent::OnPollResponseReceived);
    HttpRequest->ProcessRequest();
}

void UBlsquiSDKComponent::OnPollResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        if (bActiveVerbose)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BlsquiSDK Poll Request Error] HTTP request failed. Retrying..."));
        }
        
        // If it fails (network blip), tell the Tick function to try again in 1.5 seconds
        if (GetWorld())
        {
            LastPollTime = GetWorld()->GetRealTimeSeconds();
            bIsWaitingForNextPoll = true;
        }
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    FString JsonText = Response->GetContentAsString();

    if (ResponseCode == 200)
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);

        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            FString Status = JsonObject->GetStringField(TEXT("status"));
            if (Status.IsEmpty()) Status = TEXT("PENDING");

            if (bActiveVerbose)
            {
                UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK Poll] HTTP 200 | Status: '%s' | JSON: %s"), *Status, *JsonText);
            }

            FString UpperStatus = Status.ToUpper();
            if (UpperStatus == TEXT("SEALED") || UpperStatus == TEXT("EXECUTED") || UpperStatus == TEXT("FINALIZED") || UpperStatus == TEXT("SUCCESS"))
            {
                if (bActiveVerbose)
                {
                    UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] 🎉 Transaction Sealed! TX ID: %s"), *JsonObject->GetStringField(TEXT("txId")));
                }

                FTxResult SuccessResult;
                SuccessResult.Status = Status;
                SuccessResult.TxId = JsonObject->GetStringField(TEXT("txId"));
                // ... populate the rest of the struct ...

                CompleteTransaction(SuccessResult);
            }
            else if (UpperStatus == TEXT("EXPIRED"))
            {
                FTxResult ExpiredResult;
                ExpiredResult.Status = TEXT("EXPIRED");
                ExpiredResult.Error = TEXT("Transaction expired on-chain");
                CompleteTransaction(ExpiredResult);
            }
            else if (Status == TEXT("PENDING"))
            {
                // =========================================================================
                // STATUS IS 'PENDING': Tell the Tick function to wait 1.5 seconds, then fire again!
                // =========================================================================
                UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Status still PENDING. Re-scheduling PollTick in %.2f sec..."), POLL_INTERVAL_SECONDS);

                if (GetWorld())
                {
                    LastPollTime = GetWorld()->GetRealTimeSeconds();
                    bIsWaitingForNextPoll = true; 
                }
            }
        }
    }
}

void UBlsquiSDKComponent::CompleteTransaction(const FTxResult& Result)
{
    // Stop the custom timer loop
    bIsWaitingForNextPoll = false;

    // Broadcast the result to any listening Blueprints or Widgets
    OnTransactionCompleted.Broadcast(Result);
}

FString UBlsquiSDKComponent::GenerateClientNonce()
{
    // Calls Unreal Engine’s built-in platform-agnostic Globally Unique Identifier (GUID / UUID v4) generator.
    // Formats the GUID as a raw hexadecimal string without dashes or braces.
    // Because one GUID produces 32 hex characters (128 bits), combining two back-to-back produces a 64-character hex string (256 bits).
    FString FirstHalf = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString SecondHalf = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    return (FirstHalf + SecondHalf).ToLower();
}
