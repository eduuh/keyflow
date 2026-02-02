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

    // Custom modifiers (bits 9-31 available for user-defined modifiers)
    Custom1 = 1 << 9,
    Custom2 = 1 << 10,
    Custom3 = 1 << 11,
    Custom4 = 1 << 12,
    Custom5 = 1 << 13,
    Custom6 = 1 << 14,
    Custom7 = 1 << 15,
    Custom8 = 1 << 16,
    Custom9 = 1 << 17,
    Custom10 = 1 << 18,
    Custom11 = 1 << 19,
    Custom12 = 1 << 20,
    Custom13 = 1 << 21,
    Custom14 = 1 << 22,
    Custom15 = 1 << 23,
    Custom16 = 1 << 24,
    Custom17 = 1 << 25,
    Custom18 = 1 << 26,
    Custom19 = 1 << 27,
    Custom20 = 1 << 28,
    Custom21 = 1 << 29,
    Custom22 = 1 << 30,
    Custom23 = 1u << 31,

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
 *
 * Note: This function handles standard modifiers only. Custom modifiers
 * are resolved at runtime by ModifierTracker.
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
 * @brief Get custom modifier bit by index (1-23)
 */
constexpr ModifierBit getCustomModifierBit(int index) noexcept {
    switch (index) {
        case 1:
            return ModifierBit::Custom1;
        case 2:
            return ModifierBit::Custom2;
        case 3:
            return ModifierBit::Custom3;
        case 4:
            return ModifierBit::Custom4;
        case 5:
            return ModifierBit::Custom5;
        case 6:
            return ModifierBit::Custom6;
        case 7:
            return ModifierBit::Custom7;
        case 8:
            return ModifierBit::Custom8;
        case 9:
            return ModifierBit::Custom9;
        case 10:
            return ModifierBit::Custom10;
        case 11:
            return ModifierBit::Custom11;
        case 12:
            return ModifierBit::Custom12;
        case 13:
            return ModifierBit::Custom13;
        case 14:
            return ModifierBit::Custom14;
        case 15:
            return ModifierBit::Custom15;
        case 16:
            return ModifierBit::Custom16;
        case 17:
            return ModifierBit::Custom17;
        case 18:
            return ModifierBit::Custom18;
        case 19:
            return ModifierBit::Custom19;
        case 20:
            return ModifierBit::Custom20;
        case 21:
            return ModifierBit::Custom21;
        case 22:
            return ModifierBit::Custom22;
        case 23:
            return ModifierBit::Custom23;
        default:
            return ModifierBit::None;
    }
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
