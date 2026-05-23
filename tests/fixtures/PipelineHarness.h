#pragma once

#include "config/BehaviorEnumerator.h"
#include "config/ConfigBuilder.h"
#include "config/ConfigLoader.h"
#include "config/JsonConfig.h"
#include "hardware/KeyEvent.h"
#include "pipeline/Pipeline.h"
#include "processors/ModifierTracker.h"

#include <string>
#include <vector>

namespace keyflow::testing {

// One driven event + its resulting pipeline output, plus a snapshot of the
// modifier bitmask AFTER processing. Tests use this for assertions and for
// constructing diagnostic context when a step diverges from expected.
struct DrivenStep {
  KeyEvent event;
  ProcessingResult result;
  uint32_t modifiersAfter;

  DrivenStep(const KeyEvent &e, const ProcessingResult &r, uint32_t mods)
      : event(e), result(r), modifiersAfter(mods) {}
};

// Owns a Pipeline built from a config, plus the ModifierTracker pointer
// (needed to inspect bit state for diagnostics and assertions). Mirrors the
// initialization pattern in src/app/main.cpp so the test pipeline matches
// production behavior bit-for-bit.
class PipelineHarness {
public:
  // Loads config from a JSON file (typically under tests/fixtures/configs/)
  // and builds the pipeline. Throws if config can't be loaded or built.
  static PipelineHarness fromConfigFile(const std::string &path) {
    JsonConfig config = ConfigLoader::loadFromFile(path);
    return fromConfig(std::move(config));
  }

  // Loads config from an in-memory JSON string. Useful for ad-hoc test cases.
  static PipelineHarness fromConfigString(const std::string &jsonStr) {
    JsonConfig config = ConfigLoader::loadFromString(jsonStr);
    return fromConfig(std::move(config));
  }

  static PipelineHarness fromConfig(JsonConfig config) {
    PipelineHarness harness;
    harness.config_ = std::move(config);
    ModifierTracker *tracker = nullptr;
    // verbose=false suppresses [Config] stdout chatter during tests.
    if (!ConfigBuilder::buildPipeline(harness.config_, harness.pipeline_, false,
                                      &tracker)) {
      throw std::runtime_error("ConfigBuilder::buildPipeline failed");
    }
    harness.modTracker_ = tracker;
    return harness;
  }

  // Drives one key event through the pipeline and records the step.
  // Returns the result; the step is also appended to history() for later
  // inspection by diagnostic reporting.
  ProcessingResult drive(uint16_t scancode, bool isDown) {
    KeyEvent event(scancode, isDown);
    ProcessingResult result = pipeline_.process(event);
    history_.emplace_back(event, result, currentModifiers());
    return result;
  }

  // Sugar: drive a down event followed by an up event. Returns the two results.
  std::pair<ProcessingResult, ProcessingResult> tap(uint16_t scancode) {
    auto down = drive(scancode, true);
    auto up = drive(scancode, false);
    return {down, up};
  }

  // Clears the recorded step history. The pipeline's internal modifier state
  // is NOT reset — call reset() for that.
  void clearHistory() noexcept { history_.clear(); }

  // Returns the current modifier bitmask from ModifierTracker, or 0 if the
  // config doesn't include one (configs with only `remapping` and no layers).
  [[nodiscard]] uint32_t currentModifiers() const noexcept {
    return modTracker_ ? modTracker_->getModifiers() : 0;
  }

  // Returns the ModifierTracker pointer, or nullptr if this config doesn't
  // include one. Tests for CustomModifier behaviors need this to assert that
  // a custom modifier bit was correctly set after a key-down.
  [[nodiscard]] ModifierTracker *modifierTracker() noexcept {
    return modTracker_;
  }

  [[nodiscard]] const JsonConfig &config() const noexcept { return config_; }
  [[nodiscard]] const std::vector<DrivenStep> &history() const noexcept {
    return history_;
  }
  [[nodiscard]] Pipeline &pipeline() noexcept { return pipeline_; }

private:
  PipelineHarness() = default;

  JsonConfig config_;
  Pipeline pipeline_;
  ModifierTracker *modTracker_ = nullptr;
  std::vector<DrivenStep> history_;
};

} // namespace keyflow::testing
