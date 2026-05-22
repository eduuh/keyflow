#pragma once

#include "platform/IPlatform.h"

#include <atomic>

namespace keyflow {

class WindowsHardwareIO final : public IHardwareIO {
  public:
    WindowsHardwareIO() = default;
    ~WindowsHardwareIO() override { shutdown(); }

    // Non-copyable, non-movable: owns Interception driver context.
    // Managed via std::unique_ptr<IHardwareIO> — moving the pointer suffices.
    WindowsHardwareIO(const WindowsHardwareIO&) = delete;
    WindowsHardwareIO& operator=(const WindowsHardwareIO&) = delete;
    WindowsHardwareIO(WindowsHardwareIO&&) = delete;
    WindowsHardwareIO& operator=(WindowsHardwareIO&&) = delete;

    [[nodiscard]] bool initialize() noexcept override;
    void shutdown() noexcept override;
    [[nodiscard]] std::optional<KeyEvent> waitForKey(int timeoutMS = 0) noexcept override;
    void sendKey(uint16_t scancode, bool isDown) noexcept override;
    [[nodiscard]] bool isInitialized() const noexcept override { return initialized_; }

  private:
    bool initialized_ = false;
    void* context_ = nullptr;
    // Atomic because the console-ctrl handler thread reads it via
    // sendKey() during shutdown while the main thread writes it in waitForKey().
    std::atomic<int> currentDevice_{0};
};

} // namespace keyflow
