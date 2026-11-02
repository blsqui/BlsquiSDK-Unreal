#include "BlsquiCurrentUser.h"
#include "BlsquiSDKComponent.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

void UBlsquiCurrentUser::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Presence = NewObject<UBlsquiWebRTCPresence>(this);
    Presence->OnMessageReceived.AddDynamic(this, &UBlsquiCurrentUser::OnPresenceMessageReceived);
}

void UBlsquiCurrentUser::Deinitialize()
{
    StopHeartbeat();
    if (Presence)
    {
        Presence->Cleanup();
    }
    Super::Deinitialize();
}

void UBlsquiCurrentUser::Configure(const FString& InAppId, const FString& InPublisherAddress, const FString& InNetwork)
{
    AppId = InAppId.TrimStartAndEnd().ToLower();
    PublisherAddress = InPublisherAddress.TrimStartAndEnd();
    Network = InNetwork;
    UserState.Network = Network;
}

FString UBlsquiCurrentUser::GenerateChannelKey()
{
    uint8 RandomBytes[32];
    for (int32 i = 0; i < 32; ++i)
    {
        RandomBytes[i] = FMath::RandRange(0, 255);
    }
    return FString::FromHexBlob(RandomBytes, 32).ToLower();
}

FString UBlsquiCurrentUser::GetOrCreateChannelKey()
{
    if (SessionChannelKey.IsEmpty())
    {
        SessionChannelKey = GenerateChannelKey();
    }
    return SessionChannelKey;
}

void UBlsquiCurrentUser::OpenDashboard()
{
    const FString BaseUrl = (Network == TEXT("testnet")) ? TESTNET_BASE_URL : MAINNET_BASE_URL;
    const FString ChannelKey = GetOrCreateChannelKey();

    // 1. Initialize WebRTC peer session
    Presence->InitPresence(ChannelKey, TEXT("game"));

    // 2. Construct dashboard URL
    FString Query = FString::Printf(TEXT("channel_key=%s"), *FGenericPlatformHttp::UrlEncode(ChannelKey));

    if (!AppId.IsEmpty())
    {
        Query += FString::Printf(TEXT("&app_id=%s"), *FGenericPlatformHttp::UrlEncode(AppId));
    }
    if (!PublisherAddress.IsEmpty())
    {
        Query += FString::Printf(TEXT("&publisher_address=%s"), *FGenericPlatformHttp::UrlEncode(PublisherAddress));
    }
    if (!UserState.Addr.IsEmpty())
    {
        Query += FString::Printf(TEXT("&addr=%s"), *FGenericPlatformHttp::UrlEncode(UserState.Addr));
    }

    const FString FullUrl = FString::Printf(TEXT("%s/?%s"), *BaseUrl, *Query);
    UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Opening Dashboard: %s"), *FullUrl);

    FPlatformProcess::LaunchURL(*FullUrl, nullptr, nullptr);
}

void UBlsquiCurrentUser::RequestUseItem(const FString& ItemId)
{
    if (!Presence) return;

    int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
    FString Payload = FString::Printf(
        TEXT("{\"type\":\"TRIGGER_ITEM_ACTION\",\"action\":\"use_equipped_item\",\"itemId\":\"%s\",\"player\":\"%s\",\"timestamp\":%lld}"),
        *ItemId, *UserState.Addr.ToLower(), NowMs);

    Presence->SendData(Payload);
}

void UBlsquiCurrentUser::OnPresenceMessageReceived(const FString& RawText)
{
    if (RawText.Contains(TEXT("PRESENCE_PING")))
    {
        // If our stored address is empty, adopt the player address from the dashboard's ping
        if (UserState.Addr.IsEmpty())
        {
            FString PingPlayer = ExtractStringValue(RawText, TEXT("player"));
            if (!PingPlayer.IsEmpty())
            {
                UserState.Addr = PingPlayer;
            }
        }
        SendPresenceLiveSignal();
        return;
    }

    if (RawText.Contains(TEXT("BLSQUI_LOGOUT")))
    {
        UserState.Addr = TEXT("");
        UserState.bLoggedIn = false;
        UserState.AvatarImage = TEXT("");
        UserState.UserName = TEXT("");
        StopHeartbeat();
        OnUserUpdated.Broadcast(UserState);
        OnSlotsUpdated.Broadcast(TArray<FStageSlotItem>());
        return;
    }

    if (RawText.Contains(TEXT("BLSQUI_USER_UPDATE")) || RawText.Contains(TEXT("SLOT_SYNC")) || RawText.Contains(TEXT("BLSQUI_SLOT_SYNC")))
    {
        FString Addr = ExtractStringValue(RawText, TEXT("address"));
        if (Addr.IsEmpty())
        {
            Addr = ExtractStringValue(RawText, TEXT("player"));
        }

        if (!Addr.IsEmpty())
        {
            UserState.Addr = Addr;
            UserState.bLoggedIn = true;
            UserState.UserName = ExtractStringValue(RawText, TEXT("userName"));
            UserState.AvatarImage = ExtractStringValue(RawText, TEXT("avatarImage"));

            StartHeartbeat();
            OnUserUpdated.Broadcast(UserState);
        }

        TArray<FStageSlotItem> Slots = ExtractSlotsFromPayload(RawText);
        if (Slots.Num() > 0)
        {
            OnSlotsUpdated.Broadcast(Slots);
        }
    }

    if (RawText.Contains(TEXT("ITEM_USE_REQUESTED")))
    {
        FString ReqFlixId = ExtractStringValue(RawText, TEXT("flixId"));
        if (ReqFlixId.IsEmpty()) ReqFlixId = ExtractStringValue(RawText, TEXT("flix_id"));

        FString ReqItemId = ExtractStringValue(RawText, TEXT("itemId"));
        if (ReqItemId.IsEmpty()) ReqItemId = ExtractStringValue(RawText, TEXT("item_id"));

        int32 Serial = ExtractIntValue(RawText, TEXT("serialNumber"));
        if (Serial <= 0) Serial = ExtractIntValue(RawText, TEXT("serial_number"));

        FItemUseRequestPayload Payload;
        Payload.Type = TEXT("ITEM_USE_REQUESTED");
        Payload.FlixId = ReqFlixId;
        Payload.ItemId = ReqItemId;
        Payload.Player = ExtractStringValue(RawText, TEXT("player"));
        Payload.AppId = ExtractStringValue(RawText, TEXT("app_id"));
        Payload.SerialNumber = Serial;

        // Broadcast the event to whoever is listening (HUD or Character)
        OnItemUseRequested.Broadcast(Payload);
    }
}

