#include "BlsquiSDKComponent.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformMisc.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Guid.h"

UBlsquiSDKComponent::UBlsquiSDKComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    bIsWaitingForNextPoll = false;
    bIsPollingActive = false;
    bIsCanceled = false;
    LastPollTime = 0.0f;
    StartTime = 0.0f;
    bActiveVerbose = false;
    bIsTestnetActive = true;
}

void UBlsquiSDKComponent::RequestTransaction(const FBlsquiTxOptions& Options)
{
    bIsCanceled = false;
    bActiveVerbose = Options.bVerbose;
    bIsTestnetActive = Options.bIsTestnet;
    ActiveNonce = GenerateClientNonce();

    ActiveGatewayBaseUrl = Options.bIsTestnet ? TESTNET_GATEWAY_URL : MAINNET_GATEWAY_URL;
    FString BasePollApi = Options.bIsTestnet ? TESTNET_POLL_API : MAINNET_POLL_API;
    ActivePollUrl = FString::Printf(TEXT("%s?nonce=%s"), *BasePollApi, *FGenericPlatformHttp::UrlEncode(ActiveNonce));

    int64 CurrentTime = FDateTime::UtcNow().ToUnixTimestamp();

    // Request signed session token
    RequestSessionToken(Options, ActiveNonce, CurrentTime);
}

void UBlsquiSDKComponent::RequestSessionToken(const FBlsquiTxOptions& Options, const FString& Nonce, int64 CurrentTime)
{
    FString SignApiUrl = Options.bIsTestnet ? TESTNET_SESSION_SIGN_API : MAINNET_SESSION_SIGN_API;

    TSharedPtr<FJsonObject> JsonPayload = MakeShared<FJsonObject>();
    JsonPayload->SetStringField(TEXT("flix"), Options.FlixId);
    JsonPayload->SetNumberField(TEXT("issued_time"), CurrentTime);
    JsonPayload->SetStringField(TEXT("nonce"), Nonce);

    for (const TPair<FString, FString>& Pair : Options.Args)
    {
        FString TrimmedVal = Pair.Value.TrimStartAndEnd();
        if (!TrimmedVal.IsEmpty())
        {
            JsonPayload->SetStringField(Pair.Key, TrimmedVal);
        }
    }

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

    CurrentHttpRequest = FHttpModule::Get().CreateRequest();
    CurrentHttpRequest->SetVerb(TEXT("POST"));
    CurrentHttpRequest->SetURL(SignApiUrl);
    CurrentHttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    CurrentHttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    CurrentHttpRequest->SetContentAsString(RequestBody);

    TWeakObjectPtr<UBlsquiSDKComponent> WeakThis(this);
    CurrentHttpRequest->OnProcessRequestComplete().BindLambda(
        [WeakThis](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (UBlsquiSDKComponent* StrongThis = WeakThis.Get())
            {
                StrongThis->OnSessionSignResponseReceived(Request, Response, bWasSuccessful);
            }
        }
    );

    CurrentHttpRequest->ProcessRequest();
}

