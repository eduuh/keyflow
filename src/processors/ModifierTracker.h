#pragma once

#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

namespace keyflow {

/**
 * @brief Tracks modifier key state
 *
 * Keeps track of which modifiers (Shift, Ctrl, Alt, Win) are currently
 * held down. Sets flags in the context for other processors to use.
 *
 * In EDUUH_MOD config, these become MOD layers:
 * - MOD11: Activated by RALT (numpad layer)
 * - MOD12: Activated by LALT (arrows/symbols layer)
 * - MOD13: Activated by LCTRL or LWIN (tab/brackets layer)
 */
class ModifierTracker : public IProcessor {
  public:
    bool process(Context& ctx) override {
        // Check if this is a modifier key (use outputScancode after Rewire)
        if (isModifierKey(ctx.outputScancode)) {
            ModifierBit modBit = getModifierBit(ctx.outputScancode);

            // Update modifier state
            if (ctx.isDown) {
                activeModifiers_ |= static_cast<uint32_t>(modBit);
            } else {
                activeModifiers_ &= ~static_cast<uint32_t>(modBit);
            }
        }

        // Store modifier state in context for other processors
        ctx.modifiers = activeModifiers_;

        return true; // Continue pipeline
    }

    const char* name() const noexcept override { return "ModifierTracker"; }

    /**
     * @brief Get current modifier state
     */
    uint32_t getModifiers() const noexcept { return activeModifiers_; }

    /**
     * @brief Check if specific modifier is active
     */
    bool isModifierActive(ModifierBit modBit) const noexcept {
        return (activeModifiers_ & modBit) != 0;
    }

  private:
    uint32_t activeModifiers_ = 0; // Bitmask of active modifiers
};

} // namespace keyflow
