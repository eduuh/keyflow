#pragma once

#include "platform/IPlatform.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <string>
#include <string_view>
#include <windows.h>

namespace keyflow {

class WindowsPlatformInit final : public IPlatformInit {
  public:
    WindowsPlatformInit() noexcept {
        // Pin CWD to exe directory so config.json, keyboard-icon.ico, and
        // keyflow_debug.log resolve regardless of how the exe was launched.
        // Without this, `keyflow.exe` from any shell or shortcut whose "Start in"
        // isn't set will fail to find config.json.
        wchar_t path[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return;
        }
        std::wstring dir(path, len);
        auto slash = dir.find_last_of(L"\\/");
        if (slash == std::wstring::npos) {
            return;
        }
        dir.resize(slash);
        SetCurrentDirectoryW(dir.c_str());
    }

    void showFatalError(std::string_view message) noexcept override {
        // string_view isn't null-terminated; copy to a std::string for the
        // C API. MB_SETFOREGROUND brings the dialog above other windows so
        // the user actually sees it when launched as a background process.
        std::string text(message);
        MessageBoxA(nullptr, text.c_str(), "Keyflow", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
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
