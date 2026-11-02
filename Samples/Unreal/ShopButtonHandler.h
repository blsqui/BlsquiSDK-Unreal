#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlsquiSDKComponent.h"
#include "ShopButtonHandler.generated.h"

class UTextBlock;
class UWidget;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnShopResultReady, 
    const FString&, Title, 
    const FString&, Status, 
    const FString&, DetailsText, 
    bool, bIsSuccess);

/**
 * UShopButtonHandler
 * Demonstrates executing in-game shop transactions using FLIX interaction templates
 * via UBlsquiSDKComponent.
 */
UCLASS(ClassGroup = (Blsqui), meta = (BlueprintSpawnableComponent))
class BLSQUISDK_API UShopButtonHandler : public UActorComponent
{
    GENERATED_BODY()

public:
    UShopButtonHandler();

    virtual void BeginPlay() override;

    /** Requests a purchase transaction using the configured FLIX template */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Shop")
    void ExecuteShopPurchase();

    /** Closes the shop dialog window */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Shop")
    void OnCloseShopDialog();

    // --- Transaction Parameters ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui|Config")
    FString FlixId = TEXT("351d9d6f23ebe5ddbe9bf5b4f937a352c5caaa6a4439b5e464f3b16c1d671472");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui|Config")
    FString MerchantAddress = TEXT("0xa090f900023d6d34");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui|Config")
    bool bIsTestnet = true;

    // --- UI References ---
    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UWidget> ShopDialog;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UButton> CancelButton;

    UPROPERTY(BlueprintAssignable, Category = "Blsqui|Shop")
    FOnShopResultReady OnShopResultReady;

    /** Helper that formats an FTxResult struct into display-ready strings */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Shop")
    static void FormatTxResultForModal(
        const FTxResult& Result, 
        FString& OutTitle, 
        FString& OutStatus, 
        FString& OutDetails, 
        bool& bOutSuccess);

private:
    UPROPERTY()
    TObjectPtr<UBlsquiSDKComponent> BlsquiSDKComp;

    UFUNCTION()
    void HandleTransactionSealed(const FString& TxId, bool bSuccess);
};
