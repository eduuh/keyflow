#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

// Debug logging (compiles to nothing in Release builds)
#ifdef DEBUG_BUILD
#    define DEBUG_LOG(x)                                                                           \
        do {                                                                                       \
            std::cout << x;                                                                        \
        } while (0)
#else
#    define DEBUG_LOG(x)                                                                           \
        do {                                                                                       \
        } while (0)
#endif

// Verbose logging - always enabled, writes to file
// Global flag to control verbose logging
namespace keyflow {
extern bool g_verbose_logging;
}

#define VERBOSE_LOG(x)                                                                             \
    do {                                                                                           \
        if (keyflow::g_verbose_logging) {                                                          \
            std::ofstream logFile("keyflow_debug.log", std::ios::app);                             \
            if (logFile.is_open()) {                                                               \
                auto now = std::chrono::system_clock::now();                                       \
                auto time = std::chrono::system_clock::to_time_t(now);                             \
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(                   \
                              now.time_since_epoch()) %                                            \
                          1000;                                                                    \
                std::tm tm_buf;                                                                    \
                localtime_s(&tm_buf, &time);                                                       \
                logFile << std::put_time(&tm_buf, "%H:%M:%S") << "." << std::setfill('0')          \
                        << std::setw(3) << ms.count() << " " << x;                                 \
                logFile.flush();                                                                   \
            }                                                                                      \
        }                                                                                          \
    } while (0)
