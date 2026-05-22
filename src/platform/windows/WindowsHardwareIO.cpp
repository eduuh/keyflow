#include "WindowsHardwareIO.h"

#include "DebugLog.h"
#include "interception.h"

#include <iostream>

namespace keyflow {

bool WindowsHardwareIO::initialize() noexcept {
    if (initialized_)
        return true;

    context_ = interception_create_context();
    if (!context_) {
        std::cerr << "[HardwareIO] Failed to create Interception context\n";
        return false;
    }

    interception_set_filter(context_, interception_is_keyboard,
                            INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP |
                                INTERCEPTION_FILTER_KEY_E0 | INTERCEPTION_FILTER_KEY_E1);

    initialized_ = true;
    DEBUG_LOG("[HardwareIO] Initialized\n");
    return true;
}

void WindowsHardwareIO::shutdown() noexcept {
    if (context_) {
        interception_destroy_context(context_);
        context_ = nullptr;
    }
    initialized_ = false;
    DEBUG_LOG("[HardwareIO] Shutdown\n");
}

std::optional<KeyEvent> WindowsHardwareIO::waitForKey(int timeoutMS) noexcept {
    if (!initialized_ || !context_)
        return std::nullopt;

    InterceptionDevice device = 0;
    InterceptionStroke stroke;

    if (timeoutMS > 0) {
        device = interception_wait_with_timeout(context_, timeoutMS);
        if (!device)
            return std::nullopt;
    } else {
        device = interception_wait(context_);
    }

    int received = interception_receive(context_, device, &stroke, 1);
    if (received <= 0)
        return std::nullopt;

    currentDevice_.store(device, std::memory_order_relaxed);

    auto* keyStroke = reinterpret_cast<InterceptionKeyStroke*>(&stroke);
    uint16_t scancode = keyStroke->code;
    bool isDown = !(keyStroke->state & INTERCEPTION_KEY_UP);

    if (keyStroke->state & INTERCEPTION_KEY_E0)
        scancode |= 0xE000;
    if (keyStroke->state & INTERCEPTION_KEY_E1)
        scancode |= 0xE100;

    return KeyEvent(scancode, isDown);
}

void WindowsHardwareIO::sendKey(uint16_t scancode, bool isDown) noexcept {
    const int device = currentDevice_.load(std::memory_order_relaxed);
    if (!initialized_ || context_ == nullptr || device == 0) {
        return;
    }

    InterceptionKeyStroke stroke = {};
    uint16_t baseScancode = scancode & 0x00FF;
    bool isE0 = (scancode & 0xE000) == 0xE000;
    bool isE1 = (scancode & 0xE100) == 0xE100;

    stroke.code = baseScancode;
    stroke.state = isDown ? 0 : INTERCEPTION_KEY_UP;
    if (isE0)
        stroke.state |= INTERCEPTION_KEY_E0;
    if (isE1)
        stroke.state |= INTERCEPTION_KEY_E1;

    interception_send(context_, device, reinterpret_cast<InterceptionStroke*>(&stroke), 1);
}

} // namespace keyflow