TArray<FStageSlotItem> UBlsquiCurrentUser::ExtractSlotsFromPayload(const FString& RawText)
{
    TArray<FStageSlotItem> Result;

    TSharedPtr<FJsonObject> RootObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawText);

    if (FJsonSerializer::Deserialize(Reader, RootObj) && RootObj.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;

        if (RootObj->TryGetArrayField(TEXT("slots"), SlotsArray) || RootObj->TryGetArrayField(TEXT("slot"), SlotsArray))
        {
            for (const auto& Val : *SlotsArray)
            {
                TSharedPtr<FJsonObject> SlotObj = Val->AsObject();
                if (!SlotObj.IsValid()) continue;

                FStageSlotItem Item;
                Item.SlotIndex = SlotObj->GetIntegerField(TEXT("slotIndex"));
                Item.ItemId = SlotObj->GetStringField(TEXT("itemId")); // Empty string if unequipped
                Item.ItemName = SlotObj->GetStringField(TEXT("itemName"));
                Item.Icon = SlotObj->GetStringField(TEXT("icon"));
                Item.RemainingCharges = SlotObj->GetIntegerField(TEXT("remainingCharges"));
                Item.MaxCharges = SlotObj->GetIntegerField(TEXT("maxCharges"));
                Result.Add(Item);
            }
        }
    }

    // Sort by SlotIndex (1 through 8)
    Result.Sort([](const FStageSlotItem& A, const FStageSlotItem& B) {
        return A.SlotIndex < B.SlotIndex;
    });

    UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Extracted %d total slots from payload"), Result.Num());
    return Result;
}

void UBlsquiCurrentUser::StartHeartbeat()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            HeartbeatTimerHandle, this, &UBlsquiCurrentUser::SendPresenceLiveSignal, 10.0f, true);
    }
}

void UBlsquiCurrentUser::StopHeartbeat()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
    }
}

void UBlsquiCurrentUser::SendPresenceLiveSignal()
{
    int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
    FString PongJson = FString::Printf(
        TEXT("{\"type\":\"PRESENCE_PONG\",\"role\":\"game\",\"player\":\"%s\",\"status\":\"LIVE\",\"game_online\":true,\"timestamp\":%lld}"),
        *UserState.Addr.ToLower(), NowMs);

    Presence->SendData(PongJson);
}

FString UBlsquiCurrentUser::ExtractStringValue(const FString& Json, const FString& Key)
{
    FString Pattern = FString::Printf(TEXT("\"%s\":\""), *Key);
    int32 Start = Json.Find(Pattern);
    if (Start == INDEX_NONE) return TEXT("");
    Start += Pattern.Len();
    int32 End = Json.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, Start);
    return (End != INDEX_NONE) ? Json.Mid(Start, End - Start) : TEXT("");
}

int32 UBlsquiCurrentUser::ExtractIntValue(const FString& Json, const FString& Key)
{
    FString Pattern = FString::Printf(TEXT("\"%s\":"), *Key);
    int32 Start = Json.Find(Pattern);
    if (Start == INDEX_NONE) return 0;
    Start += Pattern.Len();

    while (Start < Json.Len() && FChar::IsWhitespace(Json[Start]))
    {
        Start++;
    }

    int32 End = Start;
    while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '-'))
    {
        End++;
    }

    if (End > Start)
    {
        return FCString::Atoi(*Json.Mid(Start, End - Start));
    }
    return 0;
}
