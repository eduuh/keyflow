#include "Application.h"
#include "DebugLog.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "hardware/Scancodes.h"
#include "platform/PlatformFactory.h"

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace keyflow;

// Global application pointer for signal handlers (atomic for safe signal handler access)
static std::atomic<Application*> g_app{nullptr};

static void shutdownCallback() {
    auto* app = g_app.load();
    if (app) {
        app->releaseAllModifiers();
        app->requestShutdown();
    }
}

int main(int argc, char* argv[]) {
    auto platformInit = PlatformFactory::createPlatformInit();
    platformInit->hideConsoleIfRelease();

#ifdef DEBUG_BUILD
    std::cout << "keyflow - Keyboard Remapper\n\n";
#endif

    // Create platform-specific components
    auto hardware = PlatformFactory::createHardwareIO();
    auto sysTray = PlatformFactory::createSystemTray();
    auto instanceLock = PlatformFactory::createSingleInstanceLock();

    // Create application instance with injected dependencies
    Application app(std::move(hardware), std::move(sysTray));
    g_app = &app;

    app.config().parseArgs(argc, argv);

    bool validateOnly = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--validate") {
            validateOnly = true;
        } else if (std::string(argv[i]) == "--verbose" || std::string(argv[i]) == "-v") {
            g_verbose_logging = true;
            std::cout << "[Main] Verbose logging enabled - writing to keyflow_debug.log\n";
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
            std::cerr << "\n[Config] Validation failed:\n\n";
            for (const auto& [field, message] : validation.errors) {
                std::cerr << "  - " << field << ": " << message << "\n";
            }
            std::cerr << "\nTips:\n";
            std::cerr << "  - Check key names in CONFIG_FORMAT.md\n";
            std::cerr << "  - Key names are case-sensitive (e.g., 'CapsLock' not 'capslock')\n";
            std::cerr << "  - See examples/ directory for working configs\n\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "\n[Config] Failed to load: " << configPath << "\n\n";
        std::cerr << "Error: " << e.what() << "\n\n";

        std::ifstream test(configPath);
        if (!test.good()) {
            std::cerr << "File not found! Make sure config.json exists.\n\n";
            std::cerr << "Quick start:\n";
            std::cerr << "  1. Copy an example: cp examples/minimal.json config.json\n";
            std::cerr << "  2. Or create config.json with:\n";
            std::cerr << "     {\n";
            std::cerr << "       \"version\": \"1.0\",\n";
            std::cerr << "       \"name\": \"My Config\",\n";
            std::cerr << "       \"remapping\": { \"CapsLock\": \"LeftCtrl\" }\n";
            std::cerr << "     }\n\n";
        } else {
            std::cerr << "Common issues:\n";
            std::cerr << "  - Check JSON syntax (missing commas, brackets)\n";
            std::cerr << "  - Ensure 'version' field exists\n";
            std::cerr << "  - Validate at https://jsonlint.com\n\n";
        }
        return 1;
    }

    // If validate-only mode, exit after successful validation
    if (validateOnly) {
        std::cout << "[Config] Validation successful\n";
        std::cout << "[Config] Config name: " << jsonConfig.name << "\n";
        std::cout << "[Config] Remappings: " << jsonConfig.remapping.size() << "\n";
        std::cout << "[Config] NoModCombos: " << jsonConfig.noModCombos.size() << "\n";
        std::cout << "[Config] Layers: " << jsonConfig.layers.size() << "\n";
        return 0;
    }

    platformInit->installSignalHandlers(shutdownCallback);

    // Check for single instance (must be done before hardware initialization)
    if (!instanceLock->acquire()) {
        std::cerr << "[Main] Another instance of Keyflow is already running\n";
        std::cerr << "[Main] Only one instance can run at a time\n";
        g_app = nullptr;
        return 1;
    }

    if (!app.initialize("Keyflow - Keyboard Remapper")) {
        std::cerr << "[Main] Failed to initialize hardware\n";
        std::cerr << "[Main] Make sure:\n";
        std::cerr << "  1. Running as Administrator\n";
        std::cerr << "  2. Interception driver is installed\n";
        g_app = nullptr;
        return 1;
    }

    bool verbose = !app.config().debugMode;
    ModifierTracker* modTracker = nullptr;
    if (!ConfigBuilder::buildPipeline(jsonConfig, app.pipeline(), verbose, &modTracker)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        g_app = nullptr;
        return 1;
    }

    // Store ModifierTracker pointer in Application
    if (modTracker) {
        app.setModifierTracker(modTracker);
    }

#ifdef DEBUG_BUILD
    if (!jsonConfig.name.empty()) {
        std::cout << "[Main] Config: " << jsonConfig.name << "\n";
    }
    std::cout << "[Main] Pipeline: " << app.pipeline().processorCount() << " processors\n";
    std::cout << "[Main] Ctrl+Escape to exit\n\n";
