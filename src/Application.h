#pragma once

#include "Config.h"
#include "SystemTray.h"
#include "hardware/HardwareIO.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"

#include <atomic>
#include <memory>
#include <windows.h>

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
 * - Move-only: No copying (hardware resources are unique)
 * - Exception-safe: Cleanup guaranteed even on exceptions
 * - Privacy-first: No logging, no data collection
 *
 * Usage:
 *   Application app;
 *   if (!app.initialize("MyApp")) { return 1; }
 *   while (app.isRunning()) { ... main loop ... }
 *   // Automatic cleanup on scope exit
 */
class Application {
  public:
    Application() = default;
    ~Application() noexcept {
        cleanup();
        releaseSingleInstanceLock();
    }

    // Move-only semantics: Application owns unique hardware resources
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;            // Hardware state not movable
    Application& operator=(Application&&) = delete; // Hardware state not movable

    /**
     * @brief Check if another instance is already running
     * @return true if this is the only instance, false if another instance exists
     */
    [[nodiscard]] bool acquireSingleInstanceLock() noexcept {
        // Create a named mutex that persists across the system
        // Use "Global\\" prefix to work across user sessions
        singleInstanceMutex_ = CreateMutexA(nullptr, TRUE, "Global\\KeyflowSingleInstanceMutex");

        if (singleInstanceMutex_ == nullptr) {
            return false; // Failed to create mutex
        }

        // Check if mutex already existed (another instance is running)
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(singleInstanceMutex_);
            singleInstanceMutex_ = nullptr;
            return false; // Another instance is already running
        }

        return true; // Successfully acquired single-instance lock
    }

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
     * @brief Get modifier tracker
     */
    ModifierTracker& modifierTracker() noexcept { return *modifierTracker_; }

    /**
     * @brief Set modifier tracker pointer (called during pipeline build)
     */
    void setModifierTracker(ModifierTracker* tracker) noexcept { modifierTracker_ = tracker; }

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

    void releaseSingleInstanceLock() noexcept {
        if (singleInstanceMutex_ != nullptr) {
            ReleaseMutex(singleInstanceMutex_);
            CloseHandle(singleInstanceMutex_);
            singleInstanceMutex_ = nullptr;
        }
    }

    Config config_;
    HardwareIO hardware_;
    SystemTray sysTray_;
    Pipeline pipeline_;
    bool running_{true};
    ModifierTracker* modifierTracker_ = nullptr;   // Pointer to ModifierTracker in pipeline
    HANDLE singleInstanceMutex_ = nullptr;         // Mutex for single-instance enforcement
};

} // namespace keyflow
