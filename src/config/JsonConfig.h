#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace keyflow {

/**
 * @brief No-modifier combo mapping (e.g., number row → symbols)
 */
struct NoModCombo {
    std::string key;         // Source key name
    std::string output;      // Output key name
    bool shift = false;      // Send with shift modifier
    std::string description; // Human-readable description

    NoModCombo() = default;
    NoModCombo(const std::string& k, const std::string& o, bool s = false,
               const std::string& d = "")
        : key(k), output(o), shift(s), description(d) {}
};

/**
 * @brief Shift mapping within a layer (for symbols)
 */
struct ShiftMapping {
    std::string key;         // Source key in layer
    std::string output;      // Output key
    bool shift = true;       // Send with shift (default true)
    std::string description; // Description (e.g., "$ symbol")

    ShiftMapping() = default;
    ShiftMapping(const std::string& k, const std::string& o, bool s = true,
                 const std::string& d = "")
        : key(k), output(o), shift(s), description(d) {}
};

/**
 * @brief Custom modifier definition
 */
struct CustomModifier {
    std::string key;          // Physical key name (e.g., "Space")
    std::string modifierName; // Modifier name for layer triggers (e.g., "SPACE_MOD")
    bool blockOutput = true;  // Block key output when used as modifier

    CustomModifier() = default;
    CustomModifier(const std::string& k, const std::string& m, bool block = true)
        : key(k), modifierName(m), blockOutput(block) {}
};

/**
 * @brief Layer definition with modifier trigger
 */
struct Layer {
    std::string name;                                      // Layer name
    std::vector<std::string> triggers;                     // Trigger modifiers (OR logic)
    std::unordered_map<std::string, std::string> mappings; // Key mappings
    std::vector<ShiftMapping> shiftMappings;               // Shift+key mappings

    Layer() = default;
    Layer(const std::string& n) : name(n) {}
};

/**
 * @brief Complete JSON-based configuration
 */
struct JsonConfig {
    std::string version = "1.0";
    std::string name;

    std::unordered_map<std::string, std::string> remapping;
    std::vector<NoModCombo> noModCombos;
    std::vector<CustomModifier> customModifiers;
    std::vector<Layer> layers;

    // Disable caps lock: Block CapsLock key from reaching OS (prevents accidental toggle)
    bool disableCapsLock = false;

    JsonConfig() = default;
};

} // namespace keyflow
