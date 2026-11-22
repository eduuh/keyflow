#include "platform/linux/LinuxPlatformInit.h"

#ifdef __linux__

#    include <cstdio>

namespace keyflow {

volatile std::sig_atomic_t LinuxPlatformInit::shutdownRequested_ = 0;
LinuxPlatformInit* LinuxPlatformInit::owner_ = nullptr;

LinuxPlatformInit::~LinuxPlatformInit() {
    disarm();
}

void LinuxPlatformInit::showFatalError(std::string_view message) noexcept {
    std::fputs("Keyflow: ", stderr);
    if (!message.empty()) {
        std::fwrite(message.data(), 1, message.size(), stderr);
    }
    std::fputc('\n', stderr);
}

void LinuxPlatformInit::signalHandler(int /*signal*/) noexcept {
    shutdownRequested_ = 1;
}

void LinuxPlatformInit::installSignalHandlers(ShutdownCallback callback) noexcept {
    if (callback == nullptr) {
        disarm();
        return;
    }
    if (owner_ == this) {
        callback_ = callback;
        return;
    }
    if (owner_ != nullptr) {
        showFatalError("shutdown signal handlers already have an owner");
        return;
    }

    struct sigaction action{};
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGTERM);
    shutdownRequested_ = 0;
    if (sigaction(SIGINT, &action, &previousInterrupt_) != 0) {
        showFatalError("cannot install SIGINT shutdown handler");
        return;
    }
    if (sigaction(SIGTERM, &action, &previousTerminate_) != 0) {
        sigaction(SIGINT, &previousInterrupt_, nullptr);
        showFatalError("cannot install SIGTERM shutdown handler");
        return;
    }
    callback_ = callback;
    delivered_ = false;
    owner_ = this;
}

void LinuxPlatformInit::processSignals() noexcept {
    if (owner_ != this || delivered_ || shutdownRequested_ == 0) {
        return;
    }
    delivered_ = true;
    const auto callback = callback_;
    // Do not touch this after delivery: shutdown may destroy the platform adapter.
    if (callback != nullptr) {
        callback();
    }
}

void LinuxPlatformInit::disarm() noexcept {
    callback_ = nullptr;
    if (owner_ != this) {
        return;
    }
    owner_ = nullptr;
    struct sigaction current{};
    if (sigaction(SIGINT, nullptr, &current) == 0 && current.sa_handler == signalHandler) {
        sigaction(SIGINT, &previousInterrupt_, nullptr);
    }
    if (sigaction(SIGTERM, nullptr, &current) == 0 && current.sa_handler == signalHandler) {
        sigaction(SIGTERM, &previousTerminate_, nullptr);
    }
    // Leave the flag alone until the next installation; a concurrent handler can
    // only set it, never reach the destroyed callback.
}

} // namespace keyflow

#endif
