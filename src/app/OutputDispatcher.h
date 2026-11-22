#pragma once

#include "hardware/KeyEvent.h"
#include "hardware/Scancodes.h"
#include "pipeline/Modifiers.h"
#include "pipeline/Pipeline.h"
#include "platform/IPlatform.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace keyflow {

// Owns the output selected on key-down until the final physical source releases.
class OutputDispatcher {
  public:
    OutputDispatcher(Pipeline& pipeline, IHardwareIO& hardware)
        : pipeline_(&pipeline), hardware_(&hardware) {}

    [[nodiscard]] bool process(const KeyEvent& event) {
        source_ = event.source;
        const auto INPUT = identity(event.scancode, event.native);
        auto held = held_.find(INPUT);
        if (event.action == KeyAction::Release) {
            return releaseInput(event, held);
        }

        if (held != held_.end()) {
            return repeatInput(event, held->second);
        }
        if (event.action == KeyAction::Repeat) {
            return true;
        }

        Binding binding;
        binding.sources.insert(event.source);
        binding.output = event.scancode;
        binding.native = event.native;
        if (!event.native) {
            const auto RESULT = pipeline_->process(KeyEvent(event.scancode, true));
            binding.consumed = RESULT.action == Action::Consume;
            if (RESULT.action == Action::Replace) {
                binding.output = RESULT.outputScancode;
                binding.shift = RESULT.injectShift;
            }
            binding.consumed = binding.consumed || binding.output == SC_NOP;
        }
        held_.emplace(INPUT, binding);
        if (binding.consumed) {
            return true;
        }
        if (binding.shift && !press(SC_LSHIFT, false)) {
            return false;
        }
        return press(binding.output, binding.native);
    }

    [[nodiscard]] bool physicalCtrlHeld() const noexcept {
        return held_.count(identity(SC_LCTRL, false)) != 0 ||
               held_.count(identity(SC_RCTRL, false)) != 0;
    }

    void shutdown() noexcept {
        for (bool modifiers : {false, true}) {
            for (const auto& [KEY, OWNERS] : outputs_) {
                const bool NATIVE = (KEY >> 16U) != 0;
                if ((!NATIVE && isModifierKey(static_cast<uint16_t>(KEY))) == modifiers) {
                    (void)hardware_->sendEvent(KeyEvent(static_cast<uint16_t>(KEY),
                                                        KeyAction::Release, OWNERS.source, NATIVE));
                }
            }
        }
        for (const auto& [KEY, BINDING] : held_) {
            (void)BINDING;
            if ((KEY >> 16U) == 0) {
                (void)pipeline_->process(KeyEvent(static_cast<uint16_t>(KEY), false));
            }
        }
        outputs_.clear();
        held_.clear();
    }

  private:
    struct Binding {
        std::unordered_set<uint16_t> sources;
        uint16_t output = 0;
        bool native = false;
        bool shift = false;
        bool consumed = false;
    };
    struct OutputOwners {
        unsigned count = 0;
        uint16_t source = 0;
    };
    using HeldKeys = std::unordered_map<uint32_t, Binding>;

    bool releaseInput(const KeyEvent& event, HeldKeys::iterator held) {
        if (held == held_.end() || held->second.sources.erase(event.source) == 0 ||
            !held->second.sources.empty()) {
            return true;
        }
        if (!event.native) {
            (void)pipeline_->process(KeyEvent(event.scancode, false));
        }
        const auto BINDING = held->second;
        held_.erase(held);
        if (BINDING.consumed) {
            return true;
        }
        bool ok = release(BINDING.output, BINDING.native);
        if (BINDING.shift) {
            ok = release(SC_LSHIFT, false) && ok;
        }
        return ok;
    }

    bool repeatInput(const KeyEvent& event, Binding& binding) {
        const bool EXISTING = binding.sources.count(event.source) != 0;
        if (event.action == KeyAction::Press && !EXISTING) {
            binding.sources.insert(event.source);
            return true;
        }
        if (!EXISTING || binding.consumed ||
            event.source != *std::min_element(binding.sources.begin(), binding.sources.end())) {
            return true;
        }
        const auto OUTPUT = outputs_.find(identity(binding.output, binding.native));
        if (OUTPUT == outputs_.end()) {
            return false;
        }
        return hardware_->sendEvent(
            KeyEvent(binding.output, KeyAction::Repeat, OUTPUT->second.source, binding.native));
    }

    static uint32_t identity(uint16_t key, bool native) noexcept {
        return static_cast<uint32_t>(key) | (native ? 0x10000U : 0U);
    }

    bool press(uint16_t key, bool native) {
        auto& owners = outputs_[identity(key, native)];
        if (++owners.count != 1) {
            return true;
        }
        owners.source = source_;
        return hardware_->sendEvent(KeyEvent(key, KeyAction::Press, source_, native));
    }

    bool release(uint16_t key, bool native) {
        const auto ID = identity(key, native);
        const auto IT = outputs_.find(ID);
        if (IT == outputs_.end()) {
            return true;
        }
        if (IT->second.count > 1) {
            --IT->second.count;
            return true;
        }
        const auto SOURCE = IT->second.source;
        if (!hardware_->sendEvent(KeyEvent(key, KeyAction::Release, SOURCE, native))) {
            return false;
        }
        outputs_.erase(IT);
        return true;
    }

    Pipeline* pipeline_;
    IHardwareIO* hardware_;
    HeldKeys held_;
    std::unordered_map<uint32_t, OutputOwners> outputs_;
    uint16_t source_ = 0;
};

} // namespace keyflow
