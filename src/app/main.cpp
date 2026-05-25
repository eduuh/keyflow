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
#include <sstream>
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
    bool debug = false;
    bool verbose = false;
};

CliArgs parseArgs(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
        if (arg == "--debug" || arg == "-d") {
            args.debug = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (!arg.empty() && arg[0] != '-' && arg.find(".json") != std::string_view::npos) {
            args.configPath = std::string(arg);
        }
    }
    return args;
}

void printValidationErrors(IPlatformInit& platform, const ValidationResult& result) {
    std::cerr << "\n[Config] Validation failed:\n\n";
    for (const auto& [field, message] : result.errors) {
        std::cerr << "  - " << field << ": " << message << "\n";
    }
    std::cerr << "\nTips:\n"
                 "  - Check key names in docs/CONFIG_USAGE.md\n"
                 "  - Key names are case-sensitive (e.g., 'CapsLock' not 'capslock')\n\n";

    // Build a concise MessageBox version for Explorer-launched users.
    std::ostringstream dialog;
    dialog << "Config validation failed:\n\n";
    for (const auto& [field, message] : result.errors) {
        dialog << "  - " << field << ": " << message << "\n";
    }
    platform.showFatalError(dialog.str());
}

void printLoadFailure(IPlatformInit& platform, const std::string& configPath,
                      const std::exception& e) {
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
        platform.showFatalError("config.json not found.\n\nMake sure config.json sits next to "
                                "keyflow.exe.\n\nPath checked: " +
                                configPath);
    } else {
        std::cerr << "Common issues:\n"
                     "  - Check JSON syntax (missing commas, brackets)\n"
                     "  - Ensure 'version' field exists\n"
                     "  - Validate at https://jsonlint.com\n\n";
        platform.showFatalError(std::string("Failed to load config: ") + configPath + "\n\n" +
                                e.what() + "\n\nCheck JSON syntax and the 'version' field.");
    }
}

std::optional<JsonConfig> loadAndValidateConfig(IPlatformInit& platform,
                                                const std::string& configPath) {
    try {
        DEBUG_LOG("[Config] Loading: " << configPath << "\n\n");
        JsonConfig config = ConfigLoader::loadFromFile(configPath);
        auto validation = ConfigLoader::validate(config);
        if (!validation) {
            printValidationErrors(platform, validation);
            return std::nullopt;
        }
        return config;
    } catch (const std::exception& e) {
        printLoadFailure(platform, configPath, e);
        return std::nullopt;
    }
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

    CliArgs args = parseArgs(argc, argv);

    if (args.verbose) {
        gVerboseLogging = true;
        std::cout << "[Main] Verbose logging enabled - writing to keyflow_debug.log\n";
    }

    auto config = loadAndValidateConfig(*platformInit, args.configPath);
    if (!config) {
        return 1;
    }

    Application app(PlatformFactory::createHardwareIO(), PlatformFactory::createSystemTray());
    app.config().debugMode = args.debug;
    gApp = &app;

    platformInit->installSignalHandlers(shutdownCallback);

    auto instanceLock = PlatformFactory::createSingleInstanceLock();
    if (!instanceLock->acquire()) {
        std::cerr << "[Main] Couldn't take over from existing Keyflow instance\n";
        platformInit->showFatalError(
            "Couldn't take over from the existing Keyflow instance.\n\nThe other process may be "
            "running as a different user or with elevated privileges. Try closing it manually.");
        gApp = nullptr;
        return 1;
    }

    if (!app.initialize("Keyflow - Keyboard Remapper")) {
        std::cerr
            << "\n[Main] Hardware init failed — the Interception driver couldn't be opened.\n\n"
               "How to fix:\n"
               "  1. Download Interception from\n"
               "     https://github.com/oblitum/Interception/releases\n"
               "  2. Run install-interception.exe /install (one-time setup)\n"
               "  3. Reboot\n"
               "  4. Re-run keyflow.exe\n\n"
               "If the driver is already installed, check that interception.dll\n"
               "is sitting next to keyflow.exe in the same folder.\n\n";
        platformInit->showFatalError(
            "Hardware init failed — the Interception driver couldn't be opened.\n\n"
            "Install it from:\n"
            "https://github.com/oblitum/Interception/releases\n\n"
            "Run install-interception.exe /install, reboot, then re-run keyflow.exe.\n\n"
            "If already installed, check that interception.dll sits next to keyflow.exe.");
        gApp = nullptr;
        return 1;
    }

    ModifierTracker* modTracker = nullptr;
    if (!ConfigBuilder::buildPipeline(*config, app.pipeline(), !args.debug, &modTracker)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        platformInit->showFatalError(
            "Failed to build pipeline from config.\n\nRun from PowerShell with --debug for "
            "details.");
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
