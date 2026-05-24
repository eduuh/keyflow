#pragma once

#include "app/Config.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"
#include "platform/IPlatform.h"

#include <memory>

namespace keyflow {

// Forward declaration
class ModifierTracker;

/**
 * @brief Main application encapsulating keyflow runtime state
 *
 * Encapsulates global state and provides clean lifecycle management.
 * Replaces global variables with RAII pattern.
 *
 * Design:
 * - RAII: Automatic cleanup on destruction
 * - Non-copyable, non-movable: Owns unique hardware resources via unique_ptr;
 *   not movable because external code (signal handlers) holds raw pointer
 * - Exception-safe: Cleanup guaranteed even on exceptions
 * - Privacy-first: No logging, no data collection
 * - Platform-agnostic: Uses IHardwareIO and ISystemTray interfaces
 *
 * Usage:
 *   auto hw = PlatformFactory::createHardwareIO();
 *   auto tray = PlatformFactory::createSystemTray();
 *   Application app(std::move(hw), std::move(tray));
 *   if (!app.initialize("MyApp")) { return 1; }
 *   while (app.isRunning()) { ... main loop ... }
 */
class Application {
  public:
    Application(std::unique_ptr<IHardwareIO> hardware, std::unique_ptr<ISystemTray> sysTray)
        : hardware_(std::move(hardware)), sysTray_(std::move(sysTray)) {}

    ~Application() noexcept { cleanup(); }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;            // Not movable: signal handlers hold raw pointer
    Application& operator=(Application&&) = delete; // Not movable: signal handlers hold raw pointer

    /**
     * @brief Initialize the application
     * @return true if successful
     */
    [[nodiscard]] bool initialize(std::string appName) {
        if (!hardware_->initialize()) {
            return false;
        }

        if (!sysTray_->initialize(std::move(appName))) {
            return false;
        }

        return true;
    }

    void requestShutdown() noexcept { running_ = false; }

    [[nodiscard]] bool isRunning() const noexcept { return running_; }

    Config& config() noexcept { return config_; }
    const Config& config() const noexcept { return config_; }

    IHardwareIO& hardware() noexcept { return *hardware_; }
    Pipeline& pipeline() noexcept { return pipeline_; }
    ISystemTray& sysTray() noexcept { return *sysTray_; }

    ModifierTracker& modifierTracker() noexcept { return *modifierTracker_; }
    void setModifierTracker(ModifierTracker* tracker) noexcept { modifierTracker_ = tracker; }

    /**
     * @brief Release all held modifier keys
     *
     * Called during shutdown or emergency cleanup to ensure
     * no keys are left stuck in the down position.
     */
    void releaseAllModifiers() noexcept {
        hardware_->sendKey(SC_LSHIFT, false);
        hardware_->sendKey(SC_RSHIFT, false);
        hardware_->sendKey(SC_LCTRL, false);
        hardware_->sendKey(SC_RCTRL, false);
        hardware_->sendKey(SC_LALT, false);
        hardware_->sendKey(SC_RALT, false);
        hardware_->sendKey(SC_LWIN, false);
        hardware_->sendKey(SC_RWIN, false);
    }

  private:
    void cleanup() noexcept {
        if (hardware_ && hardware_->isInitialized()) {
            releaseAllModifiers();
            hardware_->shutdown();
        }
    }

    Config config_;
    std::unique_ptr<IHardwareIO> hardware_;
    std::unique_ptr<ISystemTray> sysTray_;
    Pipeline pipeline_;
    bool running_{true};
    ModifierTracker* modifierTracker_ = nullptr;
};

} // namespace keyflow
