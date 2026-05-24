#include "DebugLog.h"
#include "app/Application.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "hardware/Scancodes.h"
#include "platform/PlatformFactory.h"

#include <atomic>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

using namespace keyflow;

namespace {

// Global application pointer for signal handlers (atomic for safe signal handler access).
std::atomic<Application*> gApp{nullptr};

void shutdownCallback() {
    if (auto* app = gApp.load()) {
        app->releaseAllModifiers();
        app->requestShutdown();
    }
}

struct CliArgs {
    std::string configPath = "config.json";
    bool validateOnly = false;
    bool debug = false;
    bool verbose = false;
};

CliArgs parseArgs(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
        if (arg == "--validate") {
            args.validateOnly = true;
        } else if (arg == "--debug" || arg == "-d") {
            args.debug = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (!arg.empty() && arg[0] != '-' && arg.find(".json") != std::string_view::npos) {
            args.configPath = std::string(arg);
        }
    }
    return args;
}

void printValidationErrors(const ValidationResult& result) {
    std::cerr << "\n[Config] Validation failed:\n\n";
    for (const auto& [field, message] : result.errors) {
        std::cerr << "  - " << field << ": " << message << "\n";
    }
    std::cerr << "\nTips:\n"
                 "  - Check key names in docs/CONFIG_USAGE.md\n"
                 "  - Key names are case-sensitive (e.g., 'CapsLock' not 'capslock')\n\n";
}

void printLoadFailure(const std::string& configPath, const std::exception& e) {
    std::cerr << "\n[Config] Failed to load: " << configPath << "\n\n"
              << "Error: " << e.what() << "\n\n";

    std::ifstream test(configPath);
    if (!test.good()) {
        std::cerr << "File not found! Make sure config.json exists.\n\n"
                     "Quick start — create config.json with:\n"
                     "     {\n"
                     "       \"version\": \"1.0\",\n"
                     "       \"name\": \"My Config\",\n"
                     "       \"remapping\": { \"CapsLock\": \"LeftCtrl\" }\n"
                     "     }\n\n"
                     "See docs/EDUUH_DH.md for the bundled layout's full spec.\n\n";
    } else {
        std::cerr << "Common issues:\n"
                     "  - Check JSON syntax (missing commas, brackets)\n"
                     "  - Ensure 'version' field exists\n"
                     "  - Validate at https://jsonlint.com\n\n";
    }
}

std::optional<JsonConfig> loadAndValidateConfig(const std::string& configPath) {
    try {
        DEBUG_LOG("[Config] Loading: " << configPath << "\n\n");
        JsonConfig config = ConfigLoader::loadFromFile(configPath);
        auto validation = ConfigLoader::validate(config);
        if (!validation) {
            printValidationErrors(validation);
            return std::nullopt;
        }
        return config;
    } catch (const std::exception& e) {
        printLoadFailure(configPath, e);
        return std::nullopt;
    }
}

void printValidateSummary(const JsonConfig& config) {
    std::cout << "[Config] Validation successful\n"
              << "[Config] Config name: " << config.name << "\n"
              << "[Config] Remappings: " << config.remapping.size() << "\n"
              << "[Config] NoModCombos: " << config.noModCombos.size() << "\n"
              << "[Config] Layers: " << config.layers.size() << "\n";
}

// Send replacement key with shift injection. Tracker is updated so the safety
// cleanup on the next key-down can recover from fast typing across shift injections.
void sendReplaceWithShift(Application& app, ModifierTracker* modTracker, uint16_t outputScancode,
                          bool isDown) {
    if (isDown) {
        app.hardware().sendKey(SC_LSHIFT, true);
        app.hardware().sendKey(outputScancode, true);
        if (modTracker != nullptr) {
            modTracker->setInjectedShift(true);
        }
    } else {
        app.hardware().sendKey(outputScancode, false);
        app.hardware().sendKey(SC_LSHIFT, false);
        // Don't clear tracker yet — the safety check on next key DOWN handles it,
        // catching fast typing before the OS processes SHIFT UP.
    }
}

// SAFETY: Ensure Windows has processed shift release before the next character.
// Prevents shift being "stuck" when typing very fast after shift injection.
void cleanupInjectedShift(Application& app, ModifierTracker* modTracker,
                          [[maybe_unused]] uint16_t scancode) {
    if (modTracker == nullptr || !modTracker->hasInjectedShift()) {
        return;
    }
    DEBUG_LOG("[Safety] Injected shift detected before key 0x" << std::hex << scancode << std::dec
                                                               << ", cleaning up\n");
    app.hardware().sendKey(SC_LSHIFT, false);
    modTracker->clearInjectedModifiers();
}

bool isCtrlEscape(uint16_t scancode, uint32_t modifiers) {
    constexpr uint32_t LCTRL_BIT = 1U << 2;
    constexpr uint32_t RCTRL_BIT = 1U << 3;
    return scancode == SC_ESCAPE &&
           (((modifiers & LCTRL_BIT) != 0U) || ((modifiers & RCTRL_BIT) != 0U));
}

void processEvent(Application& app, ModifierTracker* modTracker, const KeyEvent& event) {
    if (event.isDown) {
        cleanupInjectedShift(app, modTracker, event.scancode);
    }

    auto result = app.pipeline().process(event);

    if (event.isDown && isCtrlEscape(event.scancode, result.modifiers)) {
        DEBUG_LOG("\n[Main] Ctrl+Escape, exiting...\n");
        app.requestShutdown();
        return;
    }

    switch (result.action) {
        case Action::Forward:
            app.hardware().sendKey(event.scancode, event.isDown);
            break;
        case Action::Replace:
            if (result.injectShift) {
                sendReplaceWithShift(app, modTracker, result.outputScancode, event.isDown);
            } else {
                app.hardware().sendKey(result.outputScancode, event.isDown);
            }
            break;
        case Action::Consume:
            break;
    }
}

void runEventLoop(Application& app, ModifierTracker* modTracker) {
    // 50ms timeout: idle wakes happen ~20 Hz (cheap tray pump) while real key
    // events still unblock waitForKey immediately, so typing latency is unaffected.
    constexpr int IDLE_POLL_MS = 50;

    while (app.isRunning()) {
        auto event = app.hardware().waitForKey(IDLE_POLL_MS);
        if (!event) {
            // Idle tick — pump tray messages here; on the hot path (a key arrived)
            // we skip this to keep keystroke processing as lean as possible.
            if (!app.sysTray().processMessages()) {
                app.requestShutdown();
                break;
            }
            continue;
        }

        processEvent(app, modTracker, *event);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    auto platformInit = PlatformFactory::createPlatformInit();
    platformInit->hideConsoleIfRelease();

    CliArgs args = parseArgs(argc, argv);
    if (args.verbose) {
        gVerboseLogging = true;
        std::cout << "[Main] Verbose logging enabled - writing to keyflow_debug.log\n";
    }

    auto config = loadAndValidateConfig(args.configPath);
    if (!config) {
        return 1;
    }

    if (args.validateOnly) {
        printValidateSummary(*config);
        return 0;
    }

    Application app(PlatformFactory::createHardwareIO(), PlatformFactory::createSystemTray());
    app.config().debugMode = args.debug;
    gApp = &app;

    platformInit->installSignalHandlers(shutdownCallback);

    auto instanceLock = PlatformFactory::createSingleInstanceLock();
    if (!instanceLock->acquire()) {
        std::cerr << "[Main] Another instance of Keyflow is already running\n"
                     "[Main] Only one instance can run at a time\n";
        gApp = nullptr;
        return 1;
    }

    if (!app.initialize("Keyflow - Keyboard Remapper")) {
        std::cerr << "[Main] Failed to initialize hardware\n"
                     "[Main] Make sure:\n"
                     "  1. Running as Administrator\n"
                     "  2. Interception driver is installed\n";
        gApp = nullptr;
        return 1;
    }

    ModifierTracker* modTracker = nullptr;
    if (!ConfigBuilder::buildPipeline(*config, app.pipeline(), !args.debug, &modTracker)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        gApp = nullptr;
        return 1;
    }
    if (modTracker != nullptr) {
        app.setModifierTracker(modTracker);
    }

    runEventLoop(app, modTracker);

    DEBUG_LOG("\n[Main] Shutting down...\n");
    gApp = nullptr;
    return 0;
}
