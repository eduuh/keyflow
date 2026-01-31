#include "Config.h"
#include "DebugLog.h"
#include "SystemTray.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "hardware/HardwareIO.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"

#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <windows.h>

using namespace keyflow;
using keyflow::getScancodeNameOrNull;

// Global state
Config g_config;
volatile bool g_running = true;
HardwareIO* g_hardware = nullptr; // For signal handler access

void signalHandler(int signal) {
    (void)signal; // Unused in Release builds
    DEBUG_LOG("\n[Main] Received signal " << signal << ", shutting down...\n");

    // CRITICAL: Release all modifiers immediately to prevent stuck keys
    if (g_hardware != nullptr) {
        g_hardware->sendKey(SC_LSHIFT, false);
        g_hardware->sendKey(SC_RSHIFT, false);
        g_hardware->sendKey(SC_LCTRL, false);
        g_hardware->sendKey(SC_RCTRL, false);
        g_hardware->sendKey(SC_LALT, false);
        g_hardware->sendKey(SC_RALT, false);
        g_hardware->sendKey(SC_LWIN, false);
        g_hardware->sendKey(SC_RWIN, false);
    }

    g_running = false;
}

/**
 * @brief Release all modifier keys to prevent stuck keys on shutdown
 */
void releaseAllModifiers(HardwareIO& hardware) {
    DEBUG_LOG("[Main] Releasing all modifier keys...\n");

    // Release all modifiers (both left and right)
    hardware.sendKey(SC_LSHIFT, false);
    hardware.sendKey(SC_RSHIFT, false);
    hardware.sendKey(SC_LCTRL, false);
    hardware.sendKey(SC_RCTRL, false);
    hardware.sendKey(SC_LALT, false);
    hardware.sendKey(SC_RALT, false);
    hardware.sendKey(SC_LWIN, false);
    hardware.sendKey(SC_RWIN, false);
}

/**
 * @brief Emergency cleanup on abnormal termination
 */
void emergencyCleanup() {
    if (g_hardware != nullptr) {
        // Release all modifiers without logging (may be called during crash)
        g_hardware->sendKey(SC_LSHIFT, false);
        g_hardware->sendKey(SC_RSHIFT, false);
        g_hardware->sendKey(SC_LCTRL, false);
        g_hardware->sendKey(SC_RCTRL, false);
        g_hardware->sendKey(SC_LALT, false);
        g_hardware->sendKey(SC_RALT, false);
        g_hardware->sendKey(SC_LWIN, false);
        g_hardware->sendKey(SC_RWIN, false);
    }
}

/**
 * @brief Windows console control handler (handles Ctrl+C, close, etc.)
 */
BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
        emergencyCleanup();
        ExitProcess(0);
    }
    return TRUE;
}

