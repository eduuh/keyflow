#pragma once

#include <iostream>

/**
 * @brief Debug logging macros that compile out in Release builds
 *
 * Use DEBUG_LOG() for debug output that should only appear in Debug builds.
 * In Release builds, these macros compile to nothing (zero overhead).
 */

#ifdef DEBUG_BUILD
// Debug build - logging enabled
#    define DEBUG_LOG(x)                                                                           \
        do {                                                                                       \
            std::cout << x;                                                                        \
        } while (0)
#    define DEBUG_LOGLN(x)                                                                         \
        do {                                                                                       \
            std::cout << x << "\n";                                                                \
        } while (0)
#else
// Release build - logging compiled out
#    define DEBUG_LOG(x)                                                                           \
        do {                                                                                       \
        } while (0)
#    define DEBUG_LOGLN(x)                                                                         \
        do {                                                                                       \
        } while (0)
#endif
