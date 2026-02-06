#pragma once

#include "platform/IPlatform.h"

#include <memory>

#ifdef _WIN32
#    include "platform/windows/WindowsHardwareIO.h"
#    include "platform/windows/WindowsPlatformInit.h"
#    include "platform/windows/WindowsSingleInstanceLock.h"
#    include "platform/windows/WindowsSystemTray.h"
#endif

namespace keyflow {

struct PlatformFactory {
    [[nodiscard]] static std::unique_ptr<IHardwareIO> createHardwareIO() {
#ifdef _WIN32
        return std::make_unique<WindowsHardwareIO>();
#else
#    error "No HardwareIO backend for this platform"
#endif
    }

    [[nodiscard]] static std::unique_ptr<ISystemTray> createSystemTray() {
#ifdef _WIN32
        return std::make_unique<WindowsSystemTray>();
#else
#    error "No SystemTray backend for this platform"
#endif
    }

    [[nodiscard]] static std::unique_ptr<ISingleInstanceLock> createSingleInstanceLock() {
#ifdef _WIN32
        return std::make_unique<WindowsSingleInstanceLock>();
#else
#    error "No SingleInstanceLock backend for this platform"
#endif
    }

    [[nodiscard]] static std::unique_ptr<IPlatformInit> createPlatformInit() {
#ifdef _WIN32
        return std::make_unique<WindowsPlatformInit>();
#else
#    error "No PlatformInit backend for this platform"
#endif
    }
};

} // namespace keyflow
