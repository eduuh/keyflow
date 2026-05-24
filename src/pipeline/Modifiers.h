#pragma once

#include "../hardware/Scancodes.h"

#include <cstdint>
#include <string_view>

namespace keyflow {

/**
 * @brief Modifier key bit flags
 *
 * Used by ModifierTracker and combo processors to track
 * which modifier keys are currently held down.
 *
 * Each modifier gets a unique bit in a 32-bit bitmask.
 */
enum class ModifierBit : uint32_t {
    LeftShift = 1 << 0,
    RightShift = 1 << 1,
    LeftCtrl = 1 << 2,
    RightCtrl = 1 << 3,
    LeftAlt = 1 << 4,
    RightAlt = 1 << 5,
    LeftWin = 1 << 6,
    RightWin = 1 << 7,
    PrintScreen = 1 << 8,

    // Convenience aliases for layer systems
    None = 0,
    AnyShift = LeftShift | RightShift,
    AnyCtrl = LeftCtrl | RightCtrl,
    AnyAlt = LeftAlt | RightAlt,
    AnyWin = LeftWin | RightWin,
};

/**
 * @brief Bitwise OR operator for ModifierBit enum
 */
constexpr uint32_t operator|(ModifierBit lhs, ModifierBit rhs) noexcept {
    return static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs);
}

/**
 * @brief Bitwise AND operator for ModifierBit enum
 */
constexpr uint32_t operator&(uint32_t lhs, ModifierBit rhs) noexcept {
    return lhs & static_cast<uint32_t>(rhs);
}

/**
 * @brief Bitwise OR assignment for combining modifiers
 */
constexpr uint32_t operator|=(uint32_t& lhs, ModifierBit rhs) noexcept {
    return lhs |= static_cast<uint32_t>(rhs);
}

/**
 * @brief Bitwise AND assignment for masking modifiers
 */
constexpr uint32_t operator&=(uint32_t& lhs, ModifierBit rhs) noexcept {
    return lhs &= static_cast<uint32_t>(rhs);
}

/**
 * @brief Check if scancode is a modifier key
 */
constexpr bool isModifierKey(uint16_t scancode) noexcept {
    return scancode == SC_LSHIFT || scancode == SC_RSHIFT || scancode == SC_LCTRL ||
           scancode == SC_RCTRL || scancode == SC_LALT || scancode == SC_RALT ||
           scancode == SC_LWIN || scancode == SC_RWIN || scancode == SC_PRINTSCREEN;
}

/**
 * @brief Get modifier bit for a given scancode
 * @return ModifierBit value, or ModifierBit::None if not a modifier
 */
constexpr ModifierBit getModifierBit(uint16_t scancode) noexcept {
    switch (scancode) {
        case SC_LSHIFT:
            return ModifierBit::LeftShift;
        case SC_RSHIFT:
            return ModifierBit::RightShift;
        case SC_LCTRL:
            return ModifierBit::LeftCtrl;
        case SC_RCTRL:
            return ModifierBit::RightCtrl;
        case SC_LALT:
            return ModifierBit::LeftAlt;
        case SC_RALT:
            return ModifierBit::RightAlt;
        case SC_LWIN:
            return ModifierBit::LeftWin;
        case SC_RWIN:
            return ModifierBit::RightWin;
        case SC_PRINTSCREEN:
            return ModifierBit::PrintScreen;
        default:
            return ModifierBit::None;
    }
}

/**
 * @brief Type-safe modifier name to bit conversion
 *
 * Using string_view for compile-time string handling where possible.
 * This replaces C-string strcmp() with type-safe comparison.
 */
constexpr ModifierBit modifierNameToBit(std::string_view modName) noexcept {
    // Check common names
    if (modName == "LALT" || modName == "MOD12")
        return ModifierBit::LeftAlt;
    if (modName == "RALT" || modName == "MOD11")
        return ModifierBit::RightAlt;
    if (modName == "LCTRL" || modName == "MOD13")
        return ModifierBit::LeftCtrl;
    if (modName == "LWIN")
        return ModifierBit::LeftWin;
    if (modName == "PRINT")
        return ModifierBit::PrintScreen;
    if (modName == "LSHIFT")
        return ModifierBit::LeftShift;
    if (modName == "RSHIFT")
        return ModifierBit::RightShift;
    if (modName == "RCTRL")
        return ModifierBit::RightCtrl;
    if (modName == "RWIN")
        return ModifierBit::RightWin;

    return ModifierBit::None;
}

/**
 * @brief Check if any shift key is pressed
 */
[[nodiscard]] constexpr bool hasAnyShift(uint32_t modifiers) noexcept {
    return (modifiers & (ModifierBit::LeftShift | ModifierBit::RightShift)) != 0;
}

/**
 * @brief Check if any ctrl key is pressed
 */
[[nodiscard]] constexpr bool hasAnyCtrl(uint32_t modifiers) noexcept {
    return (modifiers & (ModifierBit::LeftCtrl | ModifierBit::RightCtrl)) != 0;
}

/**
 * @brief Check if any alt key is pressed
 */
[[nodiscard]] constexpr bool hasAnyAlt(uint32_t modifiers) noexcept {
    return (modifiers & (ModifierBit::LeftAlt | ModifierBit::RightAlt)) != 0;
}

/**
 * @brief Check if any win/super key is pressed
 */
[[nodiscard]] constexpr bool hasAnyWin(uint32_t modifiers) noexcept {
    return (modifiers & (ModifierBit::LeftWin | ModifierBit::RightWin)) != 0;
}

/**
 * @brief Check if no modifiers are pressed
 */
[[nodiscard]] constexpr bool hasNoModifiers(uint32_t modifiers) noexcept {
    return modifiers == 0;
}

} // namespace keyflow
