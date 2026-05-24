#pragma once

#include "../pipeline/Pipeline.h"
#include "../processors/ComboAdvanced.h"
#include "../processors/LayerTriggerBlocker.h"
#include "../processors/ModifierTracker.h"
#include "../processors/Rewire.h"
#include "../processors/StrictModeFilter.h"
#include "JsonConfig.h"
#include "KeyNameMapper.h"

#include <iostream>
#include <memory>

namespace keyflow {

/**
 * @brief Builds Pipeline from JsonConfig
 */
class ConfigBuilder {
  public:
    /**
     * @brief Build pipeline from JSON configuration
     * @param config JsonConfig structure
     * @param pipeline Pipeline to populate
     * @param verbose Enable verbose logging
     * @return true if successful, false otherwise
     */
    [[nodiscard]] static bool buildPipeline(const JsonConfig& config, Pipeline& pipeline,
                                            bool verbose = true) {
        if (verbose && !config.name.empty()) {
            std::cout << "[Config] Loading: " << config.name << "\n";
        }

        // Step 1: Add Rewire processor if we have remappings
        if (!config.remapping.empty()) {
            if (!addRewireProcessor(config, pipeline, verbose)) {
                return false;
            }
        }

        // Step 2: Add ModifierTracker (needed for layers)
        if (!config.layers.empty() || !config.noModCombos.empty()) {
            auto modTracker = std::make_unique<ModifierTracker>();
            pipeline.addProcessor(std::move(modTracker));
            if (verbose) {
                std::cout << "[Config] Added ModifierTracker\n";
            }
        }

        // Step 3: Add ComboAdvanced processor if we have combos or layers
        if (!config.noModCombos.empty() || !config.layers.empty()) {
            if (!addComboProcessor(config, pipeline, verbose)) {
                return false;
            }
        }

        // Step 4: Add LayerTriggerBlocker if we have layers
        if (!config.layers.empty()) {
            if (!addLayerTriggerBlocker(config, pipeline, verbose)) {
                return false;
            }
        }

        // Step 5: Add StrictModeFilter if enabled (must be LAST in pipeline)
        if (config.strictMode) {
            if (!addStrictModeFilter(config, pipeline, verbose)) {
                return false;
            }
        }

        if (verbose) {
            std::cout << "[Config] Pipeline built with " << pipeline.processorCount()
                      << " processors\n\n";
        }

        return true;
    }

  private:
    /**
     * @brief Add Rewire processor with remappings
     */
    static bool addRewireProcessor(const JsonConfig& config, Pipeline& pipeline, bool verbose) {
        auto rewire = std::make_unique<Rewire>();
        int mappingCount = 0;

        if (verbose) {
            std::cout << "[Config] Key remappings:\n";
        }

        for (const auto& [keyName, targetName] : config.remapping) {
            auto keyScancode = KeyNameMapper::nameToScancode(keyName);
            auto targetScancode = KeyNameMapper::nameToScancode(targetName);

            if (!keyScancode) {
                std::cerr << "[Config] ERROR: Unknown key name '" << keyName << "'\n";
                return false;
            }

            if (!targetScancode) {
                std::cerr << "[Config] ERROR: Unknown target name '" << targetName << "'\n";
                return false;
            }

            rewire->setMapping(*keyScancode, *targetScancode);
            mappingCount++;

            if (verbose) {
                std::cout << "  " << keyName << " → " << targetName << "\n";
            }
        }

        pipeline.addProcessor(std::move(rewire));

        if (verbose) {
            std::cout << "  Total: " << mappingCount << " mappings\n\n";
        }

        return true;
    }

