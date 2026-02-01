#pragma once

#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

#include <cstdint>
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
        combos_.push_back(combo);
    }

    /**
     * @brief Add a combo with Shift+key output
     * @param matchPhysical If true, match against physical key (before remapping)
     */
    void addComboWithShift(uint32_t modifiers, uint16_t triggerKey, uint16_t outputKey,
                           bool matchPhysical = false) {
        ComboMapping combo(modifiers, 0, triggerKey, matchPhysical);
        combo.output.emplace_back(outputKey, true); // Output with Shift
        combos_.push_back(combo);
    }

    /**
     * @brief Add combo with no modifiers required (for top-row remapping)
     */
    void addNoModCombo(uint16_t triggerKey, uint16_t outputKey, bool withShift = false) {
        ComboMapping combo(0, 0xFFFFFFFF, triggerKey, false); // Match remapped key
        combo.output.emplace_back(outputKey, withShift);
        combos_.push_back(combo);
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
        // Check each combo in order (first match wins)
        for (const auto& combo : combos_) {
            if (matchesCombo(ctx, combo)) {
                // Output first key in sequence
                if (!combo.output.empty()) {
                    const auto& action = combo.output[0];

                    // Set output key
                    ctx.outputScancode = action.scancode;
                    ctx.action = Action::Replace;

                    // Set shift injection flag if needed
                    ctx.injectShift = action.withShift;
                }
                return true; // Combo handled
            }
        }

        return true; // Continue
    }

    [[nodiscard]] const char* name() const noexcept override { return "ComboAdvanced"; }

    [[nodiscard]] size_t comboCount() const noexcept { return combos_.size(); }

  private:
    std::vector<ComboMapping> combos_;

    bool matchesCombo(const Context& ctx, const ComboMapping& combo) const noexcept {
        // Check trigger key
        // - Layers match against physical key (before remapping)
        // - NoModCombos match against remapped key (after Rewire)
        uint16_t keyToMatch = combo.matchPhysicalKey ? ctx.scancode : ctx.outputScancode;
        if (keyToMatch != combo.triggerKey) {
            return false;
        }

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