#endif

    while (app.isRunning()) {
        if (!app.sysTray().processMessages()) {
            app.requestShutdown();
            break;
        }

        auto event = app.hardware().waitForKey(10);
        if (!event)
            continue;

        VERBOSE_LOG("[Event] Received key: scancode=0x" << std::hex << event->scancode << std::dec
                                                        << " isDown=" << event->isDown << "\n");

        if (!app.config().keyflowEnabled) {
            app.hardware().sendKey(event->scancode, event->isDown);
            continue;
        }

        // SAFETY: Ensure Windows has processed shift release before new character
        // This prevents shift from being "stuck" when typing very fast after shift injection
        if (event->isDown && modTracker && modTracker->hasInjectedShift()) {
            DEBUG_LOG("[Safety] Injected shift detected before key 0x"
                      << std::hex << event->scancode << std::dec << ", cleaning up\n");
            // Send redundant SHIFT UP unconditionally to guarantee the OS sees it
            // This is safe even if physical shift is held because:
            // 1. Physical shift will send its own DOWN event
            // 2. The OS handles multiple SHIFT UP/DOWN pairs gracefully
            DEBUG_LOG("[Safety] Sending redundant SHIFT UP to prevent stuck shift\n");
            app.hardware().sendKey(SC_LSHIFT, false);
            modTracker->clearInjectedModifiers();
        }

        auto result = app.pipeline().process(*event);

        VERBOSE_LOG("[Pipeline] Result: action=" << static_cast<int>(result.action)
                                                 << " modifiers=0x" << std::hex << result.modifiers
                                                 << std::dec << " outputScancode=0x" << std::hex
                                                 << result.outputScancode << std::dec
                                                 << " injectShift=" << result.injectShift << "\n");

        if (event->isDown && event->scancode == SC_ESCAPE &&
            (result.modifiers & (1 << 2) || result.modifiers & (1 << 3))) {
            DEBUG_LOG("\n[Main] Ctrl+Escape, exiting...\n");
            app.requestShutdown();
            continue;
        }
        switch (result.action) {
            case Action::Forward:
                VERBOSE_LOG("[Action] Forward: scancode=0x" << std::hex << event->scancode
                                                            << std::dec << "\n");
                app.hardware().sendKey(event->scancode, event->isDown);
                break;

            case Action::Replace:
                if (result.injectShift) {
                    if (event->isDown) {
                        DEBUG_LOG("[ShiftInject] Injecting shift for key 0x"
                                  << std::hex << event->scancode << std::dec << " -> 0x" << std::hex
                                  << result.outputScancode << std::dec << "\n");
                        VERBOSE_LOG("[Action] Replace with shift: 0x"
                                    << std::hex << event->scancode << std::dec << " -> SHIFT+0x"
                                    << std::hex << result.outputScancode << std::dec << "\n");
                        VERBOSE_LOG("[Hardware] Sending SHIFT DOWN (0x2A)\n");
                        app.hardware().sendKey(SC_LSHIFT, true);
                        VERBOSE_LOG("[Hardware] Sending KEY DOWN (0x"
                                    << std::hex << result.outputScancode << std::dec << ")\n");
                        app.hardware().sendKey(result.outputScancode, true);
                        if (modTracker) {
                            modTracker->setInjectedShift(true);
                        }
                    } else {
                        DEBUG_LOG("[ShiftInject] Releasing shift for key 0x"
                                  << std::hex << event->scancode << std::dec
                                  << " (tracker NOT cleared yet)\n");
                        VERBOSE_LOG("[Hardware] Sending KEY UP (0x"
                                    << std::hex << result.outputScancode << std::dec << ")\n");
                        app.hardware().sendKey(result.outputScancode, false);
                        VERBOSE_LOG("[Hardware] Sending SHIFT UP (0x2A)\n");
                        app.hardware().sendKey(SC_LSHIFT, false);
                        // DON'T clear tracker yet - let safety check on next key DOWN handle it
                        // This ensures we catch fast typing before the OS processes SHIFT UP
                    }
                } else {
                    VERBOSE_LOG("[Action] Replace: 0x"
                                << std::hex << event->scancode << std::dec << " -> 0x" << std::hex
                                << result.outputScancode << std::dec << "\n");
                    app.hardware().sendKey(result.outputScancode, event->isDown);
                }
                break;

            case Action::Consume:
                VERBOSE_LOG("[Action] Consume: scancode=0x" << std::hex << event->scancode
                                                            << std::dec << "\n");
                break;
        }
    }

    DEBUG_LOG("\n[Main] Shutting down...\n");
    g_app = nullptr;
    // app destructor handles cleanup; instanceLock destructor releases mutex
    return 0;
}