void UBlsquiSDKComponent::OnSessionSignResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    CurrentHttpRequest.Reset();

    if (bIsCanceled)
    {
        return;
    }

    if (!bWasSuccessful || !Response.IsValid())
    {
        FTxResult ErrorResult;
        ErrorResult.Status = TEXT("FAILED");
        ErrorResult.Nonce = ActiveNonce;
        ErrorResult.Error = TEXT("Failed to sign transaction session: HTTP request failed");
        CompleteTransaction(ErrorResult);
        return;
    }

    int32 ResponseCode = Response->GetResponseCode();
    FString BodyText = Response->GetContentAsString();

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyText);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        FTxResult ErrorResult;
        ErrorResult.Status = TEXT("FAILED");
        ErrorResult.Nonce = ActiveNonce;
        ErrorResult.Error = FString::Printf(TEXT("Failed to sign transaction session: Invalid JSON (HTTP %d)"), ResponseCode);
        CompleteTransaction(ErrorResult);
        return;
    }

    if (ResponseCode < 200 || ResponseCode >= 300)
    {
        FString ErrorMsg = JsonObject->HasField(TEXT("error")) ? JsonObject->GetStringField(TEXT("error")) : FString::Printf(TEXT("HTTP_%d"), ResponseCode);
        if (bActiveVerbose)
        {
            UE_LOG(LogTemp, Error, TEXT("[BlsquiSDK] Session token signing failed: %s"), *ErrorMsg);
        }

        FTxResult ErrorResult;
        ErrorResult.Status = TEXT("FAILED");
        ErrorResult.Nonce = ActiveNonce;
        ErrorResult.Error = FString::Printf(TEXT("Failed to sign transaction session: %s"), *ErrorMsg);
        CompleteTransaction(ErrorResult);
        return;
    }

    FString SignedToken = JsonObject->GetStringField(TEXT("token"));
    if (SignedToken.IsEmpty())
    {
        FTxResult ErrorResult;
        ErrorResult.Status = TEXT("FAILED");
        ErrorResult.Nonce = ActiveNonce;
        ErrorResult.Error = TEXT("Failed to sign transaction session: Response missing signed token");
        CompleteTransaction(ErrorResult);
        return;
    }

    // Build tamper-proof URL and launch system browser
    FString FullUrl = FString::Printf(TEXT("%s?request=%s"), *ActiveGatewayBaseUrl, *FGenericPlatformHttp::UrlEncode(SignedToken));

    if (bActiveVerbose)
    {
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Mode: Unreal Native Browser [%s]"), bIsTestnetActive ? TEXT("TESTNET") : TEXT("MAINNET"));
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Wallet Gateway URL: %s"), *FullUrl);
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Nonce: %s"), *ActiveNonce);
    }

    FPlatformProcess::LaunchURL(*FullUrl, nullptr, nullptr);

    // Step 3: Begin polling backend
    StartPolling();
}

void UBlsquiSDKComponent::CancelTransaction()
{
    bIsCanceled = true;
    if (CurrentHttpRequest.IsValid())
    {
        CurrentHttpRequest->CancelRequest();
        CurrentHttpRequest.Reset();
    }

    if (bIsPollingActive)
    {
        FTxResult CanceledResult;
        CanceledResult.Status = TEXT("CANCELED");
        CanceledResult.Nonce = ActiveNonce;
        CanceledResult.Error = TEXT("Transaction canceled by user.");
        CompleteTransaction(CanceledResult);
    }
}

void UBlsquiSDKComponent::StartPolling()
{
    bIsPollingActive = true;
    bIsWaitingForNextPoll = false;
    SetComponentTickEnabled(true);
    StartTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;

    if (bActiveVerbose)
    {
        UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] 🚀 Starting polling loop..."));
    }

    PollTick();
}

void UBlsquiSDKComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsPollingActive)
    {
        return;
    }

    if (bIsWaitingForNextPoll && GetWorld())
    {
        float CurrentRealTime = GetWorld()->GetRealTimeSeconds();
        if (CurrentRealTime - LastPollTime >= POLL_INTERVAL_SECONDS)
        {
            bIsWaitingForNextPoll = false;
            PollTick();
        }
    }
}

void UBlsquiSDKComponent::PollTick()
{
    if (bIsCanceled)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    if ((World->GetRealTimeSeconds() - StartTime) >= TIMEOUT_SECONDS)
    {
        if (bActiveVerbose)
        {
            UE_LOG(LogTemp, Error, TEXT("[BlsquiSDK] Transaction poll timed out after %.0f seconds."), TIMEOUT_SECONDS);
        }

        FTxResult TimeoutResult;
        TimeoutResult.Status = TEXT("TIMEOUT");
        TimeoutResult.Nonce = ActiveNonce;
        TimeoutResult.Error = FString::Printf(TEXT("Transaction poll timed out after %.0f seconds."), TIMEOUT_SECONDS);
        CompleteTransaction(TimeoutResult);
        return;
    }

    CurrentHttpRequest = FHttpModule::Get().CreateRequest();
    CurrentHttpRequest->SetVerb(TEXT("GET"));
    CurrentHttpRequest->SetURL(ActivePollUrl);
    CurrentHttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));

    TWeakObjectPtr<UBlsquiSDKComponent> WeakThis(this);
    CurrentHttpRequest->OnProcessRequestComplete().BindLambda(
        [WeakThis](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (UBlsquiSDKComponent* StrongThis = WeakThis.Get())
            {
                StrongThis->OnPollResponseReceived(Request, Response, bWasSuccessful);
            }
        }
    );

    CurrentHttpRequest->ProcessRequest();
}

