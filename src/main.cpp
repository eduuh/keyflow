#include "Config.h"
#include "DebugLog.h"
#include "SystemTray.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "hardware/HardwareIO.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"

#include <csignal>
#include <iostream>
#include <stdexcept>
#include <windows.h>

using namespace keyflow;

Config g_config;
volatile bool g_running = true;
HardwareIO* g_hardware = nullptr;

void releaseAllModifiers(HardwareIO* hardware) {
    if (hardware) {
        hardware->sendKey(SC_LSHIFT, false);
        hardware->sendKey(SC_RSHIFT, false);
        hardware->sendKey(SC_LCTRL, false);
        hardware->sendKey(SC_RCTRL, false);
        hardware->sendKey(SC_LALT, false);
        hardware->sendKey(SC_RALT, false);
        hardware->sendKey(SC_LWIN, false);
        hardware->sendKey(SC_RWIN, false);
    }
}

void signalHandler(int signal) {
    (void)signal;
    DEBUG_LOG("\n[Main] Signal received, shutting down...\n");
    releaseAllModifiers(g_hardware);
    g_running = false;
}

void emergencyCleanup() {
    releaseAllModifiers(g_hardware);
}

BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
        emergencyCleanup();
        ExitProcess(0);
    }
    return TRUE;
}

int main(int argc, char* argv[]) {
#ifndef DEBUG_BUILD
    HWND console = GetConsoleWindow();
    if (console)
        ShowWindow(console, SW_HIDE);
#else
    std::cout << "keyflow - Keyboard Remapper\n\n";
#endif

    g_config.parseArgs(argc, argv);

    bool validateOnly = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--validate") {
            validateOnly = true;
            break;
        }
    }

    std::string configPath = "config.json";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] != '-' && arg.find(".json") != std::string::npos) {
            configPath = arg;
            break;
        }
    }

    JsonConfig jsonConfig;
    try {
        DEBUG_LOG("[Config] Loading: " << configPath << "\n\n");
        jsonConfig = ConfigLoader::loadFromFile(configPath);

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

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
    SetConsoleCtrlHandler(consoleHandler, TRUE);
    std::atexit(emergencyCleanup);

    HardwareIO hardware;
    if (!hardware.initialize()) {
        std::cerr << "[Main] Failed to initialize hardware\n";
        std::cerr << "[Main] Make sure:\n";
        std::cerr << "  1. Running as Administrator\n";
        std::cerr << "  2. Interception driver is installed\n";
        return 1;
    }

    g_hardware = &hardware;

    SystemTray sysTray;
    if (!sysTray.initialize("Keyflow - Keyboard Remapper")) {
        std::cerr << "[Main] Failed to initialize system tray\n";
        return 1;
    }

    Pipeline pipeline;
    bool verbose = !g_config.debugMode;
    if (!ConfigBuilder::buildPipeline(jsonConfig, pipeline, verbose)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        return 1;
    }

#ifdef DEBUG_BUILD
    if (!jsonConfig.name.empty()) {
        std::cout << "[Main] Config: " << jsonConfig.name << "\n";
    }
    std::cout << "[Main] Pipeline: " << pipeline.processorCount() << " processors\n";
    std::cout << "[Main] Ctrl+Escape to exit\n\n";
#endif

    while (g_running) {
        if (!sysTray.processMessages()) {
            g_running = false;
            break;
        }

        auto event = hardware.waitForKey(2);
        if (!event)
            continue;

        if (!g_config.keyflowEnabled) {
            hardware.sendKey(event->scancode, event->isDown);
            continue;
        }

        auto result = pipeline.process(*event);

        if (event->isDown && event->scancode == SC_ESCAPE &&
            (result.modifiers & (1 << 2) || result.modifiers & (1 << 3))) {
            DEBUG_LOG("\n[Main] Ctrl+Escape, exiting...\n");
            g_running = false;
            continue;
        }
        switch (result.action) {
            case Action::Forward:
                hardware.sendKey(event->scancode, event->isDown);
                break;

            case Action::Replace:
                if (result.injectShift) {
                    if (event->isDown) {
                        hardware.sendKey(SC_LSHIFT, true);
                        hardware.sendKey(result.outputScancode, true);
                    } else {
                        hardware.sendKey(result.outputScancode, false);
                        hardware.sendKey(SC_LSHIFT, false);
                    }
                } else {
                    hardware.sendKey(result.outputScancode, event->isDown);
                }
                break;

            case Action::Consume:
                break;
        }
    }

    DEBUG_LOG("\n[Main] Shutting down...\n");
    releaseAllModifiers(&hardware);
    g_hardware = nullptr;

    hardware.shutdown();
    return 0;
}
