#pragma once

namespace keyflow {

/**
 * @brief Runtime flags for keyflow
 *
 * Populated from CLI args by main(). Per-keyboard configuration lives in JsonConfig.
 */
struct Config {
    bool debugMode = false;
};

} // namespace keyflow
