#pragma once

#include <windows.h>

#include "platform/IPlatform.h"

#include <atomic>
#include <csignal>
#include <cstdlib>

namespace keyflow {

class WindowsPlatformInit final : public IPlatformInit {
  public:
    void hideConsoleIfRelease() noexcept override {
#ifndef DEBUG_BUILD
        HWND console = GetConsoleWindow();
        if (console)
            ShowWindow(console, SW_HIDE);
#else
        // In debug builds, keep console visible
#endif
    }

    void installSignalHandlers(ShutdownCallback callback) noexcept override {
        s_callback.store(callback);
        std::signal(SIGINT, posixSignalHandler);
        std::signal(SIGTERM, posixSignalHandler);
        std::signal(SIGABRT, posixSignalHandler);
        SetConsoleCtrlHandler(consoleHandler, TRUE);
        std::atexit(atexitHandler);
    }

  private:
    // Atomic for safe access from signal handlers and console handler
    static inline std::atomic<ShutdownCallback> s_callback{nullptr};

    static void posixSignalHandler(int /*signal*/) {
        auto cb = s_callback.load();
        if (cb)
            cb();
    }

    static BOOL WINAPI consoleHandler(DWORD signal) {
        if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
            auto cb = s_callback.load();
            if (cb)
                cb();
            ExitProcess(0);
        }
        return TRUE;
    }

    static void atexitHandler() {
        auto cb = s_callback.load();
        if (cb)
            cb();
    }
};

} // namespace keyflow
