#pragma once

#include "JsonConfig.h"
#include "KeyNameMapper.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

// For JSON parsing, we'll use nlohmann/json (single header library)
// Download from: https://github.com/nlohmann/json/releases
// Or install via vcpkg: vcpkg install nlohmann-json
#include <nlohmann/json.hpp>

namespace keyflow {

using json = nlohmann::json;

/**
 * @brief Validation result
 */
struct ValidationResult {
    bool valid = true;
    std::vector<std::pair<std::string, std::string>> errors; // field, message

    operator bool() const { return valid; }
};

/**
 * @brief Loads and validates JSON configuration files
 */
class ConfigLoader {
  public:
    /**
     * @brief Load configuration from JSON file
     * @param filepath Path to JSON config file
     * @return JsonConfig structure
     * @throws std::runtime_error if file cannot be loaded or parsed
     */
    [[nodiscard]] static JsonConfig loadFromFile(const std::string& filepath) {
        // Read file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + filepath);
        }

        // Parse JSON
        json j;
        try {
            file >> j;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
        }

        // Convert to JsonConfig
        return parseConfig(j);
    }

    /**
     * @brief Load configuration from JSON string
     * @param jsonStr JSON string
     * @return JsonConfig structure
     */
    [[nodiscard]] static JsonConfig loadFromString(const std::string& jsonStr) {
        json j;
        try {
            j = json::parse(jsonStr);
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
        }

        return parseConfig(j);
    }

    /**
     * @brief Validate configuration
     * @param config The configuration to validate
     * @return ValidationResult with clear error messages
     */
    [[nodiscard]] static ValidationResult validate(const JsonConfig& config) {
        ValidationResult result;
        result.valid = true;

        // Validate remappings
        std::unordered_set<std::string> seenFrom;
        for (const auto& [from, to] : config.remapping) {
            // Skip comment fields
            if (from.find("_comment") == 0)
                continue;

            // Check for duplicates
            if (seenFrom.count(from)) {
                result.errors.emplace_back("remapping",
                                           "Duplicate mapping for key: '" + from + "'");
                result.valid = false;
            }
            seenFrom.insert(from);

            // Validate key names
            if (!KeyNameMapper::nameToScancode(from)) {
                result.errors.emplace_back("remapping." + from, "Unknown key name: '" + from + "'");
                result.valid = false;
            }
            if (!KeyNameMapper::nameToScancode(to)) {
                result.errors.emplace_back("remapping." + from, "Unknown target key: '" + to + "'");
                result.valid = false;
            }
        }

        // Validate noModCombos
        for (size_t i = 0; i < config.noModCombos.size(); ++i) {
            const auto& combo = config.noModCombos[i];
            std::string context = "noModCombos[" + std::to_string(i) + "]";

            if (!KeyNameMapper::nameToScancode(combo.key)) {
                result.errors.emplace_back(context + ".key",
                                           "Unknown key name: '" + combo.key + "'");
                result.valid = false;
            }
            if (!KeyNameMapper::nameToScancode(combo.output)) {
                result.errors.emplace_back(context + ".output",
                                           "Unknown key name: '" + combo.output + "'");
                result.valid = false;
            }
        }

        // Validate customModifiers
        std::unordered_set<std::string> seenModifierNames;
        for (size_t i = 0; i < config.customModifiers.size(); ++i) {
            const auto& customMod = config.customModifiers[i];
            std::string context = "customModifiers[" + std::to_string(i) + "]";

            if (!KeyNameMapper::nameToScancode(customMod.key)) {
                result.errors.emplace_back(context + ".key",
                                           "Unknown key name: '" + customMod.key + "'");
                result.valid = false;
            }

            if (customMod.modifierName.empty()) {
                result.errors.emplace_back(context + ".modifierName",
                                           "Modifier name cannot be empty");
                result.valid = false;
            }

            // Check for duplicate modifier names
            if (seenModifierNames.count(customMod.modifierName)) {
                result.errors.emplace_back(context + ".modifierName", "Duplicate modifier name: '" +
                                                                          customMod.modifierName +
                                                                          "'");
                result.valid = false;
            }
            seenModifierNames.insert(customMod.modifierName);

            // Limit to 23 custom modifiers (bits 9-31)
            if (i >= 23) {
                result.errors.emplace_back(context,
                                           "Maximum 23 custom modifiers allowed (limit reached)");
                result.valid = false;
            }
        }

        // Validate layers
        for (size_t i = 0; i < config.layers.size(); ++i) {
            const auto& layer = config.layers[i];
            std::string layerContext = "layers[" + std::to_string(i) + "]";

            // Validate layer has a name
            if (layer.name.empty()) {
                result.errors.emplace_back(layerContext + ".name", "Layer name cannot be empty");
                result.valid = false;
            }

            // Validate layer has triggers
            if (layer.triggers.empty()) {
                result.errors.emplace_back(layerContext + ".triggers",
                                           "Layer must have at least one trigger");
                result.valid = false;
            }

            // Validate trigger keys
            for (const auto& trigger : layer.triggers) {
                if (!KeyNameMapper::nameToScancode(trigger)) {
                    result.errors.emplace_back(layerContext + ".trigger",
                                               "Unknown key name: '" + trigger + "'");
                    result.valid = false;
                }
            }

            // Validate mappings
            for (const auto& [from, to] : layer.mappings) {
                if (!KeyNameMapper::nameToScancode(from)) {
                    result.errors.emplace_back(layerContext + ".mappings." + from,
                                               "Unknown key name: '" + from + "'");
                    result.valid = false;
                }
                if (!KeyNameMapper::nameToScancode(to)) {
                    result.errors.emplace_back(layerContext + ".mappings." + from,
                                               "Unknown target key: '" + to + "'");
                    result.valid = false;
                }
            }

            // Validate shift mappings
            for (size_t j = 0; j < layer.shiftMappings.size(); ++j) {
                const auto& shiftMapping = layer.shiftMappings[j];
                std::string context = layerContext + ".shiftMappings[" + std::to_string(j) + "]";

                if (!KeyNameMapper::nameToScancode(shiftMapping.key)) {
                    result.errors.emplace_back(context + ".key",
                                               "Unknown key name: '" + shiftMapping.key + "'");
                    result.valid = false;
                }
                if (!KeyNameMapper::nameToScancode(shiftMapping.output)) {
                    result.errors.emplace_back(context + ".output",
                                               "Unknown key name: '" + shiftMapping.output + "'");
                    result.valid = false;
                }
            }
        }

        return result;
    }

