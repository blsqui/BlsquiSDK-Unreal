#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "BlsquiSDKComponent.generated.h"

// Options passed to RequestTransaction
USTRUCT(BlueprintType)
struct FBlsquiTxOptions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui SDK")
    bool bIsTestnet = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui SDK")
    FString FlixId = TEXT("7d9d4b154547d7f6ec95e8b95741ed84663592d8c0016dbc4b28b6f9bf435ba5");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui SDK")
    TMap<FString, FString> Args;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui SDK")
    bool bVerbose = false;
};

// Flow Transaction Result
USTRUCT(BlueprintType)
struct FTxResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Status; // "SEALED", "FAILED", "EXPIRED", "TIMEOUT", "CANCELED"

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString TxId;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Nonce;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Payer;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString To;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Amount;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Token;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString TxFee;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Error;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString ErrorMessage;
};

// Event Dispatcher for result notification
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransactionCompleted, const FTxResult&, Result);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLSQUISDK_API UBlsquiSDKComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBlsquiSDKComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category = "Blsqui SDK")
    FOnTransactionCompleted OnTransactionCompleted;

    /**
     * Primary Entry Point: Requests a signed session token, launches the browser gateway, and polls backend for status.
     */
    UFUNCTION(BlueprintCallable, Category = "Blsqui SDK")
    void RequestTransaction(const FBlsquiTxOptions& Options);

    /**
     * Cancels any active signing or polling loop.
     */
    UFUNCTION(BlueprintCallable, Category = "Blsqui SDK")
    void CancelTransaction();

private:
    const FString MAINNET_GATEWAY_URL      = TEXT("https://wallet.blsqui.net/transaction");
    const FString TESTNET_GATEWAY_URL      = TEXT("https://lab.blsqui.net/transaction");
    const FString MAINNET_SESSION_SIGN_API = TEXT("https://wallet.blsqui.net/api/session/sign");
    const FString TESTNET_SESSION_SIGN_API = TEXT("https://lab.blsqui.net/api/session/sign");
    const FString MAINNET_POLL_API         = TEXT("https://wallet.blsqui.net/api/status");
    const FString TESTNET_POLL_API         = TEXT("https://lab.blsqui.net/api/status");

    const float POLL_INTERVAL_SECONDS = 1.5f;
    const float TIMEOUT_SECONDS = 300.0f;

    FString ActiveGatewayBaseUrl;
    FString ActivePollUrl;
    FString ActiveNonce;
    bool bActiveVerbose;
    bool bIsTestnetActive;
    bool bIsCanceled;
    bool bIsPollingActive;
    bool bIsWaitingForNextPoll;
    float StartTime;
    float LastPollTime;

    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentHttpRequest;

    void RequestSessionToken(const FBlsquiTxOptions& Options, const FString& Nonce, int64 CurrentTime);
    void OnSessionSignResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void StartPolling();
    void PollTick();
    void OnPollResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    FString GenerateClientNonce();
    void CompleteTransaction(const FTxResult& Result);
};
