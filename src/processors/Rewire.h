#pragma once

#include "../pipeline/IProcessor.h"

#include <array>
#include <cstdint>

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
    // Sentinel value indicating no mapping is defined for this scancode
    static constexpr uint16_t NO_MAPPING = 0xFFFF;

    Rewire() {
        // Initialize to identity mapping (no changes)
        map_.fill(NO_MAPPING);
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
            map_[scancode] = NO_MAPPING;
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

        if (mapped != NO_MAPPING) {
            // Mapping defined - apply it
            ctx.outputScancode = mapped;
            ctx.action = Action::Replace;
        }

        return true; // Continue to next processor
    }

    [[nodiscard]] const char* name() const noexcept override { return "Rewire"; }

  private:
    // Simple lookup table: scancode → mapped scancode
    // NO_MAPPING = no mapping defined
    // Size 65536 to support all possible uint16_t scancodes including extended keys (0xE000+)
    std::array<uint16_t, 65536> map_;
};

} // namespace keyflow
