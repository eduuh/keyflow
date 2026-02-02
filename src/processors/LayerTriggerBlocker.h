#pragma once

#include "../pipeline/IProcessor.h"
#include "../pipeline/Modifiers.h"

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
     * @brief Add a physical trigger key to block (for custom modifiers)
     * Checks ctx.scancode (before remapping)
     */
    void addPhysicalTrigger(uint16_t scancode) { physicalTriggers_.push_back(scancode); }

    /**
     * @brief Add a trigger key to block by scancode (for custom modifiers)
     * Alias for addPhysicalTrigger for backward compatibility
     */
    void addTriggerByScancode(uint16_t scancode) { addPhysicalTrigger(scancode); }

    /**
     * @brief Add a trigger key to block (after remapping) - deprecated, use addTriggerByScancode
     */
    void addTrigger(uint16_t scancode) { addTriggerByScancode(scancode); }

    /**
     * @brief Add trigger by name (e.g., "LALT", "RALT")
     * Checks ctx.outputScancode (after remapping) for standard layer triggers
     */
    void addTrigger(std::string_view name) {
        ModifierBit modBit = modifierNameToBit(name);
        if (modBit != ModifierBit::None) {
            uint16_t scancode = getScancodeFromModifier(modBit);
            if (scancode != 0) {
                outputTriggers_.push_back(scancode);
            }
        }
    }

    bool process(Context& ctx) override {
        // Only block if the key hasn't been processed yet
        if (ctx.action != Action::Forward) {
            return true; // Already handled by another processor
        }

        // Check physical triggers (custom modifiers) - check before remapping
        for (uint16_t trigger : physicalTriggers_) {
            if (ctx.scancode == trigger) {
                // Consume this key - don't let it reach Windows
                ctx.action = Action::Consume;
                return true;
            }
        }

        // Check output triggers (standard layers) - check after remapping
        for (uint16_t trigger : outputTriggers_) {
            if (ctx.outputScancode == trigger) {
                // Consume this key - don't let it reach Windows
                ctx.action = Action::Consume;
                return true;
            }
        }

        return true; // Continue pipeline
    }

    [[nodiscard]] const char* name() const noexcept override { return "LayerTriggerBlocker"; }

    [[nodiscard]] size_t triggerCount() const noexcept {
        return physicalTriggers_.size() + outputTriggers_.size();
    }

  private:
    std::vector<uint16_t> physicalTriggers_; // Custom modifiers (check ctx.scancode)
    std::vector<uint16_t> outputTriggers_;   // Standard layers (check ctx.outputScancode)

    constexpr uint16_t getScancodeFromModifier(ModifierBit modBit) const noexcept {
        switch (modBit) {
            case ModifierBit::LeftAlt:
                return SC_LALT;
            case ModifierBit::RightAlt:
                return SC_RALT;
            case ModifierBit::LeftCtrl:
                return SC_LCTRL;
            case ModifierBit::RightCtrl:
                return SC_RCTRL;
            case ModifierBit::LeftShift:
                return SC_LSHIFT;
            case ModifierBit::RightShift:
                return SC_RSHIFT;
            case ModifierBit::LeftWin:
                return SC_LWIN;
            case ModifierBit::RightWin:
                return SC_RWIN;
            case ModifierBit::PrintScreen:
                return SC_PRINTSCREEN;
            default:
                return 0;
        }
    }
};

} // namespace keyflow
