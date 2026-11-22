#pragma once

#ifdef __linux__

#    include "platform/IPlatform.h"

#    include <csignal>

namespace keyflow {

// Installation, pumping and destruction belong to the application's main thread.
class LinuxPlatformInit final : public IPlatformInit {
  public:
    LinuxPlatformInit() noexcept = default;
    ~LinuxPlatformInit() override;

    LinuxPlatformInit(const LinuxPlatformInit&) = delete;
    LinuxPlatformInit& operator=(const LinuxPlatformInit&) = delete;
    LinuxPlatformInit(LinuxPlatformInit&&) = delete;
    LinuxPlatformInit& operator=(LinuxPlatformInit&&) = delete;

    void showFatalError(std::string_view message) noexcept override;
    void installSignalHandlers(ShutdownCallback callback) noexcept override;
    void processSignals() noexcept override;

  private:
    void disarm() noexcept;
    static void signalHandler(int signal) noexcept;

    static volatile std::sig_atomic_t shutdownRequested_;
    static LinuxPlatformInit* owner_;
    ShutdownCallback callback_ = nullptr;
    struct sigaction previousInterrupt_{};
    struct sigaction previousTerminate_{};
    bool delivered_ = false;
};

} // namespace keyflow

#endif
