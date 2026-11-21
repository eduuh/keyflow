#pragma once

#ifdef __linux__

#    include "platform/IPlatform.h"

namespace keyflow {

class LinuxSingleInstanceLock final : public ISingleInstanceLock {
  public:
    LinuxSingleInstanceLock() noexcept = default;
    ~LinuxSingleInstanceLock() override;

    LinuxSingleInstanceLock(const LinuxSingleInstanceLock&) = delete;
    LinuxSingleInstanceLock& operator=(const LinuxSingleInstanceLock&) = delete;
    LinuxSingleInstanceLock(LinuxSingleInstanceLock&&) = delete;
    LinuxSingleInstanceLock& operator=(LinuxSingleInstanceLock&&) = delete;

    [[nodiscard]] bool acquire() noexcept override;
    void release() noexcept override;

  private:
    int descriptor_ = -1;
};

} // namespace keyflow

#endif
