#include "HardwareIO.h"

#include "interception.h"

#include <iostream>

namespace keyflow {

bool HardwareIO::initialize() noexcept {
    if (initialized_) {
        return true; // Already initialized
    }

    context_ = interception_create_context();

    if (!context_) {
        std::cerr << "[HardwareIO] Failed to create Interception context\n";
        return false;
    }

    // Set keyboard filter to capture all keystrokes
    interception_set_filter(context_, interception_is_keyboard,
                            INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);

    initialized_ = true;
    std::cout << "[HardwareIO] Initialized successfully\n";
    return true;
}

void HardwareIO::shutdown() noexcept {
    if (context_) {
        interception_destroy_context(context_);
        context_ = nullptr;
    }
    initialized_ = false;
    std::cout << "[HardwareIO] Shutdown complete\n";
}

std::optional<KeyEvent> HardwareIO::waitForKey(int timeoutMS) noexcept {
    if (!initialized_ || !context_) {
        return std::nullopt;
    }

    InterceptionDevice device = 0;
    InterceptionStroke stroke;

    // Wait for device with optional timeout
    if (timeoutMS > 0) {
        device = interception_wait_with_timeout(context_, timeoutMS);
        if (!device) {
            return std::nullopt; // Timeout
        }
    } else {
        device = interception_wait(context_);
    }

    // Receive keystroke from device
    int received = interception_receive(context_, device, &stroke, 1);

    if (received <= 0) {
        return std::nullopt;
    }

    currentDevice_ = device;

    // Extract scancode and state from Interception stroke
    auto* keyStroke = reinterpret_cast<InterceptionKeyStroke*>(&stroke);
    uint16_t scancode = keyStroke->code;
    bool isDown = !(keyStroke->state & INTERCEPTION_KEY_UP);

    return KeyEvent(scancode, isDown);
}

void HardwareIO::sendKey(uint16_t scancode, bool isDown) noexcept {
    if (!initialized_ || !context_ || !currentDevice_) {
        return;
    }

    InterceptionKeyStroke stroke = {};
    stroke.code = scancode;
    stroke.state = isDown ? 0 : INTERCEPTION_KEY_UP;

    interception_send(context_, currentDevice_, reinterpret_cast<InterceptionStroke*>(&stroke), 1);
}

} // namespace keyflow
