#pragma once

#include "../hardware/Scancodes.h"
#include "../pipeline/IProcessor.h"

#include <cstdint>
#include <cstring>
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
    void addCombo(const char* modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = getModifierBit(modName);
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

    const char* name() const noexcept override { return "Combo"; }

    /**
     * @brief Get number of combos registered
     */
    size_t comboCount() const noexcept { return combos_.size(); }

  private:
    std::vector<ComboMapping> combos_;

    // Modifier bit definitions (must match ModifierTracker)
    enum ModBits : uint32_t {
        MOD_LSHIFT = 1 << 0,
        MOD_RSHIFT = 1 << 1,
        MOD_LCTRL = 1 << 2,
        MOD_RCTRL = 1 << 3,
        MOD_LALT = 1 << 4,
        MOD_RALT = 1 << 5,
        MOD_LWIN = 1 << 6,
        MOD_RWIN = 1 << 7,
        MOD_PRINT = 1 << 8,
    };

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

    uint32_t getModifierBit(const char* modName) const noexcept {
        if (strcmp(modName, "LALT") == 0 || strcmp(modName, "MOD12") == 0)
            return MOD_LALT;
        if (strcmp(modName, "RALT") == 0 || strcmp(modName, "MOD11") == 0)
            return MOD_RALT;
        if (strcmp(modName, "LCTRL") == 0 || strcmp(modName, "MOD13") == 0)
            return MOD_LCTRL;
        if (strcmp(modName, "LWIN") == 0)
            return MOD_LWIN;
        if (strcmp(modName, "PRINT") == 0)
            return MOD_PRINT;
        if (strcmp(modName, "LSHIFT") == 0)
            return MOD_LSHIFT;
        if (strcmp(modName, "RSHIFT") == 0)
            return MOD_RSHIFT;
        if (strcmp(modName, "RCTRL") == 0)
            return MOD_RCTRL;
        return 0;
    }
};

} // namespace keyflow
