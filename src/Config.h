#pragma once

#include <iostream>
#include <string>

namespace keyflow {

/**
 * @brief Runtime configuration for keyflow
 *
 * Simple configuration struct that can be set via command-line args
 * or modified at runtime.
 */
struct Config {
    // Debug settings
    bool debugMode = false; // Verbose logging during config load

    // Runtime settings
    bool keyflowEnabled = true; // Master on/off switch

    /**
     * @brief Parse command-line arguments
     */
    void parseArgs(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--debug" || arg == "-d") {
                debugMode = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp();
            }
        }
    }

    /**
     * @brief Print help message
     */
    static void printHelp() noexcept {
        std::cout << "keyflow - Keyboard Remapper\n\n";
        std::cout << "Usage: keyflow [OPTIONS] [config.json]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -d, --debug      Enable debug logging\n";
        std::cout << "  --validate       Validate config and exit\n";
        std::cout << "  -h, --help       Show this help message\n\n";
        std::cout << "Controls:\n";
        std::cout << "  Ctrl+Escape      Exit\n\n";
        std::cout << "Examples:\n";
        std::cout << "  keyflow                    Run with config.json\n";
        std::cout << "  keyflow myconfig.json      Run with custom config\n";
        std::cout << "  keyflow --validate         Validate config only\n\n";
    }
};

} // namespace keyflow
