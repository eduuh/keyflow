#pragma once

#include "../pipeline/Modifiers.h"
#include "JsonConfig.h"
#include "KeyNameMapper.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace keyflow {

// A Behavior is one observable input → output rule the config promises.
// The test suite walks these, drives each through the real Pipeline, and
// asserts the outcome matches. The config IS the spec.
struct Behavior {
    enum class Kind {
        Remap,             // remapping[K] = K' where K' is a non-modifier key
        ModifierRemap,     // remapping[K] = K' where K' is a modifier key
        NoModCombo,        // noModCombos[i]
        LayerMapping,      // layer.mappings[K] (per trigger)
        LayerShiftMapping, // layer.shiftMappings[i] (per trigger)
        CustomModifier,    // customModifiers[i]
    };

    std::string id; // Stable identifier, e.g. "remap:E"
    Kind kind;
    std::string physicalKeyName; // What the user presses
    uint16_t physicalScancode;
    std::string expectedOutputName; // For Remap/NoModCombo/Layer* kinds
    uint16_t expectedOutputScancode;
    bool expectedInjectShift = false;           // True for shifted combos / shiftMappings
    std::optional<std::string> triggerModifier; // "LALT", "RALT", "SPACE_MOD", etc.
    uint32_t triggerModifierBit = 0;            // Resolved bit; 0 if no trigger
    bool blockOutput = false;                   // For CustomModifier kind
    std::string layerName;                      // For Layer* kinds
    std::string configName;                     // Which config this came from
};

// Resolves a trigger name to a modifier bit, consulting both standard modifiers
// (LALT/RALT/etc.) and custom modifiers from the config. Custom modifier bits
// are assigned in declaration order (Custom1, Custom2, ...) — same as
// ConfigBuilder::registerCustomModifiers at src/config/ConfigBuilder.h:118-134.
inline uint32_t resolveTriggerBit(const std::string& triggerName, const JsonConfig& config) {
    uint32_t standardBit = static_cast<uint32_t>(modifierNameToBit(triggerName));
    if (standardBit != 0) {
        return standardBit;
    }
    for (size_t i = 0; i < config.customModifiers.size(); ++i) {
        if (config.customModifiers[i].modifierName == triggerName) {
            return static_cast<uint32_t>(getCustomModifierBit(static_cast<int>(i + 1)));
        }
    }
    return 0;
}

