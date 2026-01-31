#pragma once

#include "../hardware/Scancodes.h"
#include "../pipeline/IProcessor.h"

#include <cstring>
#include <vector>

namespace keyflow {

/**
 * @brief Blocks layer trigger modifier keys from reaching the OS
 *
 * When a key like LAlt or RAlt is used as a layer trigger,
 * we want to track it as a modifier (for the layer system)
 * but NOT send it to Windows.
 *
 * This processor must come AFTER ModifierTracker so the modifier
 * state is captured, but blocks the trigger keys themselves.
 */
class LayerTriggerBlocker : public IProcessor {
  public:
    /**
     * @brief Add a trigger key to block (after remapping)
     */
    void addTrigger(uint16_t scancode) { triggers_.push_back(scancode); }

    /**
     * @brief Add trigger by name (e.g., "LALT", "RALT")
     */
    void addTrigger(const char* name) {
        uint16_t scancode = getScancodeFromName(name);
        if (scancode != 0) {
            triggers_.push_back(scancode);
        }
    }

    bool process(Context& ctx) override {
        // Only block if the key hasn't been processed yet
        if (ctx.action != Action::Forward) {
            return true; // Already handled by another processor
        }

        // Check if this key (after remapping) is a layer trigger
        for (uint16_t trigger : triggers_) {
            if (ctx.outputScancode == trigger) {
                // Consume this key - don't let it reach Windows
                ctx.action = Action::Consume;
                return true;
            }
        }

        return true; // Continue pipeline
    }

    const char* name() const noexcept override { return "LayerTriggerBlocker"; }

    size_t triggerCount() const noexcept { return triggers_.size(); }

  private:
    std::vector<uint16_t> triggers_;

    uint16_t getScancodeFromName(const char* name) const noexcept {
        if (strcmp(name, "LALT") == 0)
            return SC_LALT;
        if (strcmp(name, "RALT") == 0)
            return SC_RALT;
        if (strcmp(name, "LCTRL") == 0)
            return SC_LCTRL;
        if (strcmp(name, "RCTRL") == 0)
            return SC_RCTRL;
        if (strcmp(name, "LSHIFT") == 0)
            return SC_LSHIFT;
        if (strcmp(name, "RSHIFT") == 0)
            return SC_RSHIFT;
        if (strcmp(name, "LWIN") == 0)
            return SC_LWIN;
        if (strcmp(name, "RWIN") == 0)
            return SC_RWIN;
        if (strcmp(name, "PRINT") == 0)
            return SC_PRINTSCREEN;
        return 0;
    }
};

} // namespace keyflow
