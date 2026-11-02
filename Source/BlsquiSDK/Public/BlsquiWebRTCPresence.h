#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "BlsquiTypes.h"
#include "BlsquiWebRTCPresence.generated.h"

// Forward declaration for libdatachannel types
namespace rtc {
    class PeerConnection;
    class DataChannel;
}

UCLASS(BlueprintType)
class BLSQUISDK_API UBlsquiWebRTCPresence : public UObject
{
    GENERATED_BODY()

public:
    UBlsquiWebRTCPresence();
    virtual ~UBlsquiWebRTCPresence();

    UPROPERTY(BlueprintAssignable, Category = "Blsqui|WebRTC")
    FOnPresenceMessage OnMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "Blsqui|WebRTC")
    FOnPresenceStateChanged OnStateChanged;

    UFUNCTION(BlueprintCallable, Category = "Blsqui|WebRTC")
    void InitPresence(const FString& PlayerAddress, const FString& Role = TEXT("game"));

    UFUNCTION(BlueprintCallable, Category = "Blsqui|WebRTC")
    void SendData(const FString& Payload);

    UFUNCTION(BlueprintCallable, Category = "Blsqui|WebRTC")
    void Cleanup();

    UFUNCTION(BlueprintPure, Category = "Blsqui|WebRTC")
    FPresenceState GetState() const { return State; }

private:
    const FString SIGNALING_API = TEXT("https://api.blsqui-signer.net/api/rtc/session");

    FPresenceState State;
    FString ActiveAddress;
    bool bIsConnecting = false;

    std::shared_ptr<rtc::PeerConnection> PeerConnection;
    std::shared_ptr<rtc::DataChannel> DataChannel;

    void SendOfferToSignalingServer(const FString& SdpOffer, const FString& Role, const FString& CleanAddr);
    void OnSignalingResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    FString ExtractAnswerSdp(const FString& Json);
    void HandleIncomingRaw(const FString& RawText);
};