// Walks a config and produces one Behavior per observable input→output rule.
// One mapping with N triggers produces N behaviors (one per trigger).
//
// Note: malformed entries (unknown key names) are skipped — config validation
// is ConfigLoader::validate's job, not the enumerator's. Callers should
// validate before enumerating.
inline std::vector<Behavior> enumerateBehaviors(const JsonConfig& config,
                                                const std::string& configName = "") {
    std::vector<Behavior> out;

    // 1) Remapping — split into Remap vs ModifierRemap by target scancode.
    for (const auto& [from, to] : config.remapping) {
        auto fromSc = KeyNameMapper::nameToScancode(from);
        auto toSc = KeyNameMapper::nameToScancode(to);
        if (!fromSc || !toSc) {
            continue;
        }
        Behavior b;
        b.kind = isModifierKey(*toSc) ? Behavior::Kind::ModifierRemap : Behavior::Kind::Remap;
        b.id = (b.kind == Behavior::Kind::ModifierRemap ? "modRemap:" : "remap:") + from;
        b.physicalKeyName = from;
        b.physicalScancode = *fromSc;
        b.expectedOutputName = to;
        b.expectedOutputScancode = *toSc;
        b.configName = configName;
        out.push_back(std::move(b));
    }

    // 2) NoModCombos — combo fires only when no modifiers held; may inject shift.
    for (const auto& combo : config.noModCombos) {
        auto fromSc = KeyNameMapper::nameToScancode(combo.key);
        auto toSc = KeyNameMapper::nameToScancode(combo.output);
        if (!fromSc || !toSc) {
            continue;
        }
        Behavior b;
        b.kind = Behavior::Kind::NoModCombo;
        b.id = "noModCombo:" + combo.key;
        b.physicalKeyName = combo.key;
        b.physicalScancode = *fromSc;
        b.expectedOutputName = combo.output;
        b.expectedOutputScancode = *toSc;
        b.expectedInjectShift = combo.shift;
        b.configName = configName;
        out.push_back(std::move(b));
    }

    // 3) Layer mappings + shiftMappings — one Behavior per (trigger, mapping) pair.
    for (const auto& layer : config.layers) {
        for (const auto& trigger : layer.triggers) {
            uint32_t triggerBit = resolveTriggerBit(trigger, config);
            if (triggerBit == 0) {
                continue;
            }

            for (const auto& [from, to] : layer.mappings) {
                auto fromSc = KeyNameMapper::nameToScancode(from);
                auto toSc = KeyNameMapper::nameToScancode(to);
                if (!fromSc || !toSc) {
                    continue;
                }
                Behavior b;
                b.kind = Behavior::Kind::LayerMapping;
                b.id = "layer:" + layer.name + ":" + trigger + ":" + from;
                b.physicalKeyName = from;
                b.physicalScancode = *fromSc;
                b.expectedOutputName = to;
                b.expectedOutputScancode = *toSc;
                b.triggerModifier = trigger;
                b.triggerModifierBit = triggerBit;
                b.layerName = layer.name;
                b.configName = configName;
                out.push_back(std::move(b));
            }

            for (const auto& shiftMap : layer.shiftMappings) {
                auto fromSc = KeyNameMapper::nameToScancode(shiftMap.key);
                auto toSc = KeyNameMapper::nameToScancode(shiftMap.output);
                if (!fromSc || !toSc) {
                    continue;
                }
                Behavior b;
                b.kind = Behavior::Kind::LayerShiftMapping;
                b.id = "shift:" + layer.name + ":" + trigger + ":" + shiftMap.key;
                b.physicalKeyName = shiftMap.key;
                b.physicalScancode = *fromSc;
                b.expectedOutputName = shiftMap.output;
                b.expectedOutputScancode = *toSc;
                b.expectedInjectShift = shiftMap.shift;
                b.triggerModifier = trigger;
                b.triggerModifierBit = triggerBit;
                b.layerName = layer.name;
                b.configName = configName;
                out.push_back(std::move(b));
            }
        }
    }

    // 4) Custom modifiers — these are inputs the user presses; their "output"
    // is the modifier bit they activate, not a scancode. expectedOutputScancode
    // is left at 0; tests for this kind assert on ModifierTracker state.
    for (size_t i = 0; i < config.customModifiers.size(); ++i) {
        const auto& cm = config.customModifiers[i];
        auto fromSc = KeyNameMapper::nameToScancode(cm.key);
        if (!fromSc) {
            continue;
        }
        Behavior b;
        b.kind = Behavior::Kind::CustomModifier;
        b.id = "customMod:" + cm.modifierName;
        b.physicalKeyName = cm.key;
        b.physicalScancode = *fromSc;
        b.expectedOutputName = cm.modifierName;
        b.expectedOutputScancode = 0;
        b.triggerModifier = cm.modifierName;
        b.triggerModifierBit = static_cast<uint32_t>(getCustomModifierBit(static_cast<int>(i + 1)));
        b.blockOutput = cm.blockOutput;
        b.configName = configName;
        out.push_back(std::move(b));
    }

    return out;
}

// Returns a human-readable name for a Behavior::Kind. Useful in failure messages.
inline const char* behaviorKindName(Behavior::Kind kind) noexcept {
    switch (kind) {
        case Behavior::Kind::Remap:
            return "Remap";
        case Behavior::Kind::ModifierRemap:
            return "ModifierRemap";
        case Behavior::Kind::NoModCombo:
            return "NoModCombo";
        case Behavior::Kind::LayerMapping:
            return "LayerMapping";
        case Behavior::Kind::LayerShiftMapping:
            return "LayerShiftMapping";
        case Behavior::Kind::CustomModifier:
            return "CustomModifier";
    }
    return "Unknown";
}

} // namespace keyflow
