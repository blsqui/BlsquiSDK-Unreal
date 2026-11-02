#pragma once

#include <string>
#include <unordered_map>
#include <functional>

struct StandaloneShopTxResult
{
    std::string Status;
    std::string TxId;
    std::string Payer;
    std::string To;
    std::string Amount;
    std::string Token;
    std::string TxFee;
    std::string Nonce;
    std::string Error;
    std::string ErrorMessage;
};

class ShopButtonHandlerStandalone
{
public:
    using RequestTxCallback = std::function<void(const std::string& FlixId, const std::unordered_map<std::string, std::string>& Args, bool bIsTestnet)>;
    using ResultCallback    = std::function<void(const std::string& Title, const std::string& Status, const std::string& Details, bool bSuccess)>;
    using DialogCallback    = std::function<void(const std::string& Title, const std::string& Status, bool bVisible)>;

    ShopButtonHandlerStandalone(RequestTxCallback InTxRequester, ResultCallback InModalCallback, DialogCallback InDialogCallback = nullptr);

    /** Triggers the shop purchase transaction */
    void ExecuteShopPurchase();

    /** Handles the raw transaction result from the SDK and forwards formatted strings */
    void HandleTransactionResolved(const StandaloneShopTxResult& Result);

    /** Closes or hides the shop dialog */
    void OnCloseShopDialog();

    /** Formats a transaction result into modal-ready display strings */
    static void FormatTxResultForModal(
        const StandaloneShopTxResult& Result,
        std::string& OutTitle,
        std::string& OutStatus,
        std::string& OutDetails,
        bool& bOutSuccess);

    // Configuration
    std::string FlixId = "351d9d6f23ebe5ddbe9bf5b4f937a352c5caaa6a4439b5e464f3b16c1d671472";
    std::string MerchantAddress = "0xa090f900023d6d34";
    bool bIsTestnet = true;

private:
    RequestTxCallback RequestTxFn;
    ResultCallback    ModalCallbackFn;
    DialogCallback    DialogCallbackFn;
};
