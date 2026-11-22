#pragma once

#ifdef __linux__

#    include "platform/IPlatform.h"

namespace keyflow {

// Linux runs in the foreground; no display server or desktop service is needed.
class LinuxSystemTray final : public ISystemTray {
  public:
    [[nodiscard]] bool initialize(std::string /*appName*/) override { return true; }
    void showNotification(std::string_view /*title*/,
                          std::string_view /*message*/) noexcept override {}
    [[nodiscard]] bool processMessages() noexcept override { return true; }
};

} // namespace keyflow

#endif
