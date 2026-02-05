#pragma once

#include "../hardware/Scancodes.h"
#include "../pipeline/Context.h"
#include "../pipeline/IProcessor.h"

namespace keyflow {

/**
 * @brief Blocks CapsLock key from reaching the OS
 *
 * Prevents accidental caps lock toggle by consuming any CapsLock key
 * events that make it through the pipeline. This ensures the caps lock
 * LED never toggles, even if CapsLock is pressed.
 *
 * Pipeline position: Should run late (after Rewire, before StrictModeFilter)
 *
 * Use case: When CapsLock is remapped to another key (e.g., Shift or Ctrl),
 * you may still want to ensure the actual CapsLock toggle never happens.
 */
class CapsLockBlocker : public IProcessor {
  public:
    bool process(Context& ctx) override {
        // Block any CapsLock key from reaching the OS
        // Check both input scancode (before remapping) and output scancode (after remapping)
        if (ctx.outputScancode == SC_CAPSLOCK) {
            // CapsLock made it through - consume it to prevent toggle
            ctx.action = Action::Consume;
            return false; // Stop pipeline
        }

        return true; // Continue pipeline
    }

    [[nodiscard]] const char* name() const noexcept override { return "CapsLockBlocker"; }
};

} // namespace keyflow
