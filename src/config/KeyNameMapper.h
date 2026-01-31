#pragma once

#include "../hardware/Scancodes.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace keyflow {

/**
 * @brief Maps between key names (strings) and scancodes (uint16_t)
 */
class KeyNameMapper {
  public:
    /**
     * @brief Convert key name to scancode
     * @param name Key name (e.g., "CapsLock", "A", "LeftCtrl")
     * @return Scancode if found, nullopt otherwise
     */
    static std::optional<uint16_t> nameToScancode(const std::string& name) {
        static const std::unordered_map<std::string, uint16_t> map = createNameToScancodeMap();

        auto it = map.find(name);
        if (it != map.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Convert scancode to key name
     * @param scancode Scancode value
     * @return Key name if found, nullopt otherwise
     */
    static std::optional<std::string> scancodeToName(uint16_t scancode) {
        static const std::unordered_map<uint16_t, std::string> map = createScancodeToNameMap();

        auto it = map.find(scancode);
        if (it != map.end()) {
            return it->second;
        }
        return std::nullopt;
    }

  private:
    /**
     * @brief Create name → scancode lookup map
     */
    static std::unordered_map<std::string, uint16_t> createNameToScancodeMap() {
        return {
            // Function keys
            {"Escape", SC_ESCAPE},
            {"F1", SC_F1},
            {"F2", SC_F2},
            {"F3", SC_F3},
            {"F4", SC_F4},
            {"F5", SC_F5},
            {"F6", SC_F6},
            {"F7", SC_F7},
            {"F8", SC_F8},
            {"F9", SC_F9},
            {"F10", SC_F10},
            {"F11", SC_F11},
            {"F12", SC_F12},

            // Number row
            {"Grave", SC_GRAVE},
            {"Backtick", SC_GRAVE},
            {"Tilde", SC_GRAVE},
            {"1", SC_1},
            {"2", SC_2},
            {"3", SC_3},
            {"4", SC_4},
            {"5", SC_5},
            {"6", SC_6},
            {"7", SC_7},
            {"8", SC_8},
            {"9", SC_9},
            {"0", SC_0},
            {"Minus", SC_MINUS},
            {"Equals", SC_EQUALS},
            {"Backspace", SC_BACKSPACE},

            // Top row (QWERTY)
            {"Tab", SC_TAB},
            {"Q", SC_Q},
            {"W", SC_W},
            {"E", SC_E},
            {"R", SC_R},
            {"T", SC_T},
            {"Y", SC_Y},
            {"U", SC_U},
            {"I", SC_I},
            {"O", SC_O},
            {"P", SC_P},
            {"LeftBracket", SC_LBRACKET},
            {"LBracket", SC_LBRACKET},
            {"RightBracket", SC_RBRACKET},
            {"RBracket", SC_RBRACKET},
            {"Backslash", SC_BACKSLASH},

            // Home row (ASDF)
            {"CapsLock", SC_CAPSLOCK},
            {"Caps", SC_CAPSLOCK},
            {"A", SC_A},
            {"S", SC_S},
            {"D", SC_D},
            {"F", SC_F},
            {"G", SC_G},
            {"H", SC_H},
            {"J", SC_J},
            {"K", SC_K},
            {"L", SC_L},
            {"Semicolon", SC_SEMICOLON},
            {"Apostrophe", SC_APOSTROPHE},
            {"Quote", SC_APOSTROPHE},
            {"Enter", SC_ENTER},
            {"Return", SC_ENTER},

            // Bottom row (ZXCV)
            {"LeftShift", SC_LSHIFT},
            {"LShift", SC_LSHIFT},
            {"Z", SC_Z},
            {"X", SC_X},
            {"C", SC_C},
            {"V", SC_V},
            {"B", SC_B},
            {"N", SC_N},
            {"M", SC_M},
            {"Comma", SC_COMMA},
            {"Period", SC_PERIOD},
            {"Slash", SC_SLASH},
            {"RightShift", SC_RSHIFT},
            {"RShift", SC_RSHIFT},

            // Bottom row modifiers
            {"LeftCtrl", SC_LCTRL},
            {"LCtrl", SC_LCTRL},
            {"LeftWin", SC_LWIN},
            {"LWin", SC_LWIN},
            {"LeftSuper", SC_LWIN},
            {"LeftAlt", SC_LALT},
            {"LAlt", SC_LALT},
            {"Space", SC_SPACE},
            {"RightAlt", SC_RALT},
            {"RAlt", SC_RALT},
            {"RightWin", SC_RWIN},
            {"RWin", SC_RWIN},
            {"RightSuper", SC_RWIN},
            {"Apps", SC_APPS},
            {"Menu", SC_APPS},
            {"RightCtrl", SC_RCTRL},
            {"RCtrl", SC_RCTRL},

            // Navigation cluster
            {"Insert", SC_INSERT},
            {"Delete", SC_DELETE},
            {"Home", SC_HOME},
            {"End", SC_END},
            {"PageUp", SC_PAGEUP},
            {"PageDown", SC_PAGEDOWN},

            // Arrow keys
            {"Up", SC_UP},
            {"Down", SC_DOWN},
            {"Left", SC_LEFT},
            {"Right", SC_RIGHT},

            // Special keys
            {"PrintScreen", SC_PRINTSCREEN},
            {"Print", SC_PRINTSCREEN},
            {"ScrollLock", SC_SCROLLLOCK},
            {"Pause", SC_PAUSE},

            // Numpad
            {"NumLock", SC_NUMLOCK},
            {"KP_Slash", SC_KP_SLASH},
            {"KP_Asterisk", SC_KP_ASTERISK},
            {"KP_Minus", SC_KP_MINUS},
            {"KP_Plus", SC_KP_PLUS},
            {"KP_Enter", SC_KP_ENTER},
            {"KP_Period", SC_KP_PERIOD},
            {"KP_0", SC_KP_0},
            {"KP_1", SC_KP_1},
            {"KP_2", SC_KP_2},
            {"KP_3", SC_KP_3},
            {"KP_4", SC_KP_4},
            {"KP_5", SC_KP_5},
            {"KP_6", SC_KP_6},
            {"KP_7", SC_KP_7},
            {"KP_8", SC_KP_8},
            {"KP_9", SC_KP_9},

            // Multimedia
            {"VolumeMute", SC_VOLUME_MUTE},
            {"VolumeDown", SC_VOLUME_DOWN},
            {"VolumeUp", SC_VOLUME_UP},
            {"MediaNext", SC_MEDIA_NEXT},
            {"MediaPrev", SC_MEDIA_PREV},
            {"MediaStop", SC_MEDIA_STOP},
            {"MediaPlay", SC_MEDIA_PLAY},

            // Browser keys
            {"BrowserBack", SC_BROWSER_BACK},
            {"BrowserForward", SC_BROWSER_FORWARD},
            {"BrowserRefresh", SC_BROWSER_REFRESH},
            {"BrowserStop", SC_BROWSER_STOP},
            {"BrowserSearch", SC_BROWSER_SEARCH},
            {"BrowserFavorites", SC_BROWSER_FAVORITES},
            {"BrowserHome", SC_BROWSER_HOME},

            // Special values
            {"NOP", SC_NOP},
            {"Nop", SC_NOP},
            {"None", SC_NOP},
        };
    }

    /**
     * @brief Create scancode → name lookup map
     */
    static std::unordered_map<uint16_t, std::string> createScancodeToNameMap() {
        return {
            // Function keys
            {SC_ESCAPE, "Escape"},
            {SC_F1, "F1"},
            {SC_F2, "F2"},
            {SC_F3, "F3"},
            {SC_F4, "F4"},
            {SC_F5, "F5"},
            {SC_F6, "F6"},
            {SC_F7, "F7"},
            {SC_F8, "F8"},
            {SC_F9, "F9"},
            {SC_F10, "F10"},
            {SC_F11, "F11"},
            {SC_F12, "F12"},

            // Number row
            {SC_GRAVE, "Grave"},
            {SC_1, "1"},
            {SC_2, "2"},
            {SC_3, "3"},
            {SC_4, "4"},
            {SC_5, "5"},
            {SC_6, "6"},
            {SC_7, "7"},
            {SC_8, "8"},
            {SC_9, "9"},
            {SC_0, "0"},
            {SC_MINUS, "Minus"},
            {SC_EQUALS, "Equals"},
            {SC_BACKSPACE, "Backspace"},

            // Top row
            {SC_TAB, "Tab"},
            {SC_Q, "Q"},
            {SC_W, "W"},
            {SC_E, "E"},
            {SC_R, "R"},
            {SC_T, "T"},
            {SC_Y, "Y"},
            {SC_U, "U"},
            {SC_I, "I"},
            {SC_O, "O"},
            {SC_P, "P"},
            {SC_LBRACKET, "LeftBracket"},
            {SC_RBRACKET, "RightBracket"},
            {SC_BACKSLASH, "Backslash"},

            // Home row
            {SC_CAPSLOCK, "CapsLock"},
            {SC_A, "A"},
            {SC_S, "S"},
            {SC_D, "D"},
            {SC_F, "F"},
            {SC_G, "G"},
            {SC_H, "H"},
            {SC_J, "J"},
            {SC_K, "K"},
            {SC_L, "L"},
            {SC_SEMICOLON, "Semicolon"},
            {SC_APOSTROPHE, "Apostrophe"},
            {SC_ENTER, "Enter"},

            // Bottom row
            {SC_LSHIFT, "LeftShift"},
            {SC_Z, "Z"},
            {SC_X, "X"},
            {SC_C, "C"},
            {SC_V, "V"},
            {SC_B, "B"},
            {SC_N, "N"},
            {SC_M, "M"},
            {SC_COMMA, "Comma"},
            {SC_PERIOD, "Period"},
            {SC_SLASH, "Slash"},
            {SC_RSHIFT, "RightShift"},

            // Modifiers
            {SC_LCTRL, "LeftCtrl"},
            {SC_LWIN, "LeftWin"},
            {SC_LALT, "LeftAlt"},
            {SC_SPACE, "Space"},
            {SC_RALT, "RightAlt"},
            {SC_RWIN, "RightWin"},
            {SC_APPS, "Apps"},
            {SC_RCTRL, "RightCtrl"},

            // Navigation
            {SC_INSERT, "Insert"},
            {SC_DELETE, "Delete"},
            {SC_HOME, "Home"},
            {SC_END, "End"},
            {SC_PAGEUP, "PageUp"},
            {SC_PAGEDOWN, "PageDown"},

            // Arrows
            {SC_UP, "Up"},
            {SC_DOWN, "Down"},
            {SC_LEFT, "Left"},
            {SC_RIGHT, "Right"},

            // Special
            {SC_PRINTSCREEN, "PrintScreen"},
            {SC_SCROLLLOCK, "ScrollLock"},
            {SC_PAUSE, "Pause"},

            // Numpad
            {SC_NUMLOCK, "NumLock"},
            {SC_KP_SLASH, "KP_Slash"},
            {SC_KP_ASTERISK, "KP_Asterisk"},
            {SC_KP_MINUS, "KP_Minus"},
            {SC_KP_PLUS, "KP_Plus"},
            {SC_KP_ENTER, "KP_Enter"},
            {SC_KP_PERIOD, "KP_Period"},
            {SC_KP_0, "KP_0"},
            {SC_KP_1, "KP_1"},
            {SC_KP_2, "KP_2"},
            {SC_KP_3, "KP_3"},
            {SC_KP_4, "KP_4"},
            {SC_KP_5, "KP_5"},
            {SC_KP_6, "KP_6"},
            {SC_KP_7, "KP_7"},
            {SC_KP_8, "KP_8"},
            {SC_KP_9, "KP_9"},

            // Special values
            {SC_NOP, "NOP"},
        };
    }
};

} // namespace keyflow
