#pragma once

#include "../hardware/Scancodes.h"
#include "../pipeline/IProcessor.h"

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
        bool isMod = isModifierKey(ctx.outputScancode);

        if (isMod) {
            // Update modifier state
            if (ctx.isDown) {
                activeModifiers_ |= getModifierBit(ctx.outputScancode);
            } else {
                activeModifiers_ &= ~getModifierBit(ctx.outputScancode);
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
    bool isModifierActive(uint32_t modBit) const noexcept {
        return (activeModifiers_ & modBit) != 0;
    }

  private:
    uint32_t activeModifiers_ = 0; // Bitmask of active modifiers

    // Modifier bit positions
    enum ModBits : uint32_t {
        MOD_LSHIFT = 1 << 0,
        MOD_RSHIFT = 1 << 1,
        MOD_LCTRL = 1 << 2,
        MOD_RCTRL = 1 << 3,
        MOD_LALT = 1 << 4,
        MOD_RALT = 1 << 5,
        MOD_LWIN = 1 << 6,
        MOD_RWIN = 1 << 7,
        MOD_PRINT = 1 << 8, // PrintScreen (used as MOD11 in EDUUH config)
    };

    bool isModifierKey(uint16_t scancode) const noexcept {
        return scancode == SC_LSHIFT || scancode == SC_RSHIFT || scancode == SC_LCTRL ||
               scancode == SC_RCTRL || scancode == SC_LALT || scancode == SC_RALT ||
               scancode == SC_LWIN || scancode == SC_RWIN || scancode == SC_PRINTSCREEN;
    }

    uint32_t getModifierBit(uint16_t scancode) const noexcept {
        switch (scancode) {
            case SC_LSHIFT:
                return MOD_LSHIFT;
            case SC_RSHIFT:
                return MOD_RSHIFT;
            case SC_LCTRL:
                return MOD_LCTRL;
            case SC_RCTRL:
                return MOD_RCTRL;
            case SC_LALT:
                return MOD_LALT;
            case SC_RALT:
                return MOD_RALT;
            case SC_LWIN:
                return MOD_LWIN;
            case SC_RWIN:
                return MOD_RWIN;
            case SC_PRINTSCREEN:
                return MOD_PRINT;
            default:
                return 0;
        }
    }
};

} // namespace keyflow
