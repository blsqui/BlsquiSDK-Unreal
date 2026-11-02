#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Interfaces/IHttpRequest.h"
#include "BlsquiStageSlot.generated.h"

USTRUCT(BlueprintType)
struct FStageSlotOptions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui StageSlot")
    FString TargetAddress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui StageSlot")
    FString PublisherAddress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui StageSlot")
    FString AppId = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui StageSlot")
    bool bIsTestnet = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui StageSlot")
    bool bVerbose = false;
};

USTRUCT(BlueprintType)
 struct FStageSlotItem
 {
     GENERATED_BODY()
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     int32 SlotIndex = 0;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     FString ItemId;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     FString ItemName;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     FString Icon;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     FString Category;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     int32 RemainingCharges = 0;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     int32 MaxCharges = 0;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     FString FlixId;
 
     UPROPERTY(BlueprintReadOnly, Category = "Blsqui StageSlot")
     FString PublisherAddress;
 };

// Asynchronous delegate fired when the REST call completes
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnStageSlotsResolved, const TArray<FStageSlotItem>&, Items, bool, bSuccess);

UCLASS()
class BLSQUISDK_API UBlsquiStageSlot : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Queries on-chain SesamePass stage slot equipment via Flow Access Node REST API.
     */
    UFUNCTION(BlueprintCallable, Category = "Blsqui SDK|StageSlot")
    static void GetStageSlots(const FStageSlotOptions& Options, FOnStageSlotsResolved OnResolved);

private:
    static const FString TESTNET_ACCESS_NODE;
    static const FString MAINNET_ACCESS_NODE;
    static const FString SESAME_CONTRACT_TESTNET;
    static const FString SESAME_CONTRACT_MAINNET;
    static const FString CADENCE_SCRIPT_TEMPLATE;
};
