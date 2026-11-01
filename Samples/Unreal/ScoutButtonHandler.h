#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BlsquiStageSlot.h"
#include "ScoutButtonHandler.generated.h"

class UTextBlock;
class UWidget;

/**
 * Unreal Engine sample component mirroring Unity's ScoutButtonHandler.cs.
 * Manages loading feedback, queries on-chain slots via UBlsquiStageSlot,
 * and updates UMG UI elements.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UScoutButtonHandler : public UActorComponent
{
    GENERATED_BODY()

public:
    UScoutButtonHandler();

    /** Triggers the on-chain query and shows dialog feedback. */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Sample")
    void ScoutOpponentSlots();

    /** Closes or hides the modal dialog. */
    UFUNCTION(BlueprintCallable, Category = "Blsqui|Sample")
    void OnCloseScoutDialog();

    // UI bindings (Can be assigned in Blueprint defaults or Details panel)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI References")
    TObjectPtr<UWidget> TournamentDialog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI References")
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI References")
    TObjectPtr<UTextBlock> MessageText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI References")
    TObjectPtr<UWidget> CancelButton;

private:
    const FString TARGET_ADDRESS    = TEXT("0x2f13c512df0b0ad7");
    const FString PUBLISHER_ADDRESS = TEXT("0x2f13c512df0b0ad7");
    const FString APP_ID            = TEXT("valkyrie_vanguard");

    UFUNCTION()
    void HandleSlotsResolved(const TArray<FStageSlotItem>& Items, bool bSuccess);
};