#include "DebugLog.h"
#include "app/Application.h"
#include "config/BehaviorEnumerator.h"
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
        app->requestShutdown();
    }
}

struct CliArgs {
    std::string configPath = "config.json";
    bool debug = false;
    bool verbose = false;
    bool validate = false;
    bool help = false;
    std::vector<std::string> devices;
};

CliArgs parseArgs(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
        if (arg == "--debug" || arg == "-d") {
            args.debug = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--validate") {
            args.validate = true;
        } else if (arg == "--help" || arg == "-h") {
            args.help = true;
        } else if (arg == "--device") {
            ++i;
            if (i >= argc || std::string_view(argv[i]).empty()) {
                throw std::runtime_error("--device requires a keyboard path");
            }
            args.devices.emplace_back(argv[i]);
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
#ifdef _WIN32
        platform.showFatalError("config.json not found.\n\nMake sure config.json sits next to "
                                "keyflow.exe.\n\nPath checked: " +
                                configPath);
#else
        platform.showFatalError("Config not found. Supply an explicit config path: " + configPath);
#endif
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

bool processEvent(Application& app, const KeyEvent& event) {
    if (!event.native && event.isDown && event.scancode == SC_ESCAPE &&
        app.dispatcher().physicalCtrlHeld()) {
        DEBUG_LOG("\n[Main] Ctrl+Escape, exiting...\n");
        app.requestShutdown();
        return true;
    }

    if (!app.dispatcher().process(event)) {
        std::cerr << "[Main] Keyboard output failed: " << app.hardware().lastError() << "\n";
        app.requestShutdown();
        return false;
    }
    return true;
}

bool runEventLoop(Application& app, IPlatformInit& platform) {
    // 50ms timeout: idle wakes happen ~20 Hz (cheap tray pump) while real key
    // events still unblock waitForKey immediately, so typing latency is unaffected.
    constexpr int IDLE_POLL_MS = 50;

    while (app.isRunning()) {
        platform.processSignals();
        if (!app.isRunning()) {
            break;
        }
        if (!app.sysTray().processMessages()) {
            app.requestShutdown();
            break;
        }
        auto event = app.hardware().waitForKey(IDLE_POLL_MS);
        if (app.hardware().hasFailed()) {
            std::cerr << "[Main] Keyboard input failed: " << app.hardware().lastError() << "\n";
            app.requestShutdown();
            break;
        }
        if (!event) {
            // Idle tick — pump tray messages here; on the hot path (a key arrived)
            // we skip this to keep keystroke processing as lean as possible.
            if (!app.sysTray().processMessages()) {
                app.requestShutdown();
                break;
            }
            continue;
        }

        if (!processEvent(app, *event)) {
            return false;
        }
    }
    return !app.hardware().hasFailed();
}

} // namespace

int main(int argc, char* argv[]) {
    auto platformInit = PlatformFactory::createPlatformInit();

    CliArgs args;
    try {
        args = parseArgs(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
    if (args.help) {
        std::cout << "Usage: keyflow [config.json] [--validate] [--debug] [--verbose]\n";
#ifdef __linux__
        std::cout << "Linux runtime requires --device <keyboard-path> (repeatable).\n";
#endif
        return 0;
    }

    if (args.verbose) {
        gVerboseLogging = true;
        std::cout << "[Main] Verbose logging enabled - writing to keyflow_debug.log\n";
    }

    auto config = loadAndValidateConfig(*platformInit, args.configPath);
    if (!config) {
        return 1;
    }

    // Declare the lock first so it remains held through application cleanup.
    auto instanceLock = PlatformFactory::createSingleInstanceLock();
    Application app(PlatformFactory::createHardwareIO(), PlatformFactory::createSystemTray());
    ModifierTracker* modTracker = nullptr;
    if (!ConfigBuilder::buildPipeline(*config, app.pipeline(), false, &modTracker)) {
        std::cerr << "[Main] Failed to build pipeline from config\n";
        return 1;
    }
    if (args.validate) {
        std::cout << "[Config] Valid: " << args.configPath << "\n";
        return 0;
    }
    for (const auto& behavior : enumerateBehaviors(*config)) {
        if (behavior.kind != Behavior::Kind::CustomModifier &&
            behavior.expectedOutputScancode != SC_NOP &&
            !app.hardware().supportsKey(behavior.expectedOutputScancode)) {
            std::cerr << "[Config] Unsupported output: " << behavior.expectedOutputName << "\n";
            return 1;
        }
        if (behavior.expectedInjectShift && !app.hardware().supportsKey(SC_LSHIFT)) {
            std::cerr << "[Config] Backend cannot inject Shift\n";
            return 1;
        }
    }
    if (!app.hardware().configureDevices(args.devices)) {
        std::cerr << "[Main] Invalid keyboard selection: " << app.hardware().lastError() << "\n";
        return 1;
    }
#ifdef __linux__
    for (const auto& layer : config->layers) {
        for (const auto& mapping : layer.shiftMappings) {
            if (!mapping.shift) {
                std::cerr << "[Config] Linux cannot activate shiftMappings with shift=false\n";
                return 1;
            }
        }
    }
#endif
    app.config().debugMode = args.debug;
    gApp = &app;

    platformInit->installSignalHandlers(shutdownCallback);

    if (!instanceLock->acquire()) {
#ifdef _WIN32
        std::cerr << "[Main] Couldn't take over from existing Keyflow instance\n";
        platformInit->showFatalError(
            "Couldn't take over from the existing Keyflow instance.\n\nThe other process may be "
            "running as a different user or with elevated privileges. Try closing it manually.");
#else
        std::cerr
            << "[Main] Unable to acquire instance ownership; another instance may be running\n";
#endif
        gApp = nullptr;
        return 1;
    }

    if (!app.initialize("Keyflow - Keyboard Remapper")) {
#ifdef _WIN32
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
#else
        std::cerr << "[Main] Linux keyboard initialization failed: " << app.hardware().lastError()
                  << "\n";
#endif
        gApp = nullptr;
        return 1;
    }

    platformInit->processSignals();
    if (!app.isRunning() || !app.hardware().activate()) {
        std::cerr << "[Main] Keyboard activation stopped: " << app.hardware().lastError() << "\n";
        gApp = nullptr;
        return 1;
    }
    if (modTracker != nullptr) {
        app.setModifierTracker(modTracker);
    }

    const bool SUCCEEDED = runEventLoop(app, *platformInit);

    DEBUG_LOG("\n[Main] Shutting down...\n");
    gApp = nullptr;
    app.shutdown();
    return SUCCEEDED && !app.hardware().hasFailed() ? 0 : 1;
}
