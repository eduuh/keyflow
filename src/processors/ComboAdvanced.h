#pragma once

#include "../DebugLog.h"
#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

#include <cstdint>
#include <vector>

namespace keyflow {

/**
 * @brief Advanced combo processor with key sequence output
 *
 * Can output single keys OR sequences (e.g., Shift+1 for !).
 *
 * Storage is two flat vectors (physical-key combos and remapped-key combos),
 * scanned linearly per event. Typical configs have <50 combos total; a
 * contiguous scan is cache-friendlier and faster than the previous
 * unordered_map<scancode, vector<...>> bucketing.
 */
class ComboAdvanced : public IProcessor {
  public:
    struct KeyAction {
        uint16_t scancode;
        bool withShift;

        KeyAction(uint16_t sc, bool shift = false) : scancode(sc), withShift(shift) {}
    };

    struct ComboMapping {
        uint32_t requiredModifiers;    // Modifiers that must be held
        uint32_t blockedModifiers;     // Modifiers that must NOT be held
        uint16_t triggerKey;           // Key that triggers
        std::vector<KeyAction> output; // Output sequence

        ComboMapping(uint32_t reqMods, uint32_t blockMods, uint16_t trigger)
            : requiredModifiers(reqMods), blockedModifiers(blockMods), triggerKey(trigger) {}
    };

    void addCombo(uint32_t modifiers, uint16_t triggerKey, uint16_t outputKey,
                  bool matchPhysical = false) {
        ComboMapping combo(modifiers, 0, triggerKey);
        combo.output.emplace_back(outputKey, false);
        bucket(matchPhysical).push_back(std::move(combo));
    }

    void addComboWithShift(uint32_t modifiers, uint16_t triggerKey, uint16_t outputKey,
                           bool matchPhysical = false) {
        ComboMapping combo(modifiers, 0, triggerKey);
        combo.output.emplace_back(outputKey, true);
        bucket(matchPhysical).push_back(std::move(combo));
    }

    // NoModCombos match the physical key to avoid double-transformation when a
    // remap targets a key with a noModCombo defined.
    void addNoModCombo(uint16_t triggerKey, uint16_t outputKey, bool withShift = false) {
        ComboMapping combo(0, 0xFFFFFFFF, triggerKey);
        combo.output.emplace_back(outputKey, withShift);
        physicalCombos_.push_back(std::move(combo));
    }

    void addCombo(std::string_view modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = static_cast<uint32_t>(modifierNameToBit(modName));
        addCombo(modBit, triggerKey, outputKey, true);
    }

    void addComboWithShift(std::string_view modName, uint16_t triggerKey, uint16_t outputKey) {
        uint32_t modBit = static_cast<uint32_t>(modifierNameToBit(modName));
        addComboWithShift(modBit, triggerKey, outputKey, true);
    }

    bool process(Context& ctx) override {
        if (ctx.isDown) {
            VERBOSE_LOG("[Combo] scancode=0x" << std::hex << ctx.scancode << " output=0x"
                                              << ctx.outputScancode << " mods=0x" << ctx.modifiers
                                              << std::dec << "\n");
        }
        if (matchAndApply(ctx, physicalCombos_, ctx.scancode)) {
            if (ctx.isDown) {
                VERBOSE_LOG("[Combo] MATCH (physical) -> output=0x"
                            << std::hex << ctx.outputScancode << std::dec
                            << " injectShift=" << ctx.injectShift << "\n");
            }
            return true;
        }
        if (matchAndApply(ctx, remappedCombos_, ctx.outputScancode)) {
            if (ctx.isDown) {
                VERBOSE_LOG("[Combo] MATCH (remapped) -> output=0x"
                            << std::hex << ctx.outputScancode << std::dec
                            << " injectShift=" << ctx.injectShift << "\n");
            }
        }
        return true;
    }

    [[nodiscard]] const char* name() const noexcept override { return "ComboAdvanced"; }

    [[nodiscard]] size_t comboCount() const noexcept {
        return physicalCombos_.size() + remappedCombos_.size();
    }

  private:
    std::vector<ComboMapping> physicalCombos_;
    std::vector<ComboMapping> remappedCombos_;

    std::vector<ComboMapping>& bucket(bool matchPhysical) noexcept {
        return matchPhysical ? physicalCombos_ : remappedCombos_;
    }

    static bool matchesCombo(const Context& ctx, const ComboMapping& combo) noexcept {
        if ((ctx.modifiers & combo.requiredModifiers) != combo.requiredModifiers) {
            return false;
        }
        if (combo.blockedModifiers != 0 && (ctx.modifiers & combo.blockedModifiers) != 0) {
            return false;
        }
        return true;
    }

    static void applyCombo(Context& ctx, const ComboMapping& combo) noexcept {
        if (combo.output.empty()) {
            return;
        }
        const auto& action = combo.output[0];
        ctx.outputScancode = action.scancode;
        ctx.action = Action::Replace;
        ctx.injectShift = action.withShift;
        ctx.cleanupInjectedShift = action.withShift;
    }

    static bool matchAndApply(Context& ctx, const std::vector<ComboMapping>& combos,
                              uint16_t key) noexcept {
        for (const auto& combo : combos) {
            if (combo.triggerKey == key && matchesCombo(ctx, combo)) {
                applyCombo(ctx, combo);
                return true;
            }
        }
        return false;
    }
};

} // namespace keyflow
