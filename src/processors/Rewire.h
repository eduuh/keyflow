#pragma once

#include "../pipeline/IProcessor.h"

#include <array>

namespace keyflow {

/**
 * @brief Simple 1:1 key remapping processor
 *
 * Maps scancodes to other scancodes using a lookup table.
 *
 * Example: CapsLock (0x3A) → LeftCtrl (0x1D)
 *
 * Target: ~60 lines
 */
class Rewire : public IProcessor {
  public:
    Rewire() {
        // Initialize to identity mapping (no changes)
        for (size_t i = 0; i < map_.size(); i++) {
            map_[i] = 0xFFFF; // 0xFFFF = no mapping
        }
    }

    /**
     * @brief Set a mapping from one scancode to another
     * @param from Source scancode
     * @param to Destination scancode
     */
    void setMapping(uint16_t from, uint16_t to) noexcept {
        if (from < map_.size()) {
            map_[from] = to;
        }
    }

    /**
     * @brief Clear a mapping (restore to no-op)
     */
    void clearMapping(uint16_t scancode) noexcept {
        if (scancode < map_.size()) {
            map_[scancode] = 0xFFFF;
        }
    }

    /**
     * @brief Process keystroke - apply remapping if defined
     */
    bool process(Context& ctx) override {
        if (ctx.scancode >= map_.size()) {
            return true; // Out of range, continue
        }

        uint16_t mapped = map_[ctx.scancode];

        if (mapped != 0xFFFF) {
            // Mapping defined - apply it
            ctx.outputScancode = mapped;
            ctx.action = Action::Replace;
        }

        return true; // Continue to next processor
    }

    const char* name() const noexcept override { return "Rewire"; }

  private:
    // Simple lookup table: scancode → mapped scancode
    // 0xFFFF = no mapping defined
    std::array<uint16_t, 512> map_;
};

} // namespace keyflow
