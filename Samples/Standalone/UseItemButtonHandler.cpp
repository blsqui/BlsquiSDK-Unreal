#include "UseItemButtonHandler.h"
#include <sstream>

UseItemButtonHandlerStandalone::UseItemButtonHandlerStandalone(
    SendDataCallback InSender, 
    RequestTxCallback InTxRequester, 
    ResultCallback InModalCallback)
    : SendDataFn(std::move(InSender))
    , RequestTxFn(std::move(InTxRequester))
    , ModalCallbackFn(std::move(InModalCallback))
{
}

void UseItemButtonHandlerStandalone::OnUseItemClicked(const std::string& ItemId, const std::string& PlayerAddress)
{
    if (!SendDataFn) return;

    auto NowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::ostringstream Oss;
    Oss << "{\"type\":\"TRIGGER_ITEM_ACTION\",\"action\":\"use_equipped_item\","
        << "\"itemId\":\"" << ItemId << "\","
        << "\"player\":\"" << PlayerAddress << "\","
        << "\"timestamp\":" << NowMs << "}";

    SendDataFn(Oss.str());
}

void UseItemButtonHandlerStandalone::OnItemUseRequestedReceived(const StandaloneItemUsePayload& Payload)
{
    if (!RequestTxFn) return;

    std::unordered_map<std::string, std::string> Args;
    Args["itemId"] = Payload.ItemId;
    Args["serialNumber"] = std::to_string(Payload.SerialNumber);

    RequestTxFn(Payload.FlixId, Args, Payload.bIsTestnet);
}

void UseItemButtonHandlerStandalone::OnTransactionResolved(const StandaloneTxResult& Result)
{
    if (!ModalCallbackFn) return;

    std::string Title = "BLSQUI ITEM TRANSACTION";
    std::string Status = Result.Status;
    bool bSuccess = (Result.Status == "SEALED");

    std::ostringstream Details;
    if (bSuccess)
    {
        Details << "• Transaction ID:\n  " << Result.TxId << "\n\n"
                << "• Payer: " << Result.Payer << "\n"
                << "• Status: SEALED";
    }
    else
    {
        Details << "• Error: " << (Result.Error.empty() ? "Transaction aborted." : Result.Error) << "\n\n"
                << "• Details:\n" << (Result.ErrorMessage.empty() ? "No execution details." : Result.ErrorMessage);
    }

    ModalCallbackFn(Title, Status, Details.str(), bSuccess);
}
