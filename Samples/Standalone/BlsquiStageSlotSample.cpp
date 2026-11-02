#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <functional>

struct StageSlotItem
{
    int slotIndex;
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

    static std::string BuildCadenceScript(bool isTestnet)
    {
        std::string contract = isTestnet ? SESAME_CONTRACT_TESTNET : SESAME_CONTRACT_MAINNET;
        std::ostringstream script;
        script << "import SesamePass from " << contract << "\n\n"
               << "access(all) fun main(publisher: Address, user: Address, appId: String): [String]? {\n"
               << "    return SesamePass.getUserItemSlots(publisher: publisher, user: user, appId: appId)\n"
               << "}";
        return script.str();
    }

    /**
     * Parses the JSON-CDC array returned by the Flow Access Node REST API.
     */
    static std::vector<StageSlotItem> ParseCadenceResponse(const std::string& jsonResponse)
    {
        std::vector<StageSlotItem> items;
        
        // Regex matches Cadence JSON-CDC string items: {"type":"String","value":"..."}
        std::regex slotRegex(R"(\{\"(?:value\"\s*:\s*\"([^\"]*)\"\s*,\s*\"type\"\s*:\s*\"String\"|type\"\s*:\s*\"String\"\s*,\s*\"value\"\s*:\s*\"([^\"]*)\")\})");
        
        auto words_begin = std::sregex_iterator(jsonResponse.begin(), jsonResponse.end(), slotRegex);
        auto words_end = std::sregex_iterator();

        int index = 1;
        for (std::sregex_iterator i = words_begin; i != words_end; ++i)
        {
            std::smatch match = *i;
            std::string val = match[1].matched ? match[1].str() : match[2].str();
            items.push_back({ index++, val });
        }

        return items;
    }

    /**
     * Formats retrieved slots into a display string for game UI labels.
     */
    static std::string FormatScoutDialogText(const std::string& targetAddress, const std::vector<StageSlotItem>& slots, int maxSlotsToShow = 3)
    {
        std::ostringstream ss;
        ss << "Target: " << targetAddress << "\n";
        ss << "Equipped Item Slots:\n\n";

        bool hasEquipped = false;
        int count = 0;

        for (const auto& slot : slots)
        {
            if (count >= maxSlotsToShow)
            {
                break;
            }

            if (!slot.itemId.empty())
            {
                hasEquipped = true;
                ss << "• Slot " << slot.slotIndex << ": " << slot.itemId << "\n";
            }
            else
            {
                ss << "• Slot " << slot.slotIndex << ": (Empty)\n";
            }
            count++;
        }

        if (!hasEquipped)
        {
            ss << "No items equipped in stage slots.\n";
        }

        return ss.str();
    }
};

int main()
{
    const std::string target = "0x2f13c512df0b0ad7";
    
    // Example raw response simulated from Flow Access Node
    std::string sampleFlowResponse = 
        R"({"type":"Array","value":[{"type":"String","value":"shd_valkyrie_aegis_b"},{"type":"String","value":"shd_valkyrie_aegis_a"},{"type":"String","value":"wpn_dragon_piercer_b"}]})";

    std::vector<StageSlotItem> slots = BlsquiStageSlotSample::ParseCadenceResponse(sampleFlowResponse);
    std::string uiText = BlsquiStageSlotSample::FormatScoutDialogText(target, slots, 3);

    std::cout << "[Scout Opponent Output]\n" << uiText << std::endl;
    return 0;
}
