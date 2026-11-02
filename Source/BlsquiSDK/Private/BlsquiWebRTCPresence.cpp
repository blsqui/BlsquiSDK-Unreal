#include "BlsquiWebRTCPresence.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Async/Async.h"

THIRD_PARTY_INCLUDES_START
#include <rtc/rtc.hpp>
THIRD_PARTY_INCLUDES_END

UBlsquiWebRTCPresence::UBlsquiWebRTCPresence()
{
    // Enable verbose internal logging from libdatachannel
    static bool bLoggerInitialized = false;
    if (!bLoggerInitialized)
    {
        rtc::InitLogger(rtc::LogLevel::Debug, [](rtc::LogLevel Level, std::string Message)
        {
            FString LogMsg = UTF8_TO_TCHAR(Message.c_str());
            UE_LOG(LogTemp, Log, TEXT("[libdatachannel] %s"), *LogMsg);
        });
        bLoggerInitialized = true;
    }
}

UBlsquiWebRTCPresence::~UBlsquiWebRTCPresence()
{
    Cleanup();
}

void UBlsquiWebRTCPresence::InitPresence(const FString& PlayerAddress, const FString& Role)
{
    FString CleanAddr = PlayerAddress.TrimStartAndEnd().ToLower();
    if (CleanAddr.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[WebRTC] Missing player address. Aborting handshake."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[WebRTC] InitPresence called for room/player: %s (role: %s)"), *CleanAddr, *Role);

    if (bIsConnecting || (PeerConnection && ActiveAddress == CleanAddr && State.bConnected))
    {
        UE_LOG(LogTemp, Warning, TEXT("[WebRTC] Already connecting or connected. Skipping."));
        return;
    }

    Cleanup();

    bIsConnecting = true;
    ActiveAddress = CleanAddr;

    // 1. Configure STUN servers
    rtc::Configuration Config;
    Config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    Config.iceServers.emplace_back("stun:stun.cloudflare.com:3478");

    PeerConnection = std::make_shared<rtc::PeerConnection>(Config);

    TWeakObjectPtr<UBlsquiWebRTCPresence> WeakThis(this);

    // 2. Setup DataChannel
    rtc::DataChannelInit DcInit;
    DcInit.reliability.unordered = false;
    DataChannel = PeerConnection->createDataChannel("blsqui-presence", DcInit);

    DataChannel->onOpen([WeakThis]()
    {
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("✅ [UE5 WebRTC DataChannel OPEN]"));
                WeakThis->State.bConnected = true;
                WeakThis->OnStateChanged.Broadcast(WeakThis->State);
            }
        });
    });

    DataChannel->onClosed([WeakThis]()
    {
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("🛑 [UE5 WebRTC DataChannel CLOSED]"));
                WeakThis->State.bConnected = false;
                WeakThis->State.bGameOnline = false;
                WeakThis->State.bCanUseItem = false;
                WeakThis->OnStateChanged.Broadcast(WeakThis->State);
            }
        });
    });

    DataChannel->onMessage([WeakThis](auto Data)
    {
        if (std::holds_alternative<std::string>(Data))
        {
            FString RawText = UTF8_TO_TCHAR(std::get<std::string>(Data).c_str());
            AsyncTask(ENamedThreads::GameThread, [WeakThis, RawText]()
            {
                if (WeakThis.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("📩 [UE5 WebRTC RECV]: %s"), *RawText);
                    WeakThis->HandleIncomingRaw(RawText);
                }
            });
        }
    });

    // Track whether offer was already posted for this session
    auto bOfferSent = std::make_shared<std::atomic<bool>>(false);

    // 3. Callback when local description offer is formed
    PeerConnection->onLocalDescription([WeakThis, Role, CleanAddr, bOfferSent](rtc::Description Desc)
    {
        std::string SdpStr = Desc;
        FString LocalSdp = UTF8_TO_TCHAR(SdpStr.c_str());
        UE_LOG(LogTemp, Log, TEXT("[WebRTC] onLocalDescription raw event fired (length: %d)"), LocalSdp.Len());

        AsyncTask(ENamedThreads::GameThread, [WeakThis, LocalSdp, Role, CleanAddr, bOfferSent]()
        {
            if (WeakThis.IsValid() && !bOfferSent->exchange(true))
            {
                UE_LOG(LogTemp, Log, TEXT("[WebRTC] Posting SDP offer to signaling server..."));
                WeakThis->SendOfferToSignalingServer(LocalSdp, Role, CleanAddr);
            }
        });
    });

    // 4. Candidate gathering callback (required by some versions of libdatachannel to complete offer)
    PeerConnection->onLocalCandidate([](rtc::Candidate Candidate)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[WebRTC] Local candidate gathered: %s"), UTF8_TO_TCHAR(std::string(Candidate).c_str()));
    });

    PeerConnection->onGatheringStateChange([WeakThis, Role, CleanAddr, bOfferSent](rtc::PeerConnection::GatheringState GState)
    {
        UE_LOG(LogTemp, Log, TEXT("[WebRTC] ICE Gathering State changed: %d"), static_cast<int32>(GState));
        if (GState == rtc::PeerConnection::GatheringState::Complete)
        {
            AsyncTask(ENamedThreads::GameThread, [WeakThis, Role, CleanAddr, bOfferSent]()
            {
                if (WeakThis.IsValid() && WeakThis->PeerConnection && !bOfferSent->exchange(true))
                {
                    auto OptDesc = WeakThis->PeerConnection->localDescription();
                    if (OptDesc.has_value())
                    {
                        FString LocalSdp = UTF8_TO_TCHAR(std::string(OptDesc.value()).c_str());
                        UE_LOG(LogTemp, Log, TEXT("[WebRTC] Gathering complete. Posting full SDP (length: %d)..."), LocalSdp.Len());
                        WeakThis->SendOfferToSignalingServer(LocalSdp, Role, CleanAddr);
                    }
                }
            });
        }
    });

    // 5. Trigger offer generation
    PeerConnection->setLocalDescription();
    UE_LOG(LogTemp, Log, TEXT("[WebRTC] PeerConnection setLocalDescription() triggered."));
}

