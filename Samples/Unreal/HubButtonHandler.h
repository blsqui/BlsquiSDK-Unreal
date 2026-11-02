#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlsquiCurrentUser.h"
#include "BlsquiTypes.h"
#include "HubButtonHandler.generated.h"

class UTextBlock;
class UWidget;
class UButton;

/**
 * UHubButtonHandler
 * Demonstrates launching the Blsqui Web3 Hub dashboard,
 * binding to real-time WebRTC presence events, and rendering
 * equipped items with remaining / max charges (durability).
 */
UCLASS(ClassGroup = (Blsqui), meta = (BlueprintSpawnableComponent))
class BLSQUISDK_API UHubButtonHandler : public UActorComponent
{
    GENERATED_BODY()

public:
    UHubButtonHandler();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Triggers the dashboard launch and starts WebRTC signaling */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Hub")
    void OpenHub();

    /** Closes the HUD dialog window */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Hub")
    void OnCloseHubDialog();

    /** Manually triggers item usage over the active DataChannel */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Hub")
    void UseEquippedItem(const FString& ItemId);

    // --- Configuration ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui|Config")
    FString AppId = TEXT("valkyrie_vanguard");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui|Config")
    FString PublisherAddress = TEXT("0x2f13c512df0b0ad7");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blsqui|Config")
    FString Network = TEXT("testnet");

    // --- UI Element References ---
    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UWidget> HubDialog;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UTextBlock> SlotsText;

    UPROPERTY(BlueprintReadWrite, Category = "Blsqui|UI")
    TObjectPtr<UButton> CancelButton;

private:
    UPROPERTY()
    TObjectPtr<UBlsquiCurrentUser> CachedUserSubsystem;

    UFUNCTION()
    void HandleUserUpdated(const FCurrentUserState& State);

    UFUNCTION()
    void HandleSlotsUpdated(const TArray<FStageSlotItem>& Slots);
};