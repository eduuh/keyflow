#pragma once

#include <cstdint>

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

    KeyEvent() noexcept : scancode(0), isDown(false) {}
    KeyEvent(uint16_t sc, bool down) noexcept : scancode(sc), isDown(down) {}
};

} // namespace keyflow
