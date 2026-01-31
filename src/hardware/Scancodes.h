#pragma once

/**
 * @file Scancodes.h
 * @brief Standard keyboard scancode definitions
 *
 * Makes code more readable by using named constants instead of hex values.
 * Example: SC_CAPSLOCK instead of 0x3A
 *
 * Reference: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 */

#include <cstdint>

namespace keyflow {

// ========================================
// MAIN KEYBOARD AREA
// ========================================

// Row 1: Function keys
constexpr uint16_t SC_ESCAPE = 0x01;
constexpr uint16_t SC_F1 = 0x3B;
constexpr uint16_t SC_F2 = 0x3C;
constexpr uint16_t SC_F3 = 0x3D;
constexpr uint16_t SC_F4 = 0x3E;
constexpr uint16_t SC_F5 = 0x3F;
constexpr uint16_t SC_F6 = 0x40;
constexpr uint16_t SC_F7 = 0x41;
constexpr uint16_t SC_F8 = 0x42;
constexpr uint16_t SC_F9 = 0x43;
constexpr uint16_t SC_F10 = 0x44;
constexpr uint16_t SC_F11 = 0x57;
constexpr uint16_t SC_F12 = 0x58;

// Row 2: Number row
constexpr uint16_t SC_GRAVE = 0x29; // ` ~
constexpr uint16_t SC_1 = 0x02;
constexpr uint16_t SC_2 = 0x03;
constexpr uint16_t SC_3 = 0x04;
constexpr uint16_t SC_4 = 0x05;
constexpr uint16_t SC_5 = 0x06;
constexpr uint16_t SC_6 = 0x07;
constexpr uint16_t SC_7 = 0x08;
constexpr uint16_t SC_8 = 0x09;
constexpr uint16_t SC_9 = 0x0A;
constexpr uint16_t SC_0 = 0x0B;
constexpr uint16_t SC_MINUS = 0x0C;  // - _
constexpr uint16_t SC_EQUALS = 0x0D; // = +
constexpr uint16_t SC_BACKSPACE = 0x0E;

// Row 3: QWERTY row
constexpr uint16_t SC_TAB = 0x0F;
constexpr uint16_t SC_Q = 0x10;
constexpr uint16_t SC_W = 0x11;
constexpr uint16_t SC_E = 0x12;
constexpr uint16_t SC_R = 0x13;
constexpr uint16_t SC_T = 0x14;
constexpr uint16_t SC_Y = 0x15;
constexpr uint16_t SC_U = 0x16;
constexpr uint16_t SC_I = 0x17;
constexpr uint16_t SC_O = 0x18;
constexpr uint16_t SC_P = 0x19;
constexpr uint16_t SC_LBRACKET = 0x1A;  // [ {
constexpr uint16_t SC_RBRACKET = 0x1B;  // ] }
constexpr uint16_t SC_BACKSLASH = 0x2B; // \ |

// Row 4: ASDF row
constexpr uint16_t SC_CAPSLOCK = 0x3A;
constexpr uint16_t SC_A = 0x1E;
constexpr uint16_t SC_S = 0x1F;
constexpr uint16_t SC_D = 0x20;
constexpr uint16_t SC_F = 0x21;
constexpr uint16_t SC_G = 0x22;
constexpr uint16_t SC_H = 0x23;
constexpr uint16_t SC_J = 0x24;
constexpr uint16_t SC_K = 0x25;
constexpr uint16_t SC_L = 0x26;
constexpr uint16_t SC_SEMICOLON = 0x27;  // ; :
constexpr uint16_t SC_APOSTROPHE = 0x28; // ' "
constexpr uint16_t SC_ENTER = 0x1C;

// Row 5: ZXCV row
constexpr uint16_t SC_LSHIFT = 0x2A;
constexpr uint16_t SC_Z = 0x2C;
constexpr uint16_t SC_X = 0x2D;
constexpr uint16_t SC_C = 0x2E;
constexpr uint16_t SC_V = 0x2F;
constexpr uint16_t SC_B = 0x30;
constexpr uint16_t SC_N = 0x31;
constexpr uint16_t SC_M = 0x32;
constexpr uint16_t SC_COMMA = 0x33;  // , <
constexpr uint16_t SC_PERIOD = 0x34; // . >
constexpr uint16_t SC_SLASH = 0x35;  // / ?
constexpr uint16_t SC_RSHIFT = 0x36;

// Row 6: Bottom row
constexpr uint16_t SC_LCTRL = 0x1D;
constexpr uint16_t SC_LWIN = 0xE05B; // Extended key
constexpr uint16_t SC_LALT = 0x38;
constexpr uint16_t SC_SPACE = 0x39;
constexpr uint16_t SC_RALT = 0xE038;  // Extended key
constexpr uint16_t SC_RWIN = 0xE05C;  // Extended key
constexpr uint16_t SC_APPS = 0xE05D;  // Context menu key
constexpr uint16_t SC_RCTRL = 0xE01D; // Extended key

// ========================================
// EXTENDED KEYS (E0 prefix)
// ========================================

// Navigation cluster
constexpr uint16_t SC_INSERT = 0xE052;
constexpr uint16_t SC_DELETE = 0xE053;
constexpr uint16_t SC_HOME = 0xE047;
constexpr uint16_t SC_END = 0xE04F;
constexpr uint16_t SC_PAGEUP = 0xE049;
constexpr uint16_t SC_PAGEDOWN = 0xE051;

// Arrow keys
constexpr uint16_t SC_UP = 0xE048;
constexpr uint16_t SC_DOWN = 0xE050;
constexpr uint16_t SC_LEFT = 0xE04B;
constexpr uint16_t SC_RIGHT = 0xE04D;

// Special keys
constexpr uint16_t SC_PRINTSCREEN = 0xE037; // Also uses 0xE02A prefix
constexpr uint16_t SC_SCROLLLOCK = 0x46;
constexpr uint16_t SC_PAUSE = 0xE11D; // Complex sequence

// Numpad (when NumLock is off, these become extended)
constexpr uint16_t SC_NUMLOCK = 0x45;
constexpr uint16_t SC_KP_SLASH = 0xE035;
constexpr uint16_t SC_KP_ASTERISK = 0x37;
constexpr uint16_t SC_KP_MINUS = 0x4A;
constexpr uint16_t SC_KP_PLUS = 0x4E;
constexpr uint16_t SC_KP_ENTER = 0xE01C;
constexpr uint16_t SC_KP_PERIOD = 0x53;
constexpr uint16_t SC_KP_0 = 0x52;
constexpr uint16_t SC_KP_1 = 0x4F;
constexpr uint16_t SC_KP_2 = 0x50;
constexpr uint16_t SC_KP_3 = 0x51;
constexpr uint16_t SC_KP_4 = 0x4B;
constexpr uint16_t SC_KP_5 = 0x4C;
constexpr uint16_t SC_KP_6 = 0x4D;
constexpr uint16_t SC_KP_7 = 0x47;
constexpr uint16_t SC_KP_8 = 0x48;
constexpr uint16_t SC_KP_9 = 0x49;

// ========================================
// MULTIMEDIA KEYS (E0 prefix)
// ========================================

constexpr uint16_t SC_VOLUME_MUTE = 0xE020;
constexpr uint16_t SC_VOLUME_DOWN = 0xE02E;
constexpr uint16_t SC_VOLUME_UP = 0xE030;
constexpr uint16_t SC_MEDIA_NEXT = 0xE019;
constexpr uint16_t SC_MEDIA_PREV = 0xE010;
constexpr uint16_t SC_MEDIA_STOP = 0xE024;
constexpr uint16_t SC_MEDIA_PLAY = 0xE022;

// ========================================
// BROWSER/APP KEYS (E0 prefix)
// ========================================

constexpr uint16_t SC_BROWSER_BACK = 0xE06A;
constexpr uint16_t SC_BROWSER_FORWARD = 0xE069;
constexpr uint16_t SC_BROWSER_REFRESH = 0xE067;
constexpr uint16_t SC_BROWSER_STOP = 0xE068;
constexpr uint16_t SC_BROWSER_SEARCH = 0xE065;
constexpr uint16_t SC_BROWSER_FAVORITES = 0xE066;
constexpr uint16_t SC_BROWSER_HOME = 0xE032;

// ========================================
// SPECIAL VALUES
// ========================================

constexpr uint16_t SC_NOP = 0x00;        // No operation (consume key)
constexpr uint16_t SC_UNMAPPED = 0xFFFF; // No mapping defined

// ========================================
// SCANCODE HELPERS
// ========================================

/**
 * @brief Check if scancode is an extended key (E0 prefix)
 */
inline constexpr bool isExtendedKey(uint16_t scancode) noexcept {
    return (scancode & 0xE000) == 0xE000;
}

/**
 * @brief Get base scancode (strip E0 prefix)
 */
inline constexpr uint16_t getBaseScancode(uint16_t scancode) noexcept {
    return scancode & 0x00FF;
}

/**
 * @brief Check if scancode is a modifier key
 */
inline constexpr bool isModifier(uint16_t scancode) noexcept {
    return scancode == SC_LSHIFT || scancode == SC_RSHIFT || scancode == SC_LCTRL ||
           scancode == SC_RCTRL || scancode == SC_LALT || scancode == SC_RALT ||
           scancode == SC_LWIN || scancode == SC_RWIN;
}

/**
 * @brief Check if scancode is a numpad key
 */
inline constexpr bool isNumpad(uint16_t scancode) noexcept {
    return (scancode >= SC_KP_0 && scancode <= SC_KP_9) || scancode == SC_KP_SLASH ||
           scancode == SC_KP_ASTERISK || scancode == SC_KP_MINUS || scancode == SC_KP_PLUS ||
           scancode == SC_KP_ENTER || scancode == SC_KP_PERIOD;
}

/**
 * @brief Get human-readable name for a scancode
 */
inline const char* getScancodeNameOrNull(uint16_t scancode) noexcept {
    switch (scancode) {
        // Function keys
        case SC_ESCAPE:
            return "Esc";
        case SC_F1:
            return "F1";
        case SC_F2:
            return "F2";
        case SC_F3:
            return "F3";
        case SC_F4:
            return "F4";
        case SC_F5:
            return "F5";
        case SC_F6:
            return "F6";
        case SC_F7:
            return "F7";
        case SC_F8:
            return "F8";
        case SC_F9:
            return "F9";
        case SC_F10:
            return "F10";
        case SC_F11:
            return "F11";
        case SC_F12:
            return "F12";

        // Number row
        case SC_GRAVE:
            return "`";
        case SC_1:
            return "1";
        case SC_2:
            return "2";
        case SC_3:
            return "3";
        case SC_4:
            return "4";
        case SC_5:
            return "5";
        case SC_6:
            return "6";
        case SC_7:
            return "7";
        case SC_8:
            return "8";
        case SC_9:
            return "9";
        case SC_0:
            return "0";
        case SC_MINUS:
            return "-";
        case SC_EQUALS:
            return "=";
        case SC_BACKSPACE:
            return "Bksp";

        // QWERTY row
        case SC_TAB:
            return "Tab";
        case SC_Q:
            return "Q";
        case SC_W:
            return "W";
        case SC_E:
            return "E";
        case SC_R:
            return "R";
        case SC_T:
            return "T";
        case SC_Y:
            return "Y";
        case SC_U:
            return "U";
        case SC_I:
            return "I";
        case SC_O:
            return "O";
        case SC_P:
            return "P";
        case SC_LBRACKET:
            return "[";
        case SC_RBRACKET:
            return "]";
        case SC_BACKSLASH:
            return "\\";

        // ASDF row
        case SC_CAPSLOCK:
            return "Caps";
        case SC_A:
            return "A";
        case SC_S:
            return "S";
        case SC_D:
            return "D";
        case SC_F:
            return "F";
        case SC_G:
            return "G";
        case SC_H:
            return "H";
        case SC_J:
            return "J";
        case SC_K:
            return "K";
        case SC_L:
            return "L";
        case SC_SEMICOLON:
            return ";";
        case SC_APOSTROPHE:
            return "'";
        case SC_ENTER:
            return "Enter";

        // ZXCV row
        case SC_LSHIFT:
            return "LShift";
        case SC_Z:
            return "Z";
        case SC_X:
            return "X";
        case SC_C:
            return "C";
        case SC_V:
            return "V";
        case SC_B:
            return "B";
        case SC_N:
            return "N";
        case SC_M:
            return "M";
        case SC_COMMA:
            return ",";
        case SC_PERIOD:
            return ".";
        case SC_SLASH:
            return "/";
        case SC_RSHIFT:
            return "RShift";

        // Bottom row
        case SC_LCTRL:
            return "LCtrl";
        case SC_LWIN:
            return "LWin";
        case SC_LALT:
            return "LAlt";
        case SC_SPACE:
            return "Space";
        case SC_RALT:
            return "RAlt";
        case SC_RWIN:
            return "RWin";
        case SC_APPS:
            return "Apps";
        case SC_RCTRL:
            return "RCtrl";

        // Navigation
        case SC_INSERT:
            return "Ins";
        case SC_DELETE:
            return "Del";
        case SC_HOME:
            return "Home";
        case SC_END:
            return "End";
        case SC_PAGEUP:
            return "PgUp";
        case SC_PAGEDOWN:
            return "PgDn";

        // Arrows
        case SC_UP:
            return "Up";
        case SC_DOWN:
            return "Down";
        case SC_LEFT:
            return "Left";
        case SC_RIGHT:
            return "Right";

        default:
            return nullptr;
    }
}

} // namespace keyflow
