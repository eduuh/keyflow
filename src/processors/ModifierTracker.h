#pragma once

#include "../DebugLog.h"
#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

#include <string>
#include <unordered_map>

namespace keyflow {

/**
 * @brief Tracks modifier key state including custom modifiers
 *
 * Keeps track of which modifiers (Shift, Ctrl, Alt, Win) are currently
 * held down. Also supports custom modifiers where any key can act as a modifier.
 *
 * In EDUUH_MOD config, these become MOD layers:
 * - MOD11: Activated by RALT (numpad layer)
 * - MOD12: Activated by LALT (arrows/symbols layer)
 * - MOD13: Activated by LCTRL or LWIN (tab/brackets layer)
 *
 * Custom modifiers example:
 * - Register Space as "SPACE_MOD" → Can use SPACE_MOD as layer trigger
 */
class ModifierTracker : public IProcessor {
  public:
    bool process(Context& ctx) override {
        // Check standard modifier keys (use outputScancode after Rewire)
        if (isModifierKey(ctx.outputScancode)) {
            ModifierBit modBit = getModifierBit(ctx.outputScancode);

            VERBOSE_LOG("[ModifierTracker] Standard modifier key detected: scancode=0x"
                        << std::hex << ctx.outputScancode << std::dec << " modBit="
                        << static_cast<int>(modBit) << " isDown=" << ctx.isDown << "\n");

            // Update modifier state
            if (ctx.isDown) {
                activeModifiers_ |= static_cast<uint32_t>(modBit);
                VERBOSE_LOG("[ModifierTracker] Modifier pressed, activeModifiers now: 0x"
                            << std::hex << activeModifiers_ << std::dec << "\n");
            } else {
                activeModifiers_ &= ~static_cast<uint32_t>(modBit);
                VERBOSE_LOG("[ModifierTracker] Modifier released, activeModifiers now: 0x"
                            << std::hex << activeModifiers_ << std::dec << "\n");
            }
        }

        // Check custom modifier keys (use physical scancode before remapping)
        auto customIt = customModifiers_.find(ctx.scancode);
        if (customIt != customModifiers_.end()) {
            ModifierBit modBit = customIt->second;

            VERBOSE_LOG("[ModifierTracker] Custom modifier key detected: scancode=0x"
                        << std::hex << ctx.scancode << std::dec << " modBit="
                        << static_cast<int>(modBit) << " isDown=" << ctx.isDown << "\n");

            // Update modifier state
            if (ctx.isDown) {
                activeModifiers_ |= static_cast<uint32_t>(modBit);
                VERBOSE_LOG("[ModifierTracker] Custom modifier pressed, activeModifiers now: 0x"
                            << std::hex << activeModifiers_ << std::dec << "\n");
            } else {
                activeModifiers_ &= ~static_cast<uint32_t>(modBit);
                VERBOSE_LOG("[ModifierTracker] Custom modifier released, activeModifiers now: 0x"
                            << std::hex << activeModifiers_ << std::dec << "\n");
            }
        }

        // Store modifier state in context for other processors
        ctx.modifiers = activeModifiers_;

        return true; // Continue pipeline
    }

    [[nodiscard]] const char* name() const noexcept override { return "ModifierTracker"; }

    /**
     * @brief Get current modifier state
     */
    [[nodiscard]] uint32_t getModifiers() const noexcept { return activeModifiers_; }

    /**
     * @brief Check if specific modifier is active
     */
    [[nodiscard]] bool isModifierActive(ModifierBit modBit) const noexcept {
        return (activeModifiers_ & modBit) != 0;
    }

    /**
     * @brief Register a custom modifier key
     * @param scancode Physical scancode that acts as modifier
     * @param modifierName Modifier name (e.g., "SPACE_MOD")
     * @param modifierBit The custom modifier bit to use
     */
    void registerCustomModifier(uint16_t scancode, const std::string& modifierName,
                                ModifierBit modifierBit) {
        customModifiers_[scancode] = modifierBit;
        customModifierNames_[modifierName] = modifierBit;
    }

    /**
     * @brief Resolve custom modifier name to bit
     * @return ModifierBit or ModifierBit::None if not found
     */
    [[nodiscard]] ModifierBit resolveCustomModifier(const std::string& modifierName) const {
        auto it = customModifierNames_.find(modifierName);
        if (it != customModifierNames_.end()) {
            return it->second;
        }
        return ModifierBit::None;
    }

    /**
     * @brief Check if a modifier name is a registered custom modifier
     */
    [[nodiscard]] bool isCustomModifier(const std::string& modifierName) const {
        return customModifierNames_.find(modifierName) != customModifierNames_.end();
    }

    /**
     * @brief Set injected shift state
     */
    void setInjectedShift(bool state) noexcept { injectedShift_ = state; }

    /**
     * @brief Check if shift was injected by combo processor
     */
    [[nodiscard]] bool hasInjectedShift() const noexcept { return injectedShift_; }

    /**
     * @brief Clear all injected modifier state
     */
    void clearInjectedModifiers() noexcept { injectedShift_ = false; }

  private:
    uint32_t activeModifiers_ = 0;                              // Bitmask of active modifiers
    std::unordered_map<uint16_t, ModifierBit> customModifiers_; // scancode → custom modifier bit
    std::unordered_map<std::string, ModifierBit> customModifierNames_; // name → custom modifier bit
    bool injectedShift_ = false;                                       // Track if we injected Shift
};

} // namespace keyflow
