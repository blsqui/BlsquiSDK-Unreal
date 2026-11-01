#include "BlsquiStageSlot.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Base64.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Internationalization/Regex.h"

const FString UBlsquiStageSlot::TESTNET_ACCESS_NODE = TEXT("https://rest-testnet.onflow.org/v1/scripts");
const FString UBlsquiStageSlot::MAINNET_ACCESS_NODE = TEXT("https://rest-mainnet.onflow.org/v1/scripts");
const FString UBlsquiStageSlot::SESAME_CONTRACT_TESTNET = TEXT("0x975b04756864e9ea");
const FString UBlsquiStageSlot::SESAME_CONTRACT_MAINNET = TEXT("0xb576a3926d239682");

const FString UBlsquiStageSlot::CADENCE_SCRIPT_TEMPLATE = TEXT(
    "import SesamePass from {0}\n\n"
    "access(all) fun main(publisher: Address, user: Address, appId: String): [String]? {\n"
    "    return SesamePass.getUserItemSlots(publisher: publisher, user: user, appId: appId)\n"
    "}"
);

void UBlsquiStageSlot::GetStageSlots(const FStageSlotOptions& Options, FOnStageSlotsResolved OnResolved)
{
    TArray<FStageSlotItem> EmptyResults;

    if (Options.TargetAddress.IsEmpty() || Options.PublisherAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[BlsquiSDK] targetAddress and publisherAddress are required."));
        OnResolved.ExecuteIfBound(EmptyResults, false);
        return;
    }

    // 1. Sanitize hex addresses (ensure 0x prefix)
    FString CleanTarget = Options.TargetAddress.TrimStartAndEnd();
    if (!CleanTarget.StartsWith(TEXT("0x"), ESearchCase::IgnoreCase))
    {
        CleanTarget = TEXT("0x") + CleanTarget;
    }

    FString CleanPublisher = Options.PublisherAddress.TrimStartAndEnd();
    if (!CleanPublisher.StartsWith(TEXT("0x"), ESearchCase::IgnoreCase))
    {
        CleanPublisher = TEXT("0x") + CleanPublisher;
    }

    // 2. Format Cadence Script & Base64 Encode
    const FString ContractAddr = Options.bIsTestnet ? SESAME_CONTRACT_TESTNET : SESAME_CONTRACT_MAINNET;
    const FString CadenceScript = FString::Format(*CADENCE_SCRIPT_TEMPLATE, { ContractAddr });
    const FString ScriptB64 = FBase64::Encode(CadenceScript);

    // 3. Construct Flow JSON-CDC Arguments
    const FString ArgPubJson = FString::Printf(TEXT("{\"type\":\"Address\",\"value\":\"%s\"}"), *CleanPublisher);
    const FString ArgUserJson = FString::Printf(TEXT("{\"type\":\"Address\",\"value\":\"%s\"}"), *CleanTarget);
    const FString ArgAppIdJson = FString::Printf(TEXT("{\"type\":\"String\",\"value\":\"%s\"}"), *Options.AppId);

    // 4. Construct Request JSON payload
    TSharedPtr<FJsonObject> PayloadObject = MakeShared<FJsonObject>();
    PayloadObject->SetStringField(TEXT("script"), ScriptB64);

    TArray<TSharedPtr<FJsonValue>> ArgumentsArray;
    ArgumentsArray.Add(MakeShared<FJsonValueString>(FBase64::Encode(ArgPubJson)));
    ArgumentsArray.Add(MakeShared<FJsonValueString>(FBase64::Encode(ArgUserJson)));
    ArgumentsArray.Add(MakeShared<FJsonValueString>(FBase64::Encode(ArgAppIdJson)));
    PayloadObject->SetArrayField(TEXT("arguments"), ArgumentsArray);

    FString RequestJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestJson);
    FJsonSerializer::Serialize(PayloadObject.ToSharedRef(), Writer);

    // 5. Send POST request to Flow Access Node
    const FString Endpoint = Options.bIsTestnet ? TESTNET_ACCESS_NODE : MAINNET_ACCESS_NODE;
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(Endpoint);
    HttpRequest->SetVerb(TEXT("POST"));
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetHeader(TEXT("Accept"), TEXT("application/json"));
    HttpRequest->SetContentAsString(RequestJson);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [Options, OnResolved](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            TArray<FStageSlotItem> Results;

            if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
            {
                if (Options.bVerbose)
                {
                    FString ErrText = Response.IsValid() ? Response->GetContentAsString() : TEXT("No response");
                    UE_LOG(LogTemp, Warning, TEXT("[BlsquiSDK] Failed to query target slots: %s"), *ErrText);
                }
                OnResolved.ExecuteIfBound(Results, false);
                return;
            }

            FString RawBody = Response->GetContentAsString().TrimStartAndEnd();
            FString DecodedJson = RawBody;

            // Flow REST sometimes wraps the JSON result in a quoted base64 string
            if (RawBody.StartsWith(TEXT("\"")) && RawBody.EndsWith(TEXT("\"")))
            {
                FString Base64Str = RawBody.Mid(1, RawBody.Len() - 2);
                FString TempDecoded;
                if (FBase64::Decode(Base64Str, TempDecoded))
                {
                    DecodedJson = TempDecoded;
                }
            }

            if (Options.bVerbose)
            {
                UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Decoded Cadence Object: %s"), *DecodedJson);
            }

            // 6. Regex extract all string values inside Cadence JSON-CDC array
            const FRegexPattern Pattern(TEXT("\\{\"(?:value\"\\s*:\\s*\"([^\"]*)\"\\s*,\\s*\"type\"\\s*:\\s*\"String\"|type\"\\s*:\\s*\"String\"\\s*,\\s*\"value\"\\s*:\\s*\"([^\"]*)\")\\}"));
            FRegexMatcher Matcher(Pattern, DecodedJson);

            int32 Index = 1;
            while (Matcher.FindNext())
            {
                FString Value = Matcher.GetCaptureGroup(1);
                if (Value.IsEmpty())
                {
                    Value = Matcher.GetCaptureGroup(2);
                }

                FStageSlotItem Item;
                Item.SlotIndex = Index++;
                Item.ItemId = Value;
                Results.Add(Item);
            }

            if (Options.bVerbose)
            {
                UE_LOG(LogTemp, Log, TEXT("[BlsquiSDK] Resolved %d equipped stage slot(s)."), Results.Num());
            }

            OnResolved.ExecuteIfBound(Results, true);
        });

    HttpRequest->ProcessRequest();
}
