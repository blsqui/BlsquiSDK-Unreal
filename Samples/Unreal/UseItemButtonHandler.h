#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlsquiSDKComponent.h"
#include "BlsquiCurrentUser.h"
#include "UseItemButtonHandler.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnItemTxResultModalReady,
    const FString&, Title,
    const FString&, Status,
    const FString&, DetailsText,
    bool, bIsSuccess);

UCLASS(ClassGroup = (Blsqui), meta = (BlueprintSpawnableComponent))
class BLSQUISDK_API UUseItemButtonHandler : public UActorComponent
{
    GENERATED_BODY()

public:
    UUseItemButtonHandler();

    /** Call this directly when the user clicks the DEF or ATK button in UI */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Item")
    void OnUseItemButtonClicked(const FString& ItemId);

    /** Initialize references to CurrentUser and SDKComponent */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Item")
    void InitializeHandler(UBlsquiCurrentUser* InCurrentUser, UBlsquiSDKComponent* InSDKComponent);

    /** Modal display event for UI (e.g. WBP_MainHUD) */
    UPROPERTY(BlueprintAssignable, Category = "Blsqui|Item")
    FOnItemTxResultModalReady OnItemTxResultModalReady;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TWeakObjectPtr<UBlsquiCurrentUser> CurrentUser;

    UPROPERTY()
    TWeakObjectPtr<UBlsquiSDKComponent> SDKComponent;

    UFUNCTION()
    void HandleItemUseRequested(const FItemUseRequestPayload& Payload);

    UFUNCTION()
    void HandleTransactionCompleted(const FTxResult& Result);
};
