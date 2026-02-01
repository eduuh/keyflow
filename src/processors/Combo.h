#pragma once

#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

#include <cstdint>
#include <vector>

namespace keyflow {

/**
 * @brief Matches modifier+key combinations
 *
 * When a modifier is held (e.g., LALT for MOD12) and a key is pressed,
 * this processor can output a different key or sequence of keys.
 *
 * Example: LALT+K → Left Arrow
 */
class Combo : public IProcessor {
  public:
    /**
     * @brief Define a simple combo: modifier + key → output key
     */
    struct ComboMapping {
        uint32_t requiredModifiers; // Modifier bitmask (must match exactly)
        uint16_t triggerKey;        // Key that triggers the combo
        uint16_t outputKey;         // Key to output instead

        ComboMapping(uint32_t mods, uint16_t trigger, uint16_t output)
            : requiredModifiers(mods), triggerKey(trigger), outputKey(output) {}
    };

    /**
     * @brief Add a combo mapping
     */
    void addCombo(uint32_t modifiers, uint16_t triggerKey, uint16_t outputKey) {
        combos_.emplace_back(modifiers, triggerKey, outputKey);
    }

    /**
     * @brief Add a combo using modifier names (helper)
     */
    void addCombo(std::string_view modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = static_cast<uint32_t>(modifierNameToBit(modName));
        if (modBit != 0) {
            addCombo(modBit, triggerKey, outputKey);
        }
    }

    bool process(Context& ctx) override {
        // Only process key down events for combos
        if (!ctx.isDown) {
            return true; // Let key up pass through
        }

        // Check if any combo matches
        for (const auto& combo : combos_) {
            if (matchesCombo(ctx, combo)) {
                // Combo matched! Replace the key
                ctx.outputScancode = combo.outputKey;
                ctx.action = Action::Replace;
                return true; // Stop processing (combo handled)
            }
        }

        return true; // Continue pipeline
    }

    [[nodiscard]] const char* name() const noexcept override { return "Combo"; }

    /**
     * @brief Get number of combos registered
     */
    [[nodiscard]] size_t comboCount() const noexcept { return combos_.size(); }

  private:
    std::vector<ComboMapping> combos_;

    bool matchesCombo(const Context& ctx, const ComboMapping& combo) const noexcept {
        // Check if the trigger key matches (use outputScancode after Rewire)
        if (ctx.outputScancode != combo.triggerKey) {
            return false;
        }

        // Check if modifiers match exactly
        // Note: We only check the modifiers we care about
        uint32_t relevantMods = ctx.modifiers & combo.requiredModifiers;
        return relevantMods == combo.requiredModifiers;
    }
};

} // namespace keyflow
