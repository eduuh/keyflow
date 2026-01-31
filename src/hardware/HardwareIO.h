#pragma once

#include "KeyEvent.h"

#include <optional>

namespace keyflow {

/**
 * @brief Hardware I/O abstraction layer
 *
 * Thin wrapper around the Interception driver.
 *
 * Responsibilities:
 * - Receive keystrokes from driver
 * - Send keystrokes to driver
 * - That's it! No logic, no state.
 *
 * Target: ~50 lines of implementation
 */
class HardwareIO {
  public:
    /**
     * @brief Initialize hardware driver connection
     * @return true if successful, false otherwise
     */
    bool initialize() noexcept;

    /**
     * @brief Cleanup and release driver resources
     */
    void shutdown() noexcept;

    /**
     * @brief Wait for next keystroke from any device
     * @param timeoutMS Timeout in milliseconds (0 = infinite)
     * @return KeyEvent if available, nullopt on timeout
     */
    std::optional<KeyEvent> waitForKey(int timeoutMS = 0) noexcept;

    /**
     * @brief Send a single keystroke to the OS
     * @param scancode Hardware scancode
     * @param isDown true for press, false for release
     */
    void sendKey(uint16_t scancode, bool isDown) noexcept;

    /**
     * @brief Check if hardware is initialized
     */
    bool isInitialized() const noexcept { return initialized_; }

  private:
    bool initialized_ = false;
    void* context_ = nullptr; // Interception context (opaque pointer)
    int currentDevice_ = 0;   // Last device that sent a key
};

} // namespace keyflow
