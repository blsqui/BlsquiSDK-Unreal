#pragma once

#include "CoreMinimal.h"
#include "BlsquiStageSlot.h"
#include "BlsquiTypes.generated.h"

USTRUCT(BlueprintType)
struct FCurrentUserState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString Addr = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    bool bLoggedIn = false;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString AvatarImage = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString UserName = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString Network = TEXT("testnet");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString LastUsedItemId = TEXT("");
};

USTRUCT(BlueprintType)
struct FItemUseRequestPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString Type = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString Player = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString AppId = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString ItemId = TEXT("");

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    int32 SerialNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    FString FlixId = TEXT("");
};

USTRUCT(BlueprintType)
struct FPresenceState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    bool bConnected = false;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    bool bGameOnline = false;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    bool bCanUseItem = false;

    UPROPERTY(BlueprintReadOnly, Category = "Blsqui")
    int64 LastPingMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserUpdated, const FCurrentUserState&, UserState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotsUpdated, const TArray<FStageSlotItem>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUseRequested, const FItemUseRequestPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPresenceMessage, const FString&, RawText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPresenceStateChanged, const FPresenceState&, State);
