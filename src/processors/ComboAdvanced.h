#pragma once

#include "../hardware/Scancodes.h"
#include "../pipeline/IProcessor.h"

#include <cstdint>
#include <cstring>
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
    void addCombo(const char* modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = getModifierBit(modName);
        addCombo(modBit, triggerKey, outputKey, true); // Layers match physical keys
    }

    void addComboWithShift(const char* modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = getModifierBit(modName);
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

    const char* name() const noexcept override { return "ComboAdvanced"; }

    size_t comboCount() const noexcept { return combos_.size(); }

  private:
    std::vector<ComboMapping> combos_;

    // Modifier bits (must match ModifierTracker)
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
