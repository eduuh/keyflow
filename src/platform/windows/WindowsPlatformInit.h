#pragma once

#include <windows.h>

#include "platform/IPlatform.h"

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
        s_callback = callback;
        std::signal(SIGINT, posixSignalHandler);
        std::signal(SIGTERM, posixSignalHandler);
        std::signal(SIGABRT, posixSignalHandler);
        SetConsoleCtrlHandler(consoleHandler, TRUE);
        std::atexit(atexitHandler);
    }

  private:
    static inline ShutdownCallback s_callback = nullptr;

    static void posixSignalHandler(int /*signal*/) {
        if (s_callback)
            s_callback();
    }

    static BOOL WINAPI consoleHandler(DWORD signal) {
        if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
            if (s_callback)
                s_callback();
            ExitProcess(0);
        }
        return TRUE;
    }

    static void atexitHandler() {
        if (s_callback)
            s_callback();
    }
};

} // namespace keyflow
