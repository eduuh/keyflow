#pragma once

#ifdef __linux__

#    include "hardware/KeyEvent.h"
#    include "hardware/Scancodes.h"

#    include <libevdev/libevdev.h>
#    include <linux/input-event-codes.h>
#    include <optional>
#    include <string_view>

namespace keyflow::linux_backend {

struct KeycodeMapping {
    uint16_t canonical;
    uint16_t evdev;
};

inline constexpr KeycodeMapping keycodeMappings[] = {
    {SC_ESCAPE, KEY_ESC},
    {SC_F1, KEY_F1},
    {SC_F2, KEY_F2},
    {SC_F3, KEY_F3},
    {SC_F4, KEY_F4},
    {SC_F5, KEY_F5},
    {SC_F6, KEY_F6},
    {SC_F7, KEY_F7},
    {SC_F8, KEY_F8},
    {SC_F9, KEY_F9},
    {SC_F10, KEY_F10},
    {SC_F11, KEY_F11},
    {SC_F12, KEY_F12},
    {SC_GRAVE, KEY_GRAVE},
    {SC_1, KEY_1},
    {SC_2, KEY_2},
    {SC_3, KEY_3},
    {SC_4, KEY_4},
    {SC_5, KEY_5},
    {SC_6, KEY_6},
    {SC_7, KEY_7},
    {SC_8, KEY_8},
    {SC_9, KEY_9},
    {SC_0, KEY_0},
    {SC_MINUS, KEY_MINUS},
    {SC_EQUALS, KEY_EQUAL},
    {SC_BACKSPACE, KEY_BACKSPACE},
    {SC_TAB, KEY_TAB},
    {SC_Q, KEY_Q},
    {SC_W, KEY_W},
    {SC_E, KEY_E},
    {SC_R, KEY_R},
    {SC_T, KEY_T},
    {SC_Y, KEY_Y},
    {SC_U, KEY_U},
    {SC_I, KEY_I},
    {SC_O, KEY_O},
    {SC_P, KEY_P},
    {SC_LBRACKET, KEY_LEFTBRACE},
    {SC_RBRACKET, KEY_RIGHTBRACE},
    {SC_BACKSLASH, KEY_BACKSLASH},
    {SC_CAPSLOCK, KEY_CAPSLOCK},
    {SC_A, KEY_A},
    {SC_S, KEY_S},
    {SC_D, KEY_D},
    {SC_F, KEY_F},
    {SC_G, KEY_G},
    {SC_H, KEY_H},
    {SC_J, KEY_J},
    {SC_K, KEY_K},
    {SC_L, KEY_L},
    {SC_SEMICOLON, KEY_SEMICOLON},
    {SC_APOSTROPHE, KEY_APOSTROPHE},
    {SC_ENTER, KEY_ENTER},
    {SC_LSHIFT, KEY_LEFTSHIFT},
    {SC_Z, KEY_Z},
    {SC_X, KEY_X},
    {SC_C, KEY_C},
    {SC_V, KEY_V},
    {SC_B, KEY_B},
    {SC_N, KEY_N},
    {SC_M, KEY_M},
    {SC_COMMA, KEY_COMMA},
    {SC_PERIOD, KEY_DOT},
    {SC_SLASH, KEY_SLASH},
    {SC_RSHIFT, KEY_RIGHTSHIFT},
    {SC_LCTRL, KEY_LEFTCTRL},
    {SC_LWIN, KEY_LEFTMETA},
    {SC_LALT, KEY_LEFTALT},
    {SC_SPACE, KEY_SPACE},
    {SC_RALT, KEY_RIGHTALT},
    {SC_RWIN, KEY_RIGHTMETA},
    {SC_APPS, KEY_COMPOSE},
    {SC_RCTRL, KEY_RIGHTCTRL},
    {SC_INSERT, KEY_INSERT},
    {SC_DELETE, KEY_DELETE},
    {SC_HOME, KEY_HOME},
    {SC_END, KEY_END},
    {SC_PAGEUP, KEY_PAGEUP},
    {SC_PAGEDOWN, KEY_PAGEDOWN},
    {SC_UP, KEY_UP},
    {SC_DOWN, KEY_DOWN},
    {SC_LEFT, KEY_LEFT},
    {SC_RIGHT, KEY_RIGHT},
    {SC_PRINTSCREEN, KEY_SYSRQ},
    {SC_SCROLLLOCK, KEY_SCROLLLOCK},
    {SC_PAUSE, KEY_PAUSE},
    {SC_NUMLOCK, KEY_NUMLOCK},
    {SC_KP_SLASH, KEY_KPSLASH},
    {SC_KP_ASTERISK, KEY_KPASTERISK},
    {SC_KP_MINUS, KEY_KPMINUS},
    {SC_KP_PLUS, KEY_KPPLUS},
    {SC_KP_ENTER, KEY_KPENTER},
    {SC_KP_PERIOD, KEY_KPDOT},
    {SC_KP_0, KEY_KP0},
    {SC_KP_1, KEY_KP1},
    {SC_KP_2, KEY_KP2},
    {SC_KP_3, KEY_KP3},
    {SC_KP_4, KEY_KP4},
    {SC_KP_5, KEY_KP5},
    {SC_KP_6, KEY_KP6},
    {SC_KP_7, KEY_KP7},
    {SC_KP_8, KEY_KP8},
    {SC_KP_9, KEY_KP9},
    {SC_VOLUME_MUTE, KEY_MUTE},
    {SC_VOLUME_DOWN, KEY_VOLUMEDOWN},
    {SC_VOLUME_UP, KEY_VOLUMEUP},
    {SC_MEDIA_NEXT, KEY_NEXTSONG},
    {SC_MEDIA_PREV, KEY_PREVIOUSSONG},
    {SC_MEDIA_STOP, KEY_STOPCD},
    {SC_MEDIA_PLAY, KEY_PLAYPAUSE},
    {SC_BROWSER_BACK, KEY_BACK},
    {SC_BROWSER_FORWARD, KEY_FORWARD},
    {SC_BROWSER_REFRESH, KEY_REFRESH},
    {SC_BROWSER_STOP, KEY_STOP},
    {SC_BROWSER_SEARCH, KEY_SEARCH},
    {SC_BROWSER_FAVORITES, KEY_BOOKMARKS},
    {SC_BROWSER_HOME, KEY_HOMEPAGE},
};

inline constexpr std::optional<uint16_t> toEvdev(uint16_t canonical) noexcept {
    for (const auto& mapping : keycodeMappings) {
        if (mapping.canonical == canonical)
            return mapping.evdev;
    }
    return std::nullopt;
}

inline constexpr std::optional<uint16_t> toCanonical(uint16_t evdev) noexcept {
    for (const auto& mapping : keycodeMappings) {
        if (mapping.evdev == evdev)
            return mapping.canonical;
    }
    return std::nullopt;
}

inline bool isKeyboardCode(uint16_t code) noexcept {
    if (code == KEY_RESERVED || code > KEY_MAX)
        return false;
    const char* name = libevdev_event_code_get_name(EV_KEY, code);
    return name && std::string_view(name).substr(0, 4) == "KEY_";
}

inline std::optional<KeyEvent> decodeKey(const libevdev* device, const input_event& event,
                                         uint16_t source) noexcept {
    if (source == 0 || event.type != EV_KEY || event.value < 0 || event.value > 2 ||
        !isKeyboardCode(event.code) || !libevdev_has_event_code(device, EV_KEY, event.code))
        return std::nullopt;
    const auto canonical = toCanonical(event.code);
    const auto action = event.value == 0   ? KeyAction::Release
                        : event.value == 1 ? KeyAction::Press
                                           : KeyAction::Repeat;
    return KeyEvent(canonical.value_or(event.code), action, source, !canonical.has_value());
}

} // namespace keyflow::linux_backend

#endif
