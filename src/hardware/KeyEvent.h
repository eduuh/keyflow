#pragma once

#include <cstdint>
#include <type_traits>

namespace keyflow {

/**
 * @brief Simple key event from hardware layer
 *
 * Minimal representation of a keystroke from the Interception driver.
 * No business logic, just raw hardware data.
 *
 * Note: scancode is uint16_t to support extended keys (E0/E1 prefixed).
 * Standard keys are 0-127, extended keys can be larger.
 */
struct KeyEvent {
    uint16_t scancode; // Hardware scancode (0-65535, typically 0-511)
    bool isDown;       // true = key pressed, false = key released

    constexpr KeyEvent() noexcept : scancode(0), isDown(false) {}
    constexpr KeyEvent(uint16_t sc, bool down) noexcept : scancode(sc), isDown(down) {}
};

// Compile-time safety checks
static_assert(std::is_trivially_copyable_v<KeyEvent>, "KeyEvent must be trivially copyable");
static_assert(sizeof(KeyEvent) <= 8, "KeyEvent should be small for efficient passing");

} // namespace keyflow
