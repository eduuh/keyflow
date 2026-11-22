#pragma once

#include "platform/IPlatform.h"

#include <memory>

#ifdef _WIN32
#    include "platform/windows/WindowsHardwareIO.h"
#    include "platform/windows/WindowsPlatformInit.h"
#    include "platform/windows/WindowsSingleInstanceLock.h"
#    include "platform/windows/WindowsSystemTray.h"
#elif defined(__linux__)
#    include "platform/linux/LinuxHardwareIO.h"
#    include "platform/linux/LinuxPlatformInit.h"
#    include "platform/linux/LinuxSingleInstanceLock.h"
#    include "platform/linux/LinuxSystemTray.h"
#endif

namespace keyflow {

/**
 * @brief Compile-time factory for platform-specific implementations.
 *
 * This file is only compiled on platforms with a backend (controlled by
 * KEYFLOW_HAS_PLATFORM_BACKEND in CMakeLists.txt). The #error directives
 * serve as compile-time guards — if a new platform is added to CMake
 * without implementing the corresponding backend, the build fails with
 * a clear message indicating which interface needs implementation.
 */
struct PlatformFactory {
    [[nodiscard]] static std::unique_ptr<IHardwareIO> createHardwareIO() {
#ifdef _WIN32
        return std::make_unique<WindowsHardwareIO>();
#elif defined(__linux__)
        return std::make_unique<LinuxHardwareIO>();
#else
#    error "No HardwareIO backend for this platform"
#endif
    }

    [[nodiscard]] static std::unique_ptr<ISystemTray> createSystemTray() {
#ifdef _WIN32
        return std::make_unique<WindowsSystemTray>();
#elif defined(__linux__)
        return std::make_unique<LinuxSystemTray>();
#else
#    error "No SystemTray backend for this platform"
#endif
    }

    [[nodiscard]] static std::unique_ptr<ISingleInstanceLock> createSingleInstanceLock() {
#ifdef _WIN32
        return std::make_unique<WindowsSingleInstanceLock>();
#elif defined(__linux__)
        return std::make_unique<LinuxSingleInstanceLock>();
#else
#    error "No SingleInstanceLock backend for this platform"
#endif
    }

    [[nodiscard]] static std::unique_ptr<IPlatformInit> createPlatformInit() {
#ifdef _WIN32
        return std::make_unique<WindowsPlatformInit>();
#elif defined(__linux__)
        return std::make_unique<LinuxPlatformInit>();
#else
#    error "No PlatformInit backend for this platform"
#endif
    }
};

} // namespace keyflow
