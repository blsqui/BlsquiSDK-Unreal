#pragma once

#include <string>
#include <vector>

struct StageSlotItem
{
    int slotIndex = 0;
    std::string itemId;
};

struct StageSlotOptions
{
    std::string targetAddress;
    std::string publisherAddress;
    std::string appId = "";
    bool isTestnet = true;
    bool verbose = false;
};

/**
 * Standard C++ reference for building Flow Cadence script payloads
 * and parsing JSON-CDC responses across any C++ engine.
 */
class BlsquiStageSlotSample
{
public:
    static const inline std::string SESAME_CONTRACT_TESTNET = "0x975b04756864e9ea";
    static const inline std::string SESAME_CONTRACT_MAINNET = "0xb576a3926d239682";

    /**
     * Constructs the Flow Cadence script query for fetching equipped slots.
     */
    static std::string BuildCadenceScript(bool isTestnet);

    /**
     * Parses the JSON-CDC array returned by the Flow Access Node REST API.
     */
    static std::vector<StageSlotItem> ParseCadenceResponse(const std::string& jsonResponse);

    /**
     * Formats retrieved slots into a display string for game UI labels.
     */
    static std::string FormatScoutDialogText(const std::string& targetAddress, const std::vector<StageSlotItem>& slots, int maxSlotsToShow = 3);
};
