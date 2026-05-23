#pragma once

#include "../app/DebugLog.h"
#include "../pipeline/Pipeline.h"
#include "../processors/CapsLockBlocker.h"
#include "../processors/ComboAdvanced.h"
#include "../processors/LayerTriggerBlocker.h"
#include "../processors/ModifierTracker.h"
#include "../processors/Rewire.h"
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
     * @param modTrackerOut Optional pointer to receive ModifierTracker pointer
     * @return true if successful, false otherwise
     */
    [[nodiscard]] static bool buildPipeline(const JsonConfig& config, Pipeline& pipeline,
                                            bool verbose = true,
                                            ModifierTracker** modTrackerOut = nullptr) {
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
        ModifierTracker* modTrackerPtr = nullptr;
        if (!config.layers.empty() || !config.noModCombos.empty() ||
            !config.customModifiers.empty()) {
            auto modTracker = std::make_unique<ModifierTracker>();
            modTrackerPtr = modTracker.get(); // Keep pointer for custom modifier registration
            pipeline.addProcessor(std::move(modTracker));
            if (verbose) {
                std::cout << "[Config] Added ModifierTracker\n";
            }
        }

        // Store ModifierTracker pointer if requested
        if (modTrackerOut) {
            *modTrackerOut = modTrackerPtr;
        }

        // Step 2.5: Register custom modifiers
        if (!config.customModifiers.empty() && modTrackerPtr) {
            if (!registerCustomModifiers(config, modTrackerPtr, verbose)) {
                return false;
            }
        }

        // Step 3: Add ComboAdvanced processor if we have combos or layers
        if (!config.noModCombos.empty() || !config.layers.empty()) {
            if (!addComboProcessor(config, pipeline, modTrackerPtr, verbose)) {
                return false;
            }
        }

        // Step 4: Add LayerTriggerBlocker if we have layers or custom modifiers
        if (!config.layers.empty() || !config.customModifiers.empty()) {
            if (!addLayerTriggerBlocker(config, pipeline, verbose)) {
                return false;
            }
        }

        // Step 4.5: Add CapsLockBlocker if disableCapsLock is enabled
        if (config.disableCapsLock) {
            auto blocker = std::make_unique<CapsLockBlocker>();
            pipeline.addProcessor(std::move(blocker));
            if (verbose) {
                std::cout << "[Config] CapsLock blocking enabled\n";
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
     * @brief Register custom modifiers with ModifierTracker
     */
    static bool registerCustomModifiers(const JsonConfig& config, ModifierTracker* modTracker,
                                        bool verbose) {
        if (verbose && !config.customModifiers.empty()) {
            std::cout << "[Config] Custom modifiers:\n";
        }

        for (size_t i = 0; i < config.customModifiers.size(); ++i) {
            const auto& customMod = config.customModifiers[i];

            auto keyScancode = KeyNameMapper::nameToScancode(customMod.key);
            if (!keyScancode) {
                std::cerr << "[Config] ERROR: Unknown key name '" << customMod.key << "'\n";
                return false;
            }

            // Assign custom modifier bit (1-based index, up to 23 custom modifiers)
            ModifierBit modBit = getCustomModifierBit(static_cast<int>(i + 1));
            if (modBit == ModifierBit::None) {
                std::cerr << "[Config] ERROR: Too many custom modifiers (max 23)\n";
                return false;
            }

            modTracker->registerCustomModifier(*keyScancode, customMod.modifierName, modBit);

            if (verbose) {
                std::cout << "  " << customMod.key << " → " << customMod.modifierName;
                if (customMod.blockOutput) {
                    std::cout << " (blocked)";
                }
                std::cout << "\n";
            }
        }

        if (verbose && !config.customModifiers.empty()) {
            std::cout << "\n";
        }

        return true;
    }

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

    // Resolve a trigger name (e.g. "LALT", "SPACE_MOD") to its modifier bit.
    // Checks standard modifiers first, then custom modifiers via the tracker.
    // Returns 0 if unknown — caller should treat that as a config error.
    static uint32_t resolveTriggerBit(const std::string& trigger,
                                      const ModifierTracker* modTracker) noexcept {
        uint32_t modBit = static_cast<uint32_t>(modifierNameToBit(trigger));
        if (modBit != 0) {
            return modBit;
        }
        if (modTracker != nullptr) {
            return static_cast<uint32_t>(modTracker->resolveCustomModifier(trigger));
        }
        return 0;
    }

    static bool appendNoModCombos(ComboAdvanced& combo, const JsonConfig& config, bool verbose) {
        if (config.noModCombos.empty()) {
            return true;
        }
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
            combo.addNoModCombo(*keyScancode, *outputScancode, noModCombo.shift);
            if (verbose) {
                std::cout << "  " << noModCombo.key << " → " << noModCombo.output;
                if (noModCombo.shift) {
                    std::cout << " (with Shift)";
                }
                if (!noModCombo.description.empty()) {
                    std::cout << "  # " << noModCombo.description;
                }
                std::cout << "\n";
            }
        }
        if (verbose) {
            std::cout << "\n";
        }
        return true;
    }

    static bool appendLayerMappings(ComboAdvanced& combo, const Layer& layer,
                                    const ModifierTracker* modTracker, bool verbose) {
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
            for (const auto& trigger : layer.triggers) {
                uint32_t modBit = resolveTriggerBit(trigger, modTracker);
                if (modBit == 0) {
                    std::cerr << "[Config] ERROR: Unknown trigger modifier '" << trigger << "'\n";
                    return false;
                }
                combo.addCombo(modBit, *keyScancode, *targetScancode, true);
            }
            if (verbose) {
                std::cout << "    " << keyName << " → " << targetName << "\n";
            }
        }
        return true;
    }

    static bool appendLayerShiftMappings(ComboAdvanced& combo, const Layer& layer,
                                         const ModifierTracker* modTracker, bool verbose) {
        for (const auto& shiftMapping : layer.shiftMappings) {
            auto keyScancode = KeyNameMapper::nameToScancode(shiftMapping.key);
            auto outputScancode = KeyNameMapper::nameToScancode(shiftMapping.output);
            if (!keyScancode) {
                std::cerr << "[Config] ERROR: Unknown key '" << shiftMapping.key << "'\n";
                return false;
            }
            if (!outputScancode) {
                std::cerr << "[Config] ERROR: Unknown output '" << shiftMapping.output << "'\n";
                return false;
            }
            for (const auto& trigger : layer.triggers) {
                uint32_t modBit = resolveTriggerBit(trigger, modTracker);
                if (modBit == 0) {
                    std::cerr << "[Config] ERROR: Unknown trigger modifier '" << trigger << "'\n";
                    return false;
                }
                // shiftMappings inject Shift with output but don't require Shift input.
                // Enables Layer+Key → Shift+Output (e.g., RALT+T → ^).
                VERBOSE_LOG("[ConfigBuilder] Registering shift combo: modBit=0x"
                            << std::hex << modBit << " triggerKey=0x" << *keyScancode
                            << " outputKey=0x" << *outputScancode << std::dec
                            << " (trigger=" << trigger << " key=" << shiftMapping.key
                            << " output=" << shiftMapping.output << ")\n");
                combo.addComboWithShift(modBit, *keyScancode, *outputScancode, true);
            }
            if (verbose) {
                std::cout << "    " << shiftMapping.key << " → Shift+" << shiftMapping.output;
                if (!shiftMapping.description.empty()) {
                    std::cout << "  # " << shiftMapping.description;
                }
                std::cout << "\n";
            }
        }
        return true;
    }

    static void printLayerHeader(const Layer& layer) {
        std::cout << "  Layer: " << layer.name << "\n    Triggers: ";
        for (size_t i = 0; i < layer.triggers.size(); i++) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << layer.triggers[i];
        }
        std::cout << "\n";
    }

    // Orchestrates the ComboAdvanced setup: noModCombos first, then each
    // layer's regular mappings, then each layer's shift mappings. Returns
    // false on the first config error so the caller can report it cleanly.
    static bool addComboProcessor(const JsonConfig& config, Pipeline& pipeline,
                                  ModifierTracker* modTracker, bool verbose) {
        auto combo = std::make_unique<ComboAdvanced>();

        if (!appendNoModCombos(*combo, config, verbose)) {
            return false;
        }

        if (!config.layers.empty()) {
            if (verbose) {
                std::cout << "[Config] Layers:\n";
            }
            for (const auto& layer : config.layers) {
                if (verbose) {
                    printLayerHeader(layer);
                }
                if (!appendLayerMappings(*combo, layer, modTracker, verbose)) {
                    return false;
                }
                if (!appendLayerShiftMappings(*combo, layer, modTracker, verbose)) {
                    return false;
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
     * @brief Add LayerTriggerBlocker to consume layer trigger keys and custom modifiers
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

        // Add custom modifiers with blockOutput=true
        for (const auto& customMod : config.customModifiers) {
            if (customMod.blockOutput) {
                auto keyScancode = KeyNameMapper::nameToScancode(customMod.key);
                if (keyScancode) {
                    blocker->addTriggerByScancode(*keyScancode);
                }
            }
        }

        if (verbose && blocker->triggerCount() > 0) {
            std::cout << "[Config] Blocking " << blocker->triggerCount()
                      << " layer trigger key(s)\n";
        }

        pipeline.addProcessor(std::move(blocker));
        return true;
    }
};

} // namespace keyflow
