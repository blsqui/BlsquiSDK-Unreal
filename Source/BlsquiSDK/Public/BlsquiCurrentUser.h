#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BlsquiTypes.h"
#include "BlsquiWebRTCPresence.h"
#include "BlsquiCurrentUser.generated.h"

UCLASS()
class BLSQUISDK_API UBlsquiCurrentUser : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category = "Blsqui|User")
    FOnUserUpdated OnUserUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Blsqui|User")
    FOnSlotsUpdated OnSlotsUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Blsqui|User")
    FOnItemUseRequested OnItemUseRequested;

    UFUNCTION(BlueprintCallable, Category = "Blsqui|User")
    void Configure(const FString& InAppId, const FString& InPublisherAddress, const FString& InNetwork = TEXT("testnet"));

    UFUNCTION(BlueprintCallable, Category = "Blsqui|User")
    void OpenDashboard();

    UFUNCTION(BlueprintCallable, Category = "Blsqui|User")
    void RequestUseItem(const FString& ItemId);

    UFUNCTION(BlueprintPure, Category = "Blsqui|User")
    FCurrentUserState GetUserState() const { return UserState; }

    UFUNCTION(BlueprintCallable, Category = "Blsqui|User")
    FString GetOrCreateChannelKey();

private:
    const FString TESTNET_BASE_URL = TEXT("https://lab.blsqui.net");
    const FString MAINNET_BASE_URL = TEXT("https://wallet.blsqui.net");

    FString AppId;
    FString PublisherAddress;
    FString Network = TEXT("testnet");
    FString SessionChannelKey;

    FCurrentUserState UserState;

    UPROPERTY()
    TObjectPtr<UBlsquiWebRTCPresence> Presence;

    FTimerHandle HeartbeatTimerHandle;

    UFUNCTION()
    void OnPresenceMessageReceived(const FString& RawText);

    void StartHeartbeat();
    void StopHeartbeat();
    void SendPresenceLiveSignal();

    FString GenerateChannelKey();
    TArray<FStageSlotItem> ExtractSlotsFromPayload(const FString& RawText);
    FString ExtractStringValue(const FString& Json, const FString& Key);
    int32 ExtractIntValue(const FString& Json, const FString& Key);
};
