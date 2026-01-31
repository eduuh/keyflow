#include "Config.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "hardware/HardwareIO.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"

#include <csignal>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace keyflow;

// Global state
Config g_config;
volatile bool g_running = true;

void signalHandler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down...\n";
    g_running = false;
}

bool handleHotkey(const KeyEvent& event) {
    if (!event.isDown)
        return false;

    if (event.scancode == 0x44) { // F10 - Toggle debug
        g_config.debugMode = !g_config.debugMode;
        g_config.showAllKeys = g_config.debugMode;
        g_config.showPipeline = g_config.debugMode;
        std::cout << "\n[HOTKEY] Debug mode: " << (g_config.debugMode ? "ON" : "OFF") << "\n\n";
        return true;
    }

    if (event.scancode == 0x57) { // F11 - Toggle keystroke logging
        g_config.showAllKeys = !g_config.showAllKeys;
        std::cout << "\n[HOTKEY] Keystroke logging: " << (g_config.showAllKeys ? "ON" : "OFF")
                  << "\n\n";
        return true;
    }

    if (event.scancode == 0x58) { // F12 - Toggle on/off
        g_config.keyflowEnabled = !g_config.keyflowEnabled;
        std::cout << "\n[HOTKEY] keyflow: " << (g_config.keyflowEnabled ? "ENABLED" : "DISABLED")
                  << " 🌶️\n\n";
        return true;
    }

    return false;
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
    std::cout << "  F10  - Toggle debug mode\n";
    std::cout << "  F11  - Toggle keystroke logging\n";
    std::cout << "  F12  - Toggle keyflow on/off\n";
    std::cout << "  Ctrl+C - Exit\n\n";

    // Main processing loop
    int keystrokeCount = 0;

    while (g_running) {
        auto event = hardware.waitForKey(2);
        if (!event)
            continue;

        keystrokeCount++;

        // Handle hotkeys first
        if (handleHotkey(*event))
            continue;

        // If disabled, pass through unchanged
        if (!g_config.keyflowEnabled) {
            hardware.sendKey(event->scancode, event->isDown);
            continue;
        }

        // Log keystroke if enabled
        if (g_config.showAllKeys) {
            std::cout << "[" << std::setw(8) << keystrokeCount << "] "
                      << "0x" << std::hex << std::setw(4) << std::setfill('0') << event->scancode
                      << std::dec << " " << (event->isDown ? "DOWN" : "UP  ") << "\n";
        }

        // Process through pipeline
        auto result = pipeline.process(*event);

        // Show pipeline decision if enabled
        if (g_config.showPipeline) {
            std::cout << "         -> ";
            switch (result.action) {
                case Action::Forward:
                    std::cout << "Forward (unchanged)";
                    break;
                case Action::Replace:
                    std::cout << "Replace: 0x" << std::hex << std::setw(4) << std::setfill('0')
                              << result.outputScancode << std::dec;
                    break;
                case Action::Consume:
                    std::cout << "Consume (blocked)";
                    break;
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

    hardware.shutdown();
    return 0;
}
