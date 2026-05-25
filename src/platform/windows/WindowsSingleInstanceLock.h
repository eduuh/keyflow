#pragma once

#include "platform/IPlatform.h"

#include <string>
// clang-format off
#include <windows.h>
#include <tlhelp32.h>
// clang-format on

namespace keyflow {

class WindowsSingleInstanceLock final : public ISingleInstanceLock {
  public:
    WindowsSingleInstanceLock() = default;
    ~WindowsSingleInstanceLock() override { release(); }

    // Non-copyable, non-movable: owns Windows mutex handle.
    // Managed via std::unique_ptr<ISingleInstanceLock> — moving the pointer suffices.
    WindowsSingleInstanceLock(const WindowsSingleInstanceLock&) = delete;
    WindowsSingleInstanceLock& operator=(const WindowsSingleInstanceLock&) = delete;
    WindowsSingleInstanceLock(WindowsSingleInstanceLock&&) = delete;
    WindowsSingleInstanceLock& operator=(WindowsSingleInstanceLock&&) = delete;

    // Auto-takeover: if another instance holds the mutex, terminate it and retry.
    // Failure means we couldn't take over (rare — permission issue or runaway
    // mutex), in which case main() surfaces a fatal MessageBox.
    [[nodiscard]] bool acquire() noexcept override {
        if (tryCreateMutex()) {
            return true;
        }
        terminateOtherInstances();
        // Brief pause for the OS to release the abandoned mutex after TerminateProcess.
        Sleep(200);
        return tryCreateMutex();
    }

    void release() noexcept override {
        if (mutex_ != nullptr) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = nullptr;
        }
    }

  private:
    HANDLE mutex_ = nullptr;

    // Returns true when we got fresh ownership of the mutex. Returns false when
    // it already existed (another instance) — the caller decides whether to retry.
    bool tryCreateMutex() noexcept {
        // Guard against a double-call leaking the previously held handle.
        // acquire() retries after takeover and would otherwise overwrite mutex_.
        if (mutex_ != nullptr) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = nullptr;
        }
        mutex_ = CreateMutexA(nullptr, TRUE, "Global\\KeyflowSingleInstanceMutex");
        if (mutex_ == nullptr) {
            return false;
        }
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(mutex_);
            mutex_ = nullptr;
            return false;
        }
        return true;
    }

    // Walk the process table and kill any process whose exe basename matches
    // our own. Comparing by basename (not full path) means a renamed copy still
    // takes precedence, and a portable build in a different folder still wins.
    void terminateOtherInstances() noexcept {
        wchar_t selfPath[MAX_PATH];
        DWORD len = GetModuleFileNameW(nullptr, selfPath, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return;
        }
        std::wstring self(selfPath, len);
        auto slash = self.find_last_of(L"\\/");
        std::wstring selfName = (slash != std::wstring::npos) ? self.substr(slash + 1) : self;

        DWORD selfPid = GetCurrentProcessId();
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            return;
        }

        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (pe.th32ProcessID != selfPid && _wcsicmp(pe.szExeFile, selfName.c_str()) == 0) {
                    HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (proc != nullptr) {
                        TerminateProcess(proc, 0);
                        CloseHandle(proc);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
};

} // namespace keyflow
