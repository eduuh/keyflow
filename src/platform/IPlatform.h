#pragma once

#include "hardware/KeyEvent.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace keyflow {

class IHardwareIO {
  public:
    virtual ~IHardwareIO() = default;
    [[nodiscard]] virtual bool initialize() noexcept = 0;
    virtual void shutdown() noexcept = 0;
    [[nodiscard]] virtual std::optional<KeyEvent> waitForKey(int timeoutMS = 0) noexcept = 0;
    virtual void sendKey(uint16_t scancode, bool isDown) noexcept = 0;
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;
};

class ISystemTray {
  public:
    virtual ~ISystemTray() = default;
    [[nodiscard]] virtual bool initialize(std::string appName) = 0;
    virtual void showNotification(std::string_view title, std::string_view message) noexcept = 0;
    [[nodiscard]] virtual bool processMessages() noexcept = 0;
};

class ISingleInstanceLock {
  public:
    virtual ~ISingleInstanceLock() = default;
    [[nodiscard]] virtual bool acquire() noexcept = 0;
    virtual void release() noexcept = 0;
};

class IPlatformInit {
  public:
    virtual ~IPlatformInit() = default;
    // Surface a fatal startup error to the user via the platform's modal UI
    // (MessageBox on Windows). Needed because SUBSYSTEM:WINDOWS swallows
    // stderr when launched from Explorer — without this, init failures look
    // like the exe did nothing.
    virtual void showFatalError(std::string_view message) noexcept = 0;
    using ShutdownCallback = void (*)();
    virtual void installSignalHandlers(ShutdownCallback callback) noexcept = 0;
};

} // namespace keyflow
