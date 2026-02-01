#pragma once

#include "Config.h"
#include "SystemTray.h"
#include "hardware/HardwareIO.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"

#include <atomic>
#include <memory>

namespace keyflow {

/**
 * @brief Main application encapsulating keyflow runtime state
 *
 * Encapsulates global state and provides clean lifecycle management.
 * Replaces global variables with RAII pattern.
 */
class Application {
  public:
    Application() = default;
    ~Application() { cleanup(); }

    // Disable copy/move (singleton-like behavior)
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief Initialize the application
     * @return true if successful
     */
    [[nodiscard]] bool initialize(std::string appName) {
        if (!hardware_.initialize()) {
            return false;
        }

        if (!sysTray_.initialize(std::move(appName))) {
            return false;
        }

        return true;
    }

    /**
     * @brief Request graceful shutdown
     */
    void requestShutdown() noexcept { running_ = false; }

    /**
     * @brief Check if application should continue running
     */
    [[nodiscard]] bool isRunning() const noexcept { return running_; }

    /**
     * @brief Get mutable config
     */
    Config& config() noexcept { return config_; }

    /**
     * @brief Get const config
     */
    const Config& config() const noexcept { return config_; }

    /**
     * @brief Get hardware I/O interface
     */
    HardwareIO& hardware() noexcept { return hardware_; }

    /**
     * @brief Get pipeline
     */
    Pipeline& pipeline() noexcept { return pipeline_; }

    /**
     * @brief Get system tray
     */
    SystemTray& sysTray() noexcept { return sysTray_; }

    /**
     * @brief Release all held modifier keys
     *
     * Called during shutdown or emergency cleanup to ensure
     * no keys are left stuck in the down position.
     */
    void releaseAllModifiers() noexcept {
        hardware_.sendKey(SC_LSHIFT, false);
        hardware_.sendKey(SC_RSHIFT, false);
        hardware_.sendKey(SC_LCTRL, false);
        hardware_.sendKey(SC_RCTRL, false);
        hardware_.sendKey(SC_LALT, false);
        hardware_.sendKey(SC_RALT, false);
        hardware_.sendKey(SC_LWIN, false);
        hardware_.sendKey(SC_RWIN, false);
    }

  private:
    void cleanup() noexcept {
        if (hardware_.isInitialized()) {
            releaseAllModifiers();
            hardware_.shutdown();
        }
    }

    Config config_;
    HardwareIO hardware_;
    SystemTray sysTray_;
    Pipeline pipeline_;
    bool running_{true};
};

} // namespace keyflow
