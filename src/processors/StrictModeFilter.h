#pragma once

#include "../pipeline/IProcessor.h"

#include <cstdint>
#include <unordered_set>

namespace keyflow {

/**
 * @brief Strict mode filter - blocks all unmapped keys
 *
 * When strictMode is enabled in config, only keys that are explicitly
 * mapped (in remapping, layers, or combos) will produce output.
 * All other keys are consumed/blocked.
 *
 * This is useful for full keyboard remappings where you want to ensure
 * no unmapped keys "leak through" and produce unexpected characters.
 *
 * Performance: O(1) hash set lookup per unmapped key
 */
class StrictModeFilter : public IProcessor {
  public:
    /**
     * @brief Add a key to the allowed list
     * @param scancode Scancode that should be allowed through
     */
    void addAllowedKey(uint16_t scancode) { allowedKeys_.insert(scancode); }

    /**
     * @brief Add multiple keys to the allowed list
     * @param scancodes Vector of scancodes to allow
     */
    void addAllowedKeys(const std::vector<uint16_t>& scancodes) {
        for (uint16_t sc : scancodes) {
            allowedKeys_.insert(sc);
        }
    }

    /**
     * @brief Check if a key is in the allowed list
     */
    [[nodiscard]] bool isAllowed(uint16_t scancode) const noexcept {
        return allowedKeys_.find(scancode) != allowedKeys_.end();
    }

    /**
     * @brief Get count of allowed keys
     */
    [[nodiscard]] size_t allowedKeyCount() const noexcept { return allowedKeys_.size(); }

    bool process(Context& ctx) override {
        // Only filter keys that haven't been processed yet
        // (i.e., they're still in Forward state - unmapped)
        if (ctx.action == Action::Forward) {
            // Check if this key is in our allowed list
            if (allowedKeys_.find(ctx.outputScancode) == allowedKeys_.end()) {
                // Key is not mapped - block it
                ctx.action = Action::Consume;
            }
        }

        // Always continue pipeline (this is a passive filter)
        return true;
    }

    [[nodiscard]] const char* name() const noexcept override { return "StrictModeFilter"; }

  private:
    std::unordered_set<uint16_t> allowedKeys_; // O(1) lookup performance
};

} // namespace keyflow