int main(int argc, char* argv[]) {
#ifndef DEBUG_BUILD
    // Hide console window in Release builds (runs as system tray app)
    HWND console = GetConsoleWindow();
    if (console) {
        ShowWindow(console, SW_HIDE);
    }
#else
    // Show banner in Debug builds
    std::cout << "==============================================\n";
    std::cout << "  keyflow 🌶️ - Keyboard Remapper\n";
    std::cout << "==============================================\n\n";
#endif

    // Parse command-line arguments
    g_config.parseArgs(argc, argv);

    // Check for validate-only mode
    bool validateOnly = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--validate") {
            validateOnly = true;
            break;
        }
    }

    // Determine config file path
    std::string configPath = "config.json";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] != '-' && arg.find(".json") != std::string::npos) {
            configPath = arg;
            break;
        }
    }

    // Load configuration from JSON
    JsonConfig jsonConfig;
    try {
        DEBUG_LOG("[Config] Loading: " << configPath << "\n\n");
        jsonConfig = ConfigLoader::loadFromFile(configPath);

        // Validate configuration
        auto validation = ConfigLoader::validate(jsonConfig);
        if (!validation) {
            std::cerr << "\n[Config] ❌ Validation failed:\n\n";
            for (const auto& [field, message] : validation.errors) {
                std::cerr << "  • " << field << ": " << message << "\n";
            }
            std::cerr << "\n💡 Tips:\n";
            std::cerr << "  - Check key names in CONFIG_FORMAT.md\n";
            std::cerr << "  - Key names are case-sensitive (e.g., 'CapsLock' not 'capslock')\n";
            std::cerr << "  - See examples/ directory for working configs\n\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[Config] ❌ Failed to load: " << configPath << "\n\n";
        std::cerr << "Error: " << e.what() << "\n\n";

        // Check if file exists
        std::ifstream test(configPath);
        if (!test.good()) {
            std::cerr << "File not found! Make sure config.json exists.\n\n";
            std::cerr << "💡 Quick start:\n";
            std::cerr << "  1. Copy an example: cp examples/minimal.json config.json\n";
            std::cerr << "  2. Or create config.json with:\n";
            std::cerr << "     {\n";
            std::cerr << "       \"version\": \"1.0\",\n";
            std::cerr << "       \"name\": \"My Config\",\n";
            std::cerr << "       \"remapping\": { \"CapsLock\": \"LeftCtrl\" }\n";
            std::cerr << "     }\n\n";
        } else {
            std::cerr << "💡 Common issues:\n";
            std::cerr << "  - Check JSON syntax (missing commas, brackets)\n";
            std::cerr << "  - Ensure 'version' field exists\n";
            std::cerr << "  - Validate at https://jsonlint.com\n\n";
        }
        return 1;
    }

    // If validate-only mode, exit after successful validation
    if (validateOnly) {
        std::cout << "[Config] ✅ Validation successful\n";
        std::cout << "[Config] Config name: " << jsonConfig.name << "\n";
        std::cout << "[Config] Remappings: " << jsonConfig.remapping.size() << "\n";
        std::cout << "[Config] NoModCombos: " << jsonConfig.noModCombos.size() << "\n";
        std::cout << "[Config] Layers: " << jsonConfig.layers.size() << "\n";
        return 0;
    }

    // Setup signal handlers (Unix-style)
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);

    // Setup Windows console handlers
    SetConsoleCtrlHandler(consoleHandler, TRUE);

    // Register emergency cleanup for abnormal termination
    std::atexit(emergencyCleanup);

    // Initialize hardware
    HardwareIO hardware;
    if (!hardware.initialize()) {
        std::cerr << "[Main] Failed to initialize hardware\n";
        std::cerr << "[Main] Make sure:\n";
        std::cerr << "  1. Running as Administrator\n";
        std::cerr << "  2. Interception driver is installed\n";
        return 1;
    }

    // Set global for signal handler (critical for stuck key prevention)
    g_hardware = &hardware;

    // Initialize system tray
    SystemTray sysTray;
    if (!sysTray.initialize("Keyflow - Keyboard Remapper")) {
        std::cerr << "[Main] Failed to initialize system tray\n";
        return 1;
    }

    // Build pipeline from configuration
    Pipeline pipeline;
    bool verbose = !g_config.debugMode; // Show config loading unless in debug mode
    if (!ConfigBuilder::buildPipeline(jsonConfig, pipeline, verbose)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        return 1;
    }

    // Display configuration summary (Debug only)
#ifdef DEBUG_BUILD
    if (!jsonConfig.name.empty()) {
        std::cout << "[Main] Configuration: " << jsonConfig.name << "\n";
    }
    std::cout << "[Main] Pipeline: " << pipeline.processorCount() << " processors\n";
    std::cout << "[Main] Starting main loop...\n\n";

    std::cout << "[Info] Runtime Controls:\n";
    std::cout << "  Ctrl+Escape  - Exit\n\n";
#endif

    // Main processing loop
#ifdef DEBUG_BUILD
    int keystrokeCount = 0;
