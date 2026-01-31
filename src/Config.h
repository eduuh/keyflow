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
    bool debugMode = false;    // Verbose logging
    bool showAllKeys = false;  // Log every keystroke
    bool showPipeline = false; // Log pipeline decisions
    bool showTimings = false;  // Log performance metrics

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
                showAllKeys = true;
                showPipeline = true;
            } else if (arg == "--verbose" || arg == "-v") {
                debugMode = true;
                showAllKeys = true;
                showPipeline = true;
                showTimings = true;
            } else if (arg == "--keys" || arg == "-k") {
                showAllKeys = true;
            } else if (arg == "--pipeline" || arg == "-p") {
                showPipeline = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp();
            }
        }
    }

    /**
     * @brief Print help message
     */
    static void printHelp() {
        std::cout << "keyflow 🌶️ - Keyboard Remapper\n\n";
        std::cout << "Usage: keyflow [OPTIONS]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -d, --debug      Enable debug mode (shows all logging)\n";
        std::cout << "  -v, --verbose    Enable verbose mode (debug + timings)\n";
        std::cout << "  -k, --keys       Show all keystrokes\n";
        std::cout << "  -p, --pipeline   Show pipeline decisions\n";
        std::cout << "  -h, --help       Show this help message\n\n";
        std::cout << "Runtime Hotkeys:\n";
        std::cout << "  F10              Toggle debug mode\n";
        std::cout << "  F11              Toggle keystroke logging\n";
        std::cout << "  F12              Toggle keyflow on/off\n";
        std::cout << "  Ctrl+C           Exit\n\n";
        std::cout << "Examples:\n";
        std::cout << "  keyflow              Run normally\n";
        std::cout << "  keyflow --debug      Run with debug logging\n";
        std::cout << "  keyflow -v           Run with verbose logging\n\n";
    }
};

} // namespace keyflow