void UBlsquiWebRTCPresence::SendOfferToSignalingServer(const FString& SdpOffer, const FString& Role, const FString& CleanAddr)
{
    TSharedPtr<FJsonObject> SdpObj = MakeShared<FJsonObject>();
    SdpObj->SetStringField(TEXT("type"), TEXT("offer"));
    SdpObj->SetStringField(TEXT("sdp"), SdpOffer);

    TSharedPtr<FJsonObject> PayloadObj = MakeShared<FJsonObject>();
    PayloadObj->SetStringField(TEXT("role"), Role);
    PayloadObj->SetStringField(TEXT("player"), CleanAddr);
    PayloadObj->SetObjectField(TEXT("sdp"), SdpObj);

    FString RequestJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestJson);
    FJsonSerializer::Serialize(PayloadObj.ToSharedRef(), Writer);

    UE_LOG(LogTemp, Log, TEXT("[WebRTC] Posting offer to Pion Hub [%s] for room: %s"), *SIGNALING_API, *CleanAddr);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(SIGNALING_API);
    Req->SetVerb(TEXT("POST"));
    Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Req->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Req->SetContentAsString(RequestJson);

    Req->OnProcessRequestComplete().BindUObject(this, &UBlsquiWebRTCPresence::OnSignalingResponse);
    Req->ProcessRequest();
}

void UBlsquiWebRTCPresence::OnSignalingResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    bIsConnecting = false;

    if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("[WebRTC] Signaling handshake failed. HTTP Code: %d, Response: %s"),
            Response.IsValid() ? Response->GetResponseCode() : 0,
            Response.IsValid() ? *Response->GetContentAsString() : TEXT("No Response"));
        State.bConnected = false;
        OnStateChanged.Broadcast(State);
        return;
    }

    FString ResContent = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("[WebRTC] Signaling server response received (length: %d)"), ResContent.Len());

    FString AnswerSdp = ExtractAnswerSdp(ResContent);
    if (!AnswerSdp.IsEmpty() && PeerConnection)
    {
        PeerConnection->setRemoteDescription(rtc::Description(TCHAR_TO_UTF8(*AnswerSdp), "answer"));
        UE_LOG(LogTemp, Log, TEXT("🚀 [WebRTC] Remote description (answer) set successfully. Waiting for DataChannel open..."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[WebRTC] Failed to extract Answer SDP from response: %s"), *ResContent);
    }
}

FString UBlsquiWebRTCPresence::ExtractAnswerSdp(const FString& Json)
{
    TSharedPtr<FJsonObject> RootObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (FJsonSerializer::Deserialize(Reader, RootObj) && RootObj.IsValid())
    {
        if (RootObj->HasField(TEXT("sdp")))
        {
            const TSharedPtr<FJsonObject>* NestedObj;
            if (RootObj->TryGetObjectField(TEXT("sdp"), NestedObj))
            {
                return (*NestedObj)->GetStringField(TEXT("sdp"));
            }
            return RootObj->GetStringField(TEXT("sdp"));
        }
    }
    return TEXT("");
}

void UBlsquiWebRTCPresence::SendData(const FString& Payload)
{
    if (DataChannel && DataChannel->isOpen())
    {
        UE_LOG(LogTemp, Log, TEXT("📤 [UE5 WebRTC SEND]: %s"), *Payload);
        DataChannel->send(TCHAR_TO_UTF8(*Payload));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[WebRTC] Send dropped: DataChannel not open (dc: %s)"),
            DataChannel ? TEXT("valid but not open") : TEXT("null"));
    }
}

void UBlsquiWebRTCPresence::HandleIncomingRaw(const FString& RawText)
{
    if (RawText.Contains(TEXT("PRESENCE_UPDATE")))
    {
        State.bGameOnline = RawText.Contains(TEXT("GAME_CONNECTED"));
        State.bCanUseItem = RawText.Contains(TEXT("\"can_use_item\":true")) || RawText.Contains(TEXT("\"can_use_item\": true"));
        OnStateChanged.Broadcast(State);
    }

    OnMessageReceived.Broadcast(RawText);
}

void UBlsquiWebRTCPresence::Cleanup()
{
    if (DataChannel)
    {
        DataChannel->close();
        DataChannel.reset();
    }
    if (PeerConnection)
    {
        PeerConnection->close();
        PeerConnection.reset();
    }
    ActiveAddress = TEXT("");
    bIsConnecting = false;
    State.bConnected = false;
    State.bGameOnline = false;
    State.bCanUseItem = false;
}
