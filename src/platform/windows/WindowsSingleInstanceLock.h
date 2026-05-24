#pragma once

#include "platform/IPlatform.h"

#include <windows.h>

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

    [[nodiscard]] bool acquire() noexcept override {
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

    void release() noexcept override {
        if (mutex_ != nullptr) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = nullptr;
        }
    }

  private:
    HANDLE mutex_ = nullptr;
};

} // namespace keyflow
