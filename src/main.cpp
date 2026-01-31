#include "Config.h"
#include "DebugLog.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "hardware/HardwareIO.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"

#include <csignal>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <windows.h>

using namespace keyflow;
using keyflow::getScancodeNameOrNull;

// Global state
Config g_config;
volatile bool g_running = true;

void signalHandler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down...\n";
    g_running = false;
}

/**
 * @brief Release all modifier keys to prevent stuck keys on shutdown
 */
void releaseAllModifiers(HardwareIO& hardware) {
    std::cout << "[Main] Releasing all modifier keys...\n";

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

int main(int argc, char* argv[]) {
    std::cout << "==============================================\n";
    std::cout << "  keyflow 🌶️ - Keyboard Remapper\n";
    std::cout << "==============================================\n\n";

    // Parse command-line arguments
    g_config.parseArgs(argc, argv);

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
        std::cout << "[Config] Loading: " << configPath << "\n\n";
        jsonConfig = ConfigLoader::loadFromFile(configPath);

        // Apply debug settings from JSON
        if (jsonConfig.debug.debugMode) {
            g_config.debugMode = true;
            g_config.showAllKeys = jsonConfig.debug.showAllKeys;
            g_config.showPipeline = jsonConfig.debug.showPipeline;
            g_config.showTimings = jsonConfig.debug.showTimings;
        }

    } catch (const std::exception& e) {
        std::cerr << "[Config] ERROR: " << e.what() << "\n";
        std::cerr << "[Config] Failed to load config file: " << configPath << "\n\n";
        std::cerr << "Make sure:\n";
        std::cerr << "  1. config.json exists in the current directory\n";
        std::cerr << "  2. JSON syntax is valid\n";
        std::cerr << "  3. All key names are recognized\n\n";
        return 1;
    }

    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Initialize hardware
    HardwareIO hardware;
    if (!hardware.initialize()) {
        std::cerr << "[Main] Failed to initialize hardware\n";
        std::cerr << "[Main] Make sure:\n";
        std::cerr << "  1. Running as Administrator\n";
        std::cerr << "  2. Interception driver is installed\n";
        return 1;
    }

    // Build pipeline from configuration
    Pipeline pipeline;
    bool verbose = !g_config.debugMode; // Show config loading unless in debug mode
    if (!ConfigBuilder::buildPipeline(jsonConfig, pipeline, verbose)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        return 1;
    }

    // Display configuration summary
    if (!jsonConfig.name.empty()) {
        std::cout << "[Main] Configuration: " << jsonConfig.name << "\n";
    }
    std::cout << "[Main] Pipeline: " << pipeline.processorCount() << " processors\n";
    std::cout << "[Main] Starting main loop...\n\n";

    std::cout << "[Info] Runtime Controls:\n";
    std::cout << "  Ctrl+Escape  - Exit\n\n";

    // Main processing loop
    int keystrokeCount = 0;

    while (g_running) {
        auto event = hardware.waitForKey(2);
        if (!event)
            continue;

        keystrokeCount++;

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
            std::cout << "\n[HOTKEY] Ctrl+Escape detected, exiting...\n";
            g_running = false;
            continue;
        }

        // Log keystroke if enabled (combined with pipeline result)
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

    std::cout << "\n[Main] Processed " << keystrokeCount << " keystrokes\n";
    std::cout << "[Main] Shutting down...\n";

    // Release all modifiers to prevent stuck keys
    releaseAllModifiers(hardware);

    hardware.shutdown();
    return 0;
}
