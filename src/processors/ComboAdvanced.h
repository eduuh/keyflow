#pragma once

#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace keyflow {

/**
 * @brief Advanced combo processor with key sequence output
 *
 * Can output single keys OR sequences (e.g., Shift+1 for !)
 */
class ComboAdvanced : public IProcessor {
  public:
    /**
     * @brief Key event in a sequence
     */
    struct KeyAction {
        uint16_t scancode;
        bool withShift;

        KeyAction(uint16_t sc, bool shift = false) : scancode(sc), withShift(shift) {}
    };

    /**
     * @brief Combo mapping with sequence output
     */
    struct ComboMapping {
        uint32_t requiredModifiers;    // Modifiers that must be held
        uint32_t blockedModifiers;     // Modifiers that must NOT be held
        uint16_t triggerKey;           // Key that triggers
        std::vector<KeyAction> output; // Output sequence
        bool matchPhysicalKey;         // Match against physical key (true for layers)

        ComboMapping(uint32_t reqMods, uint32_t blockMods, uint16_t trigger,
                     bool matchPhysical = false)
            : requiredModifiers(reqMods), blockedModifiers(blockMods), triggerKey(trigger),
              matchPhysicalKey(matchPhysical) {}
    };

    /**
     * @brief Add a simple combo (single output key)
     * @param matchPhysical If true, match against physical key (before remapping)
     */
    void addCombo(uint32_t modifiers, uint16_t triggerKey, uint16_t outputKey,
                  bool matchPhysical = false) {
        ComboMapping combo(modifiers, 0, triggerKey, matchPhysical);
        combo.output.emplace_back(outputKey, false);

        // Add to appropriate hash map based on matchPhysical flag
        auto& targetMap = matchPhysical ? physicalKeyCombos_ : remappedKeyCombos_;
        targetMap[triggerKey].push_back(combo);
    }

    /**
     * @brief Add a combo with Shift+key output
     * @param matchPhysical If true, match against physical key (before remapping)
     */
    void addComboWithShift(uint32_t modifiers, uint16_t triggerKey, uint16_t outputKey,
                           bool matchPhysical = false) {
        ComboMapping combo(modifiers, 0, triggerKey, matchPhysical);
        combo.output.emplace_back(outputKey, true); // Output with Shift

        // Add to appropriate hash map based on matchPhysical flag
        auto& targetMap = matchPhysical ? physicalKeyCombos_ : remappedKeyCombos_;
        targetMap[triggerKey].push_back(combo);
    }

    /**
     * @brief Add combo with no modifiers required (for top-row remapping)
     */
    void addNoModCombo(uint16_t triggerKey, uint16_t outputKey, bool withShift = false) {
        ComboMapping combo(0, 0xFFFFFFFF, triggerKey, false); // Match remapped key
        combo.output.emplace_back(outputKey, withShift);

        // NoModCombos always match remapped key (matchPhysical = false)
        remappedKeyCombos_[triggerKey].push_back(combo);
    }

    /**
     * @brief Add combo using modifier names (helper for layers)
     */
    void addCombo(std::string_view modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = static_cast<uint32_t>(modifierNameToBit(modName));
        addCombo(modBit, triggerKey, outputKey, true); // Layers match physical keys
    }

    void addComboWithShift(std::string_view modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = static_cast<uint32_t>(modifierNameToBit(modName));
        addComboWithShift(modBit, triggerKey, outputKey, true); // Layers match physical keys
    }

    bool process(Context& ctx) override {
        // Check physical key combos first (layers)
        auto physicalIt = physicalKeyCombos_.find(ctx.scancode);
        if (physicalIt != physicalKeyCombos_.end()) {
            for (const auto& combo : physicalIt->second) {
                if (matchesCombo(ctx, combo)) {
                    applyCombo(ctx, combo);
                    return true; // Combo handled
                }
            }
        }

        // Then check remapped key combos (noModCombos)
        auto remappedIt = remappedKeyCombos_.find(ctx.outputScancode);
        if (remappedIt != remappedKeyCombos_.end()) {
            for (const auto& combo : remappedIt->second) {
                if (matchesCombo(ctx, combo)) {
                    applyCombo(ctx, combo);
                    return true; // Combo handled
                }
            }
        }

        return true; // Continue
    }

    [[nodiscard]] const char* name() const noexcept override { return "ComboAdvanced"; }

    [[nodiscard]] size_t comboCount() const noexcept {
        size_t count = 0;
        for (const auto& [key, combos] : physicalKeyCombos_) {
            count += combos.size();
        }
        for (const auto& [key, combos] : remappedKeyCombos_) {
            count += combos.size();
        }
        return count;
    }

  private:
    // Hash maps for O(1) trigger key lookup
    // physicalKeyCombos: indexed by physical scancode (for layers)
    // remappedKeyCombos: indexed by remapped scancode (for noModCombos)
    std::unordered_map<uint16_t, std::vector<ComboMapping>> physicalKeyCombos_;
    std::unordered_map<uint16_t, std::vector<ComboMapping>> remappedKeyCombos_;

    void applyCombo(Context& ctx, const ComboMapping& combo) const noexcept {
        // Output first key in sequence
        if (!combo.output.empty()) {
            const auto& action = combo.output[0];

            // Set output key
            ctx.outputScancode = action.scancode;
            ctx.action = Action::Replace;

            // Set shift injection flag if needed
            ctx.injectShift = action.withShift;
        }
    }

    bool matchesCombo(const Context& ctx, const ComboMapping& combo) const noexcept {
        // Note: Trigger key check is now handled by hash map lookup in process()
        // Only need to verify modifier requirements

        // Check required modifiers are held
        if ((ctx.modifiers & combo.requiredModifiers) != combo.requiredModifiers) {
            return false;
        }

        // Check blocked modifiers are NOT held
        if (combo.blockedModifiers != 0) {
            if ((ctx.modifiers & combo.blockedModifiers) != 0) {
                return false;
            }
        }

        return true;
    }
};

} // namespace keyflow