    /**
     * @brief Add ComboAdvanced processor with combos and layers
     */
    static bool addComboProcessor(const JsonConfig& config, Pipeline& pipeline, bool verbose) {
        auto combo = std::make_unique<ComboAdvanced>();

        // Add noModCombos
        if (!config.noModCombos.empty()) {
            if (verbose) {
                std::cout << "[Config] No-modifier combos:\n";
            }

            for (const auto& noModCombo : config.noModCombos) {
                auto keyScancode = KeyNameMapper::nameToScancode(noModCombo.key);
                auto outputScancode = KeyNameMapper::nameToScancode(noModCombo.output);

                if (!keyScancode) {
                    std::cerr << "[Config] ERROR: Unknown key '" << noModCombo.key << "'\n";
                    return false;
                }

                if (!outputScancode) {
                    std::cerr << "[Config] ERROR: Unknown output '" << noModCombo.output << "'\n";
                    return false;
                }

                combo->addNoModCombo(*keyScancode, *outputScancode, noModCombo.shift);

                if (verbose) {
                    std::cout << "  " << noModCombo.key << " → " << noModCombo.output;
                    if (noModCombo.shift)
                        std::cout << " (with Shift)";
                    if (!noModCombo.description.empty()) {
                        std::cout << "  # " << noModCombo.description;
                    }
                    std::cout << "\n";
                }
            }

            if (verbose) {
                std::cout << "\n";
            }
        }

        // Add layers
        if (!config.layers.empty()) {
            if (verbose) {
                std::cout << "[Config] Layers:\n";
            }

            for (const auto& layer : config.layers) {
                if (verbose) {
                    std::cout << "  Layer: " << layer.name << "\n";
                    std::cout << "    Triggers: ";
                    for (size_t i = 0; i < layer.triggers.size(); i++) {
                        if (i > 0)
                            std::cout << ", ";
                        std::cout << layer.triggers[i];
                    }
                    std::cout << "\n";
                }

                // Add regular mappings
                for (const auto& [keyName, targetName] : layer.mappings) {
                    auto keyScancode = KeyNameMapper::nameToScancode(keyName);
                    auto targetScancode = KeyNameMapper::nameToScancode(targetName);

                    if (!keyScancode) {
                        std::cerr << "[Config] ERROR: Unknown key '" << keyName << "'\n";
                        return false;
                    }

                    if (!targetScancode) {
                        std::cerr << "[Config] ERROR: Unknown target '" << targetName << "'\n";
                        return false;
                    }

                    // Add combo for each trigger
                    for (const auto& trigger : layer.triggers) {
                        combo->addCombo(trigger, *keyScancode, *targetScancode);
                    }

                    if (verbose) {
                        std::cout << "    " << keyName << " → " << targetName << "\n";
                    }
                }

                // Add shift mappings
                for (const auto& shiftMapping : layer.shiftMappings) {
                    auto keyScancode = KeyNameMapper::nameToScancode(shiftMapping.key);
                    auto outputScancode = KeyNameMapper::nameToScancode(shiftMapping.output);

                    if (!keyScancode) {
                        std::cerr << "[Config] ERROR: Unknown key '" << shiftMapping.key << "'\n";
                        return false;
                    }

                    if (!outputScancode) {
                        std::cerr << "[Config] ERROR: Unknown output '" << shiftMapping.output
                                  << "'\n";
                        return false;
                    }

                    // Add shift combo for each trigger
                    for (const auto& trigger : layer.triggers) {
                        combo->addComboWithShift(trigger, *keyScancode, *outputScancode);
                    }

                    if (verbose) {
                        std::cout << "    " << shiftMapping.key << " → Shift+"
                                  << shiftMapping.output;
                        if (!shiftMapping.description.empty()) {
                            std::cout << "  # " << shiftMapping.description;
                        }
                        std::cout << "\n";
                    }
                }

                if (verbose) {
                    std::cout << "\n";
                }
            }
        }

        pipeline.addProcessor(std::move(combo));
        return true;
    }

    /**
     * @brief Add LayerTriggerBlocker to consume layer trigger keys
     */
    static bool addLayerTriggerBlocker(const JsonConfig& config, Pipeline& pipeline, bool verbose) {
        auto blocker = std::make_unique<LayerTriggerBlocker>();

        // Collect all unique trigger keys from all layers
        // Block ALL layer triggers so they don't reach Windows
        std::vector<std::string> allTriggers;
        for (const auto& layer : config.layers) {
            for (const auto& trigger : layer.triggers) {
                // Check if not already added
                bool found = false;
                for (const auto& existing : allTriggers) {
                    if (existing == trigger) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    allTriggers.push_back(trigger);
                }
            }
        }

        // Add each trigger to the blocker
        for (const auto& trigger : allTriggers) {
            blocker->addTrigger(trigger);
        }

        if (verbose && blocker->triggerCount() > 0) {
            std::cout << "[Config] Blocking " << blocker->triggerCount()
                      << " layer trigger key(s)\n";
        }

        pipeline.addProcessor(std::move(blocker));
        return true;
    }

    /**
     * @brief Add StrictModeFilter to block all unmapped keys
     *
     * Collects all explicitly mapped INPUT keys from config and creates a filter
     * that blocks everything else. This ensures only the keys you press (inputs)
     * are allowed, and all OUTPUT keys (what they map to) are blocked.
     */
    static bool addStrictModeFilter(const JsonConfig& config, Pipeline& pipeline, bool verbose) {
        auto filter = std::make_unique<StrictModeFilter>();

        // Collect all allowed INPUT keys from remappings (only keyName, NOT targetName)
        for (const auto& [keyName, targetName] : config.remapping) {
            auto keyScancode = KeyNameMapper::nameToScancode(keyName);

            if (keyScancode) {
                filter->addAllowedKey(*keyScancode);
            }
        }

        // Collect all allowed INPUT keys from noModCombos (only key, NOT output)
        for (const auto& combo : config.noModCombos) {
            auto keyScancode = KeyNameMapper::nameToScancode(combo.key);

            if (keyScancode) {
                filter->addAllowedKey(*keyScancode);
            }
        }

        // Collect all allowed INPUT keys from layers (only keyName, NOT targetName)
        for (const auto& layer : config.layers) {
            // Add layer trigger keys (these are INPUT keys the user presses)
            for (const auto& trigger : layer.triggers) {
                auto triggerScancode = KeyNameMapper::nameToScancode(trigger);
                if (triggerScancode) {
                    filter->addAllowedKey(*triggerScancode);
                }
            }

            // Regular mappings - only add INPUT keys
            for (const auto& [keyName, targetName] : layer.mappings) {
                auto keyScancode = KeyNameMapper::nameToScancode(keyName);

                if (keyScancode) {
                    filter->addAllowedKey(*keyScancode);
                }
            }

            // Shift mappings - only add INPUT keys
            for (const auto& shiftMapping : layer.shiftMappings) {
                auto keyScancode = KeyNameMapper::nameToScancode(shiftMapping.key);

                if (keyScancode) {
                    filter->addAllowedKey(*keyScancode);
                }
            }
        }

        if (verbose) {
            std::cout << "[Config] Strict Mode: Enabled (" << filter->allowedKeyCount()
                      << " allowed keys)\n";
            std::cout << "  All unmapped keys will be blocked\n";
        }

        pipeline.addProcessor(std::move(filter));
        return true;
    }
};

} // namespace keyflow