void UBlsquiSDKComponent::OnPollResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    CurrentHttpRequest.Reset();

    if (bIsCanceled)
    {
        return;
    }

    if (!bWasSuccessful || !Response.IsValid())
    {
        if (bActiveVerbose)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BlsquiSDK Poll Request Error] HTTP request failed. Retrying..."));
        }

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
            FString Status = JsonObject->GetStringField(TEXT("status")).ToUpper();
            if (Status.IsEmpty()) Status = TEXT("PENDING");

            if (bActiveVerbose)
            {
                UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK Poll] HTTP 200 | Status: '%s' | JSON: %s"), *Status, *JsonText);
            }

            if (Status == TEXT("SEALED"))
            {
                FString ErrorMsg = JsonObject->HasField(TEXT("errorMessage")) ? JsonObject->GetStringField(TEXT("errorMessage")) : TEXT("");

                if (!ErrorMsg.IsEmpty())
                {
                    FTxResult FailedResult;
                    FailedResult.Status = TEXT("FAILED");
                    FailedResult.TxId = JsonObject->GetStringField(TEXT("txId"));
                    FailedResult.Nonce = ActiveNonce;
                    FailedResult.Payer = JsonObject->GetStringField(TEXT("payer"));
                    FailedResult.Error = TEXT("On-chain execution failed");
                    FailedResult.ErrorMessage = ErrorMsg;
                    CompleteTransaction(FailedResult);
                    return;
                }

                if (bActiveVerbose)
                {
                    UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] 🎉 Transaction Sealed! TX ID: %s"), *JsonObject->GetStringField(TEXT("txId")));
                }

                FString FeeVal = TEXT("");
                if (JsonObject->HasField(TEXT("txFee")))
                {
                    FeeVal = JsonObject->GetStringField(TEXT("txFee"));
                }
                else if (JsonObject->HasField(TEXT("tx_fee")))
                {
                    FeeVal = JsonObject->GetStringField(TEXT("tx_fee"));
                }

                FTxResult SuccessResult;
                SuccessResult.Status = TEXT("SEALED");
                SuccessResult.TxId = JsonObject->GetStringField(TEXT("txId"));
                SuccessResult.Nonce = ActiveNonce;
                SuccessResult.Payer = JsonObject->GetStringField(TEXT("payer"));
                SuccessResult.To = JsonObject->GetStringField(TEXT("to"));
                SuccessResult.Amount = JsonObject->HasField(TEXT("amount")) ? JsonObject->GetStringField(TEXT("amount")) : TEXT("0.0");
                SuccessResult.Token = JsonObject->GetStringField(TEXT("token"));
                SuccessResult.TxFee = FeeVal;
                CompleteTransaction(SuccessResult);
                return;
            }
            else if (Status == TEXT("EXPIRED"))
            {
                if (bActiveVerbose)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[BlsquiSDK] Transaction EXPIRED on-chain."));
                }

                FTxResult ExpiredResult;
                ExpiredResult.Status = TEXT("EXPIRED");
                ExpiredResult.Nonce = ActiveNonce;
                ExpiredResult.Error = TEXT("Transaction expired on-chain");
                CompleteTransaction(ExpiredResult);
                return;
            }
        }
    }
    else if (bActiveVerbose)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BlsquiSDK Poll] Non-200 HTTP response: %d"), ResponseCode);
    }

    if (GetWorld())
    {
        LastPollTime = GetWorld()->GetRealTimeSeconds();
        bIsWaitingForNextPoll = true;
    }
}

void UBlsquiSDKComponent::CompleteTransaction(const FTxResult& Result)
{
    bIsPollingActive = false;
    bIsWaitingForNextPoll = false;
    SetComponentTickEnabled(false);

    OnTransactionCompleted.Broadcast(Result);
}

FString UBlsquiSDKComponent::GenerateClientNonce()
{
    FString Part1 = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString Part2 = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    return (Part1 + Part2).ToLower();
}
