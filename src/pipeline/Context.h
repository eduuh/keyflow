#pragma once

#include "../hardware/KeyEvent.h"

#include <cstdint>
#include <type_traits>
#include <vector>

namespace keyflow {

/**
 * @brief Processing action to take after pipeline completes
 */
enum class Action {
    Forward, // Forward original keystroke unchanged
    Replace, // Replace with outputScancode
    Consume  // Consume keystroke (no output)
};

/**
 * @brief Mutable processing context passed through pipeline
 *
 * Stack-allocated, cache-friendly structure (target: 64 bytes or less).
 * Each processor can read and modify this context.
 *
 * Design:
 * - Input fields: Set once at start (scancode, isDown)
 * - State fields: Set by processors (tapped, repeat, modifiers)
 * - Output fields: Determine result (action, outputScancode)
 */
struct Context {
    // ===== INPUT (set at initialization) =====
    uint16_t scancode; // Original hardware scancode (uint16_t for extended keys)
    bool isDown;       // true = press, false = release

    // ===== STATE (set by processors) =====
    bool tapped = false;      // Was this a tap? (TapDetector)
    bool tappedSlow = false;  // Was this a slow tap?
    bool tapHoldMake = false; // Tap-hold transition?
    bool repeat = false;      // Hardware key repeat?
    uint32_t modifiers = 0;   // Current modifier state (bitmask)

    // ===== OUTPUT (determines result) =====
    Action action = Action::Forward; // What to do with this keystroke
    uint16_t outputScancode = 0;     // Replacement scancode (if Replace)
    bool injectShift = false;        // Inject Shift modifier with output key

    /**
     * @brief Initialize context from hardware event
     */
    void initialize(const KeyEvent& event) noexcept {
        scancode = event.scancode;
        isDown = event.isDown;
        outputScancode = event.scancode; // Default: no change

        // Reset state
        tapped = false;
        tappedSlow = false;
        tapHoldMake = false;
        repeat = false;
        modifiers = 0;
        action = Action::Forward;
        injectShift = false;
    }

    // Helper methods for better readability
    [[nodiscard]] constexpr bool isKeyDown() const noexcept { return isDown; }
    [[nodiscard]] constexpr bool isKeyUp() const noexcept { return !isDown; }
    [[nodiscard]] constexpr bool hasModifier(uint32_t modBit) const noexcept {
        return (modifiers & modBit) != 0;
    }
};

// Compile-time safety checks
static_assert(std::is_trivially_copyable_v<Context>,
              "Context must be trivially copyable for performance");
static_assert(sizeof(Context) <= 64, "Context should fit in cache line for performance");

} // namespace keyflow
