#include "ShopButtonHandlerStandalone.h"
#include <sstream>
#include <iostream>

ShopButtonHandlerStandalone::ShopButtonHandlerStandalone(
    RequestTxCallback InTxRequester,
    ResultCallback InModalCallback,
    DialogCallback InDialogCallback)
    : RequestTxFn(std::move(InTxRequester))
    , ModalCallbackFn(std::move(InModalCallback))
    , DialogCallbackFn(std::move(InDialogCallback))
{
}

void ShopButtonHandlerStandalone::ExecuteShopPurchase()
{
    if (!RequestTxFn)
    {
        std::cerr << "[ShopStandalone] Error: RequestTxCallback is not set.\n";
        return;
    }

    if (DialogCallbackFn)
    {
        DialogCallbackFn("Blsqui In-Game Shop", "Authorizing transaction in Web3 wallet...", true);
    }

    std::unordered_map<std::string, std::string> Args;
    if (!MerchantAddress.empty())
    {
        Args["to"] = MerchantAddress;
    }

    std::cout << "[ShopStandalone] Requesting transaction with FLIX: " << FlixId << "\n";
    RequestTxFn(FlixId, Args, bIsTestnet);
}

void ShopButtonHandlerStandalone::HandleTransactionResolved(const StandaloneShopTxResult& Result)
{
    std::string Title;
    std::string Status;
    std::string Details;
    bool bSuccess = false;

    FormatTxResultForModal(Result, Title, Status, Details, bSuccess);

    if (ModalCallbackFn)
    {
        ModalCallbackFn(Title, Status, Details, bSuccess);
    }
}

void ShopButtonHandlerStandalone::OnCloseShopDialog()
{
    if (DialogCallbackFn)
    {
        DialogCallbackFn("", "", false);
    }
}

void ShopButtonHandlerStandalone::FormatTxResultForModal(
    const StandaloneShopTxResult& Result,
    std::string& OutTitle,
    std::string& OutStatus,
    std::string& OutDetails,
    bool& bOutSuccess)
{
    OutTitle = "BLSQUI SHOP TRANSACTION";
    OutStatus = Result.Status;
    bOutSuccess = (Result.Status == "SEALED");

    std::ostringstream ss;
    if (bOutSuccess)
    {
        std::string TokenName = Result.Token.empty() ? "FlowToken" : Result.Token;
        std::string Fee = Result.TxFee.empty() ? "0.00001000" : Result.TxFee;

        ss << "• Transaction ID:\n  " << Result.TxId << "\n\n"
           << "• Payer: " << Result.Payer << "\n"
           << "• Merchant: " << Result.To << "\n"
           << "• Amount: " << Result.Amount << " " << TokenName << "\n"
           << "• Network Fee: " << Fee << " FlowToken\n"
           << "• Nonce: " << Result.Nonce;
    }
    else
    {
        std::string Err = Result.Error.empty() ? "Transaction was rejected or aborted." : Result.Error;
        std::string ErrMsg = Result.ErrorMessage.empty() ? "No runtime error returned." : Result.ErrorMessage;

        ss << "• Error: " << Err << "\n\n"
           << "• Details:\n" << ErrMsg;
    }

    OutDetails = ss.str();
}
