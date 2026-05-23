#pragma once

#include "../pipeline/IProcessor.h"

#include <array>
#include <cstdint>
#include <unordered_map>

namespace keyflow {

/**
 * @brief Simple 1:1 key remapping processor
 *
 * Maps scancodes to other scancodes via a two-tier lookup:
 *  - Standard scancodes (0x00-0xFF) use a flat 256-entry array (1 cache line
 *    pair, hot in L1 during typing).
 *  - Extended scancodes (0xE0__ / 0xE1__ — RCtrl, RAlt, arrows, etc.) live in
 *    a small unordered_map. These are rare in a typing hot loop but must still
 *    be remappable.
 *
 * Compared to a flat 65536-entry array (~128 KB) this is <1 KB at rest, and
 * standard-key lookup is the same instruction sequence: one bounds-implicit
 * array index. The integration test exercises both paths through src/config.json.
 */
class Rewire : public IProcessor {
  public:
    static constexpr uint16_t NO_MAPPING = 0xFFFF;
    static constexpr uint16_t STANDARD_MAX = 0x100;

    Rewire() noexcept { standardMap_.fill(NO_MAPPING); }

    void setMapping(uint16_t from, uint16_t to) noexcept {
        if (isStandard(from)) {
            standardMap_[from] = to;
        } else {
            extendedMap_[from] = to;
        }
    }

    void clearMapping(uint16_t scancode) noexcept {
        if (isStandard(scancode)) {
            standardMap_[scancode] = NO_MAPPING;
        } else {
            extendedMap_.erase(scancode);
        }
    }

    bool process(Context& ctx) override {
        uint16_t mapped = lookup(ctx.scancode);
        if (mapped != NO_MAPPING) {
            ctx.outputScancode = mapped;
            ctx.action = Action::Replace;
        }
        return true;
    }

    [[nodiscard]] const char* name() const noexcept override { return "Rewire"; }

  private:
    static constexpr bool isStandard(uint16_t scancode) noexcept { return scancode < STANDARD_MAX; }

    [[nodiscard]] uint16_t lookup(uint16_t scancode) const noexcept {
        if (isStandard(scancode)) {
            return standardMap_[scancode];
        }
        auto it = extendedMap_.find(scancode);
        return it != extendedMap_.end() ? it->second : NO_MAPPING;
    }

    std::array<uint16_t, STANDARD_MAX> standardMap_;
    std::unordered_map<uint16_t, uint16_t> extendedMap_;
};

} // namespace keyflow
