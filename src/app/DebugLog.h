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

namespace keyflow {

// Toggled by `--verbose` / `-v`.
extern bool gVerboseLogging;

// Persistent log handle. First call opens keyflow_debug.log; subsequent
// calls reuse the open stream.
std::ofstream& verboseLogStream();

// Write `HH:MM:SS.mmm ` to the verbose log stream.
void writeVerboseTimestamp(std::ofstream& out);

} // namespace keyflow

// `x` is intentionally not parenthesized: callers pass chained stream
// expressions like `"foo" << var << bar`, which must compose with the
// outer `<<` rather than be evaluated standalone.
// NOLINTBEGIN(bugprone-macro-parentheses)
#define VERBOSE_LOG(x)                                                                             \
    do {                                                                                           \
        if (keyflow::gVerboseLogging) {                                                            \
            auto& kfLog = keyflow::verboseLogStream();                                             \
            if (kfLog.is_open()) {                                                                 \
                keyflow::writeVerboseTimestamp(kfLog);                                             \
                kfLog << x;                                                                        \
            }                                                                                      \
        }                                                                                          \
    } while (0)
// NOLINTEND(bugprone-macro-parentheses)
