#include "HubButtonHandler.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

UHubButtonHandler::UHubButtonHandler()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHubButtonHandler::BeginPlay()
{
    Super::BeginPlay();

    // Cache GameInstance Subsystem and bind WebRTC presence delegates
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
    {
        CachedUserSubsystem = GI->GetSubsystem<UBlsquiCurrentUser>();
        if (CachedUserSubsystem)
        {
            CachedUserSubsystem->OnUserUpdated.AddDynamic(this, &UHubButtonHandler::HandleUserUpdated);
            CachedUserSubsystem->OnSlotsUpdated.AddDynamic(this, &UHubButtonHandler::HandleSlotsUpdated);
        }
    }
}

void UHubButtonHandler::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CachedUserSubsystem)
    {
        CachedUserSubsystem->OnUserUpdated.RemoveDynamic(this, &UHubButtonHandler::HandleUserUpdated);
        CachedUserSubsystem->OnSlotsUpdated.RemoveDynamic(this, &UHubButtonHandler::HandleSlotsUpdated);
    }

    Super::EndPlay(EndPlayReason);
}

void UHubButtonHandler::OpenHub()
{
    if (!CachedUserSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[Hub] UBlsquiCurrentUser subsystem not found."));
        return;
    }

    // 1. Show immediate UI loading feedback
    if (HubDialog)
    {
        if (CancelButton)
        {
            CancelButton->SetVisibility(ESlateVisibility::Visible);
        }
        if (TitleText)
        {
            TitleText->SetText(FText::FromString(TEXT("Blsqui Web3 Hub")));
        }
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("Opening dashboard & establishing WebRTC presence...")));
        }
        HubDialog->SetVisibility(ESlateVisibility::Visible);
    }

    // 2. Configure app parameters and open browser dashboard
    CachedUserSubsystem->Configure(AppId, PublisherAddress, Network);
    CachedUserSubsystem->OpenDashboard();
}

void UHubButtonHandler::HandleUserUpdated(const FCurrentUserState& State)
{
    if (!StatusText)
    {
        return;
    }

    if (State.bLoggedIn)
    {
        FString UserInfo = FString::Printf(TEXT("Connected: %s (%s)"), 
            State.UserName.IsEmpty() ? TEXT("Player") : *State.UserName,
            *State.Addr);
        StatusText->SetText(FText::FromString(UserInfo));
    }
    else
    {
        StatusText->SetText(FText::FromString(TEXT("Disconnected / Logged Out")));
    }
}

void UHubButtonHandler::HandleSlotsUpdated(const TArray<FStageSlotItem>& Slots)
{
    if (!SlotsText)
    {
        return;
    }

    FString ResultString = TEXT("Equipped Stage Slots:\n\n");
    bool bHasAnyEquipped = false;

    for (const FStageSlotItem& Slot : Slots)
    {
        if (!Slot.ItemId.IsEmpty())
        {
            bHasAnyEquipped = true;

            FString DisplayName = Slot.ItemName.IsEmpty() ? Slot.ItemId : Slot.ItemName;

            // Render slot index, name, and charge durability (e.g., [3/3] or [50/50])
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
                ResultString += FString::Printf(TEXT("• Slot %d: %s\n"), 
                    Slot.SlotIndex, 
                    *DisplayName);
            }
        }
        else
        {
            ResultString += FString::Printf(TEXT("• Slot %d: (Empty)\n"), Slot.SlotIndex);
        }
    }

    if (!bHasAnyEquipped)
    {
        ResultString += TEXT("No items currently equipped.\n");
    }

    SlotsText->SetText(FText::FromString(ResultString));
}

void UHubButtonHandler::UseEquippedItem(const FString& ItemId)
{
    if (CachedUserSubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("[Hub] Requesting action for item: %s"), *ItemId);
        CachedUserSubsystem->RequestUseItem(ItemId);
    }
}

void UHubButtonHandler::OnCloseHubDialog()
{
    if (HubDialog)
    {
        HubDialog->SetVisibility(ESlateVisibility::Collapsed);
    }
}