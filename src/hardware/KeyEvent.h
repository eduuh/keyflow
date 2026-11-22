#pragma once

#include <cstdint>
#include <type_traits>

namespace keyflow {

/**
 * @brief Simple key event from hardware layer
 *
 * Normalized physical key identity with explicit transition and source.
 *
 * Note: scancode is uint16_t to support extended keys (E0/E1 prefixed).
 * Standard keys are 0-127, extended keys can be larger.
 */
enum class KeyAction : uint8_t { Release, Press, Repeat };

struct KeyEvent {
    uint16_t scancode; // Hardware scancode (0-65535, typically 0-511)
    uint16_t source;
    bool isDown; // true = key pressed, false = key released
    KeyAction action;
    bool native; // Linux passthrough code outside the canonical key namespace

    constexpr KeyEvent() noexcept : KeyEvent(0, false) {}
    constexpr KeyEvent(uint16_t sc, bool down) noexcept
        : KeyEvent(sc, down ? KeyAction::Press : KeyAction::Release) {}
    constexpr KeyEvent(uint16_t sc, KeyAction transition, uint16_t device = 0,
                       bool nativeCode = false) noexcept
        : scancode(sc), source(device), isDown(transition != KeyAction::Release),
          action(transition), native(nativeCode) {}
};

// Compile-time safety checks
static_assert(std::is_trivially_copyable_v<KeyEvent>, "KeyEvent must be trivially copyable");
static_assert(sizeof(KeyEvent) <= 8, "KeyEvent should be small for efficient passing");

} // namespace keyflow
