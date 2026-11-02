#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>

struct StandaloneItemUsePayload
{
    std::string FlixId;
    std::string ItemId;
    std::string Player;
    int32_t SerialNumber = 0;
    bool bIsTestnet = true;
};

struct StandaloneTxResult
{
    std::string Status;
    std::string TxId;
    std::string Payer;
    std::string Error;
    std::string ErrorMessage;
};

class UseItemButtonHandlerStandalone
{
public:
    using ResultCallback = std::function<void(const std::string& Title, const std::string& Status, const std::string& Details, bool bSuccess)>;
    using SendDataCallback = std::function<void(const std::string& JsonPayload)>;
    using RequestTxCallback = std::function<void(const std::string& FlixId, const std::unordered_map<std::string, std::string>& Args, bool bIsTestnet)>;

    UseItemButtonHandlerStandalone(SendDataCallback InSender, RequestTxCallback InTxRequester, ResultCallback InModalCallback);

    /** Called when the player clicks USE on an equipped item */
    void OnUseItemClicked(const std::string& ItemId, const std::string& PlayerAddress);

    /** Called when WebRTC receives ITEM_USE_REQUESTED from dashboard */
    void OnItemUseRequestedReceived(const StandaloneItemUsePayload& Payload);

    /** Called when SDK transaction resolves (SEALED/FAILED) */
    void OnTransactionResolved(const StandaloneTxResult& Result);

private:
    SendDataCallback SendDataFn;
    RequestTxCallback RequestTxFn;
    ResultCallback ModalCallbackFn;
};
