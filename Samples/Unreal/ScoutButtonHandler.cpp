#include "ScoutButtonHandler.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/Button.h"

UScoutButtonHandler::UScoutButtonHandler()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UScoutButtonHandler::ScoutOpponentSlots()
{
    UE_LOG(LogTemp, Log, TEXT("[Scout] Querying equipped slots for: %s"), *TARGET_ADDRESS);

    // 1. Show immediate loading feedback on the dialog
    if (TournamentDialog)
    {
        if (CancelButton)
        {
            CancelButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (TitleText)
        {
            TitleText->SetText(FText::FromString(TEXT("Scouting Opponent...")));
        }
        if (MessageText)
        {
            FString Msg = FString::Printf(TEXT("Connecting to Flow Access Node...\nTarget: %s"), *TARGET_ADDRESS);
            MessageText->SetText(FText::FromString(Msg));
        }
        TournamentDialog->SetVisibility(ESlateVisibility::Visible);
    }

    // 2. Fetch equipped slots from Flow Testnet
    FStageSlotOptions Options;
    Options.TargetAddress = TARGET_ADDRESS;
    Options.PublisherAddress = PUBLISHER_ADDRESS;
    Options.AppId = APP_ID;
    Options.bIsTestnet = true;
    Options.bVerbose = true;

    FOnStageSlotsResolved Callback;
    Callback.BindDynamic(this, &UScoutButtonHandler::HandleSlotsResolved);

    UBlsquiStageSlot::GetStageSlots(Options, Callback);
}

void UScoutButtonHandler::HandleSlotsResolved(const TArray<FStageSlotItem>& Items, bool bSuccess)
{
    if (!TournamentDialog || !MessageText)
    {
        return;
    }

    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("Opponent Loadout")));
    }

    if (!bSuccess)
    {
        MessageText->SetText(FText::FromString(TEXT("Failed to retrieve stage slots from Flow Access Node.")));
        return;
    }

    const int32 MaxSlotsToShow = 3;
    FString ResultString;
    ResultString += FString::Printf(TEXT("Target: %s\n"), *TARGET_ADDRESS);
    ResultString += TEXT("Equipped Item Slots:\n\n");

    bool bHasAnyEquipped = false;
    int32 Count = 0;

    for (const FStageSlotItem& Slot : Items)
    {
        // ========================================================================
        // Detailed Slot Metadata Available via FStageSlotItem:
        // - Slot.SlotIndex        : 1-based equipped slot index (1 to 8)
        // - Slot.ItemId           : On-chain unique item ID (e.g., "shd_valkyrie_aegis_b")
        // - Slot.ItemName         : Localized display name (e.g., "Valkyrie Shield Ⅱ")
        // - Slot.RemainingCharges : Current remaining durability/charges (e.g., 0)
        // - Slot.MaxCharges       : Maximum charges/durability cap (e.g., 3)
        // - Slot.Category         : Item category / equipment type
        // - Slot.Icon             : WebP image asset URL hosted on CDN
        // - Slot.FlixId           : Interaction template ID for transaction verification
        // ========================================================================

        if (!Slot.ItemId.IsEmpty())
        {
            // Verbose debug log detailing item specifications and durability
            UE_LOG(LogTemp, Log, TEXT("[Scout] Slot %d -> '%s' (ID: %s) [%d/%d charges] Category: '%s' Icon: '%s'"),
                Slot.SlotIndex,
                Slot.ItemName.IsEmpty() ? TEXT("(No Name)") : *Slot.ItemName,
                *Slot.ItemId,
                Slot.RemainingCharges,
                Slot.MaxCharges,
                Slot.Category.IsEmpty() ? TEXT("None") : *Slot.Category,
                Slot.Icon.IsEmpty() ? TEXT("None") : *Slot.Icon);
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("[Scout] Slot %d -> (Empty)"), Slot.SlotIndex);
        }

        // Limit dialog UI display to the first few preview slots
        if (Count < MaxSlotsToShow)
        {
            if (!Slot.ItemId.IsEmpty())
            {
                bHasAnyEquipped = true;
                FString DisplayName = Slot.ItemName.IsEmpty() ? Slot.ItemId : Slot.ItemName;

                if (Slot.MaxCharges > 0)
                {
                    ResultString += FString::Printf(TEXT("• Slot %d: %s [%d/%d]\n"),
                        Slot.SlotIndex,
                        *DisplayName,
                        Slot.RemainingCharges,
                        Slot.MaxCharges);
                }
                else
                {
                    ResultString += FString::Printf(TEXT("• Slot %d: %s\n"), Slot.SlotIndex, *DisplayName);
                }
            }
            else
            {
                ResultString += FString::Printf(TEXT("• Slot %d: (Empty)\n"), Slot.SlotIndex);
            }
            Count++;
        }
    }

    if (!bHasAnyEquipped)
    {
        ResultString += TEXT("No items equipped in stage slots.\n");
    }

    MessageText->SetText(FText::FromString(ResultString));
}

void UScoutButtonHandler::OnCloseScoutDialog()
{
    if (TournamentDialog)
    {
        TournamentDialog->SetVisibility(ESlateVisibility::Collapsed);
    }
}