#endif

    while (g_running) {
        // Process system tray messages
        if (!sysTray.processMessages()) {
            g_running = false;
            break;
        }

        auto event = hardware.waitForKey(2);
        if (!event)
            continue;

#ifdef DEBUG_BUILD
        keystrokeCount++;
#endif

        // If disabled, pass through unchanged
        if (!g_config.keyflowEnabled) {
            hardware.sendKey(event->scancode, event->isDown);
            continue;
        }

        // Process through pipeline
        auto result = pipeline.process(*event);

        // Check for Ctrl+Escape to exit
        if (event->isDown && event->scancode == SC_ESCAPE &&
            (result.modifiers & (1 << 2) || result.modifiers & (1 << 3))) { // LCTRL or RCTRL
            DEBUG_LOG("\n[HOTKEY] Ctrl+Escape detected, exiting...\n");
            g_running = false;
            continue;
        }

        // Log keystroke if enabled (compiled out in Release)
#ifdef DEBUG_BUILD
        if (g_config.showAllKeys) {
            // Get key name or use hex if unknown
            const char* keyName = getScancodeNameOrNull(event->scancode);

            std::cout << "[" << std::setw(5) << keystrokeCount << "] ";

            // Show key name or scancode
            if (keyName) {
                std::cout << std::setw(8) << std::left << keyName << std::right;
            } else {
                std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0')
                          << event->scancode << std::dec << "  ";
            }

            std::cout << " " << (event->isDown ? "DN" : "UP");

            // Show modifier state
            if (result.modifiers != 0) {
                std::cout << " [";
                bool first = true;
                if (result.modifiers & (1 << 4)) {
                    std::cout << (first ? "" : "+") << "LA";
                    first = false;
                }
                if (result.modifiers & (1 << 5)) {
                    std::cout << (first ? "" : "+") << "RA";
                    first = false;
                }
                if (result.modifiers & (1 << 2)) {
                    std::cout << (first ? "" : "+") << "LC";
                    first = false;
                }
                if (result.modifiers & (1 << 3)) {
                    std::cout << (first ? "" : "+") << "RC";
                    first = false;
                }
                if (result.modifiers & (1 << 0)) {
                    std::cout << (first ? "" : "+") << "LS";
                    first = false;
                }
                if (result.modifiers & (1 << 1)) {
                    std::cout << (first ? "" : "+") << "RS";
                    first = false;
                }
                if (result.modifiers & (1 << 6)) {
                    std::cout << (first ? "" : "+") << "LW";
                    first = false;
                }
                if (result.modifiers & (1 << 8)) {
                    std::cout << (first ? "" : "+") << "PR";
                    first = false;
                }
                std::cout << "]";
            }

            // Show pipeline decision
            if (g_config.showPipeline) {
                std::cout << " -> ";
                switch (result.action) {
                    case Action::Forward:
                        std::cout << "Pass";
                        break;
                    case Action::Replace: {
                        const char* outName = getScancodeNameOrNull(result.outputScancode);
                        if (result.injectShift)
                            std::cout << "Shift+";
                        if (outName) {
                            std::cout << outName;
                        } else {
                            std::cout << "0x" << std::hex << result.outputScancode << std::dec;
                        }
                    } break;
                    case Action::Consume:
                        std::cout << "Block";
                        break;
                }
            }

            std::cout << "\n";
        }
#endif // DEBUG_BUILD

        // Execute result
        switch (result.action) {
            case Action::Forward:
                hardware.sendKey(event->scancode, event->isDown);
                break;

            case Action::Replace:
                if (result.injectShift) {
                    // Inject Shift modifier for symbols (!, @, #, etc.)
                    if (event->isDown) {
                        // Key down: Send Shift down, then key down
                        hardware.sendKey(SC_LSHIFT, true);
                        hardware.sendKey(result.outputScancode, true);
                    } else {
                        // Key up: Send key up, then Shift up
                        hardware.sendKey(result.outputScancode, false);
                        hardware.sendKey(SC_LSHIFT, false);
                    }
                } else {
                    // Normal replacement without shift
                    hardware.sendKey(result.outputScancode, event->isDown);
                }
                break;

            case Action::Consume:
                // Don't send anything
                break;
        }
    }

    DEBUG_LOG("\n[Main] Processed " << keystrokeCount << " keystrokes\n");
    DEBUG_LOG("[Main] Shutting down...\n");

    // Release all modifiers to prevent stuck keys
    releaseAllModifiers(hardware);

    // Clear global pointer before hardware destruction
    g_hardware = nullptr;

    hardware.shutdown();
    return 0;
}
