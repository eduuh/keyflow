#pragma once

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