  private:
    /**
     * @brief Parse JSON object into JsonConfig
     */
    static JsonConfig parseConfig(const json& j) {
        JsonConfig config;

        // Version (required)
        if (!j.contains("version")) {
            throw std::runtime_error("Config must contain 'version' field");
        }
        config.version = j["version"].get<std::string>();

        // Name (optional)
        if (j.contains("name")) {
            config.name = j["name"].get<std::string>();
        }

        // Remapping
        if (j.contains("remapping")) {
            parseRemapping(j["remapping"], config.remapping);
        }

        // NoMod combos
        if (j.contains("noModCombos")) {
            parseNoModCombos(j["noModCombos"], config.noModCombos);
        }

        // Custom modifiers
        if (j.contains("customModifiers")) {
            parseCustomModifiers(j["customModifiers"], config.customModifiers);
        }

        // Layers
        if (j.contains("layers")) {
            parseLayers(j["layers"], config.layers);
        }

        // Disable caps lock (optional, defaults to false)
        if (j.contains("disableCapsLock")) {
            config.disableCapsLock = j["disableCapsLock"].get<bool>();
        }

        return config;
    }

    /**
     * @brief Parse remapping section
     */
    static void parseRemapping(const json& j,
                               std::unordered_map<std::string, std::string>& remapping) {
        for (auto& [key, value] : j.items()) {
            // Skip comment keys (keys starting with _)
            if (key[0] == '_')
                continue;

            if (value.is_string()) {
                remapping[key] = value.get<std::string>();
            }
        }
    }

    /**
     * @brief Parse noModCombos array
     */
    static void parseNoModCombos(const json& j, std::vector<NoModCombo>& combos) {
        if (!j.is_array()) {
            throw std::runtime_error("noModCombos must be an array");
        }

        for (const auto& item : j) {
            NoModCombo combo;

            if (!item.contains("key") || !item.contains("output")) {
                throw std::runtime_error("noModCombo must have 'key' and 'output' fields");
            }

            combo.key = item["key"].get<std::string>();
            combo.output = item["output"].get<std::string>();

            if (item.contains("shift")) {
                combo.shift = item["shift"].get<bool>();
            }

            if (item.contains("description")) {
                combo.description = item["description"].get<std::string>();
            }

            combos.push_back(combo);
        }
    }

    /**
     * @brief Parse layers array
     */
    static void parseLayers(const json& j, std::vector<Layer>& layers) {
        if (!j.is_array()) {
            throw std::runtime_error("layers must be an array");
        }

        for (const auto& item : j) {
            Layer layer;

            // Name (required)
            if (!item.contains("name")) {
                throw std::runtime_error("Layer must have 'name' field");
            }
            layer.name = item["name"].get<std::string>();

            // Trigger (required) - can be string or array
            if (!item.contains("trigger") && !item.contains("triggers")) {
                throw std::runtime_error("Layer must have 'trigger' or 'triggers' field");
            }

            if (item.contains("trigger")) {
                if (item["trigger"].is_string()) {
                    layer.triggers.push_back(item["trigger"].get<std::string>());
                } else if (item["trigger"].is_array()) {
                    for (const auto& trigger : item["trigger"]) {
                        layer.triggers.push_back(trigger.get<std::string>());
                    }
                }
            } else if (item.contains("triggers")) {
                if (item["triggers"].is_string()) {
                    layer.triggers.push_back(item["triggers"].get<std::string>());
                } else if (item["triggers"].is_array()) {
                    for (const auto& trigger : item["triggers"]) {
                        layer.triggers.push_back(trigger.get<std::string>());
                    }
                }
            }

            // Mappings (required)
            if (!item.contains("mappings")) {
                throw std::runtime_error("Layer must have 'mappings' field");
            }

            for (auto& [key, value] : item["mappings"].items()) {
                if (value.is_string()) {
                    layer.mappings[key] = value.get<std::string>();
                }
            }

            // Shift mappings (optional)
            if (item.contains("shiftMappings")) {
                parseShiftMappings(item["shiftMappings"], layer.shiftMappings);
            }

            layers.push_back(layer);
        }
    }

    /**
     * @brief Parse shift mappings array
     */
    static void parseShiftMappings(const json& j, std::vector<ShiftMapping>& shiftMappings) {
        if (!j.is_array()) {
            throw std::runtime_error("shiftMappings must be an array");
        }

        for (const auto& item : j) {
            ShiftMapping mapping;

            if (!item.contains("key") || !item.contains("output")) {
                throw std::runtime_error("shiftMapping must have 'key' and 'output' fields");
            }

            mapping.key = item["key"].get<std::string>();
            mapping.output = item["output"].get<std::string>();

            if (item.contains("shift")) {
                mapping.shift = item["shift"].get<bool>();
            }

            if (item.contains("description")) {
                mapping.description = item["description"].get<std::string>();
            }

            shiftMappings.push_back(mapping);
        }
    }

    /**
     * @brief Parse customModifiers array
     */
    static void parseCustomModifiers(const json& j, std::vector<CustomModifier>& customModifiers) {
        if (!j.is_array()) {
            throw std::runtime_error("customModifiers must be an array");
        }

        for (const auto& item : j) {
            CustomModifier customMod;

            if (!item.contains("key") || !item.contains("modifierName")) {
                throw std::runtime_error(
                    "customModifier must have 'key' and 'modifierName' fields");
            }

            customMod.key = item["key"].get<std::string>();
            customMod.modifierName = item["modifierName"].get<std::string>();

            if (item.contains("blockOutput")) {
                customMod.blockOutput = item["blockOutput"].get<bool>();
            }

            customModifiers.push_back(customMod);
        }
    }
};

} // namespace keyflow
