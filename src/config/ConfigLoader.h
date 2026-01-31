#pragma once

#include "JsonConfig.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

// For JSON parsing, we'll use nlohmann/json (single header library)
// Download from: https://github.com/nlohmann/json/releases
// Or install via vcpkg: vcpkg install nlohmann-json
#include <nlohmann/json.hpp>

namespace keyflow {

using json = nlohmann::json;

/**
 * @brief Loads JSON configuration files
 */
class ConfigLoader {
  public:
    /**
     * @brief Load configuration from JSON file
     * @param filepath Path to JSON config file
     * @return JsonConfig structure
     * @throws std::runtime_error if file cannot be loaded or parsed
     */
    static JsonConfig loadFromFile(const std::string& filepath) {
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
    static JsonConfig loadFromString(const std::string& jsonStr) {
        json j;
        try {
            j = json::parse(jsonStr);
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
        }

        return parseConfig(j);
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

        // Layers
        if (j.contains("layers")) {
            parseLayers(j["layers"], config.layers);
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
};

} // namespace keyflow
