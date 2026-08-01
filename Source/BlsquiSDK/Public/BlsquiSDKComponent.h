#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "BlsquiSDKComponent.generated.h"

// Blueprint structure representing the Flow Transaction Result
USTRUCT(BlueprintType)
struct FTxResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Status;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString TxId;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Error;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Nonce;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Payer;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString To;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Amount;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui SDK")
    FString Token;
};

// Event Dispatcher fired when a transaction finishes (Sealed, Expired, or Timeout)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransactionCompleted, const FTxResult&, Result);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SDKTEST1_API UBlsquiSDKComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBlsquiSDKComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    // Event Dispatcher that Blueprints can assign to
    UPROPERTY(BlueprintAssignable, Category = "Blsqui SDK")
    FOnTransactionCompleted OnTransactionCompleted;

    /**
     * Primary Entry Point: Opens system browser and polls backend server for completion.
     */
    UFUNCTION(BlueprintCallable, Category = "Blsqui SDK")
    void RequestTransaction(float Amount, const FString& Destination, bool bIsTestnet = true, bool bVerbose = false);

private:
    const FString MAINNET_URL = TEXT("https://signer.blsqui.net");
    const FString TESTNET_URL = TEXT("https://testnet-signer.blsqui.net");
    const float POLL_INTERVAL_SECONDS = 1.5f;
    const float TIMEOUT_SECONDS = 300.0f;

    FString ActiveBaseUrl;
    FString ActiveNonce;
    bool bActiveVerbose;
    float StartTime;
    bool bIsWaitingForNextPoll;
    float LastPollTime;

    void StartPolling();
    void PollTick();
    void OnPollResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    FString GenerateClientNonce();
    void CompleteTransaction(const FTxResult& Result);
};