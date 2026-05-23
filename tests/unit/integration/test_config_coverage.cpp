// Verifies that every remapping in src/config.json produces the final output
// the config promises. The config IS the test spec — edit config.json and
// the test surface updates automatically.
//
// Scope is intentionally narrow: only the final state (action + output
// scancode + injectShift) is asserted. Intermediate pipeline state (modifier
// tracker bitmask, etc.) is not checked here.

#include "config/BehaviorEnumerator.h"
#include "fixtures/PipelineHarness.h"
#include "hardware/Scancodes.h"
#include "pipeline/Context.h"
#include "pipeline/Modifiers.h"

#include <gtest/gtest.h>

#include <string>

using namespace keyflow;
using namespace keyflow::testing;

namespace {

const char *actionName(Action a) noexcept {
  switch (a) {
  case Action::Forward:
    return "Forward";
  case Action::Replace:
    return "Replace";
  case Action::Consume:
    return "Consume";
  }
  return "?";
}

// Layer behaviors need the trigger key's physical scancode to synthesize the
// "trigger down → key → trigger up" sequence. Standard modifier names
// (LALT/RALT/etc.) map directly to scancodes; custom modifier names are
// resolved by finding their physical key in the config's customModifiers.
uint16_t resolveTriggerScancode(const Behavior &b, const JsonConfig &config) {
  if (!b.triggerModifier)
    return 0;
  switch (modifierNameToBit(*b.triggerModifier)) {
  case ModifierBit::LeftShift:
    return SC_LSHIFT;
  case ModifierBit::RightShift:
    return SC_RSHIFT;
  case ModifierBit::LeftCtrl:
    return SC_LCTRL;
  case ModifierBit::RightCtrl:
    return SC_RCTRL;
  case ModifierBit::LeftAlt:
    return SC_LALT;
  case ModifierBit::RightAlt:
    return SC_RALT;
  case ModifierBit::LeftWin:
    return SC_LWIN;
  case ModifierBit::RightWin:
    return SC_RWIN;
  case ModifierBit::PrintScreen:
    return SC_PRINTSCREEN;
  default:
    break;
  }
  for (const auto &cm : config.customModifiers) {
    if (cm.modifierName == *b.triggerModifier) {
      auto sc = KeyNameMapper::nameToScancode(cm.key);
      if (sc)
        return *sc;
    }
  }
  return 0;
}

void expectFinalState(const ProcessingResult &r, Action expectedAction,
                      uint16_t expectedOutput, bool expectedInjectShift,
                      const std::string &step) {
  EXPECT_EQ(r.action, expectedAction)
      << step << ": action mismatch — expected " << actionName(expectedAction)
      << ", got " << actionName(r.action);
  if (expectedAction == Action::Replace) {
    EXPECT_EQ(r.outputScancode, expectedOutput)
        << step << ": outputScancode mismatch — expected 0x" << std::hex
        << expectedOutput << ", got 0x" << r.outputScancode << std::dec;
    EXPECT_EQ(r.injectShift, expectedInjectShift)
        << step << ": injectShift mismatch — expected "
        << (expectedInjectShift ? "true" : "false") << ", got "
        << (r.injectShift ? "true" : "false");
  }
}

void verifyBehavior(PipelineHarness &h, const Behavior &b) {
  SCOPED_TRACE("Behavior: " + b.id);

  switch (b.kind) {
  case Behavior::Kind::Remap:
  case Behavior::Kind::ModifierRemap: {
    // Plain remap: down → up. Both events Replace with the configured target.
    // Modifier remaps look identical at this layer — the modifier-bit
    // bookkeeping is internal pipeline state, not "final" output.
    auto down = h.drive(b.physicalScancode, true);
    expectFinalState(down, Action::Replace, b.expectedOutputScancode, false,
                     "down");
    auto up = h.drive(b.physicalScancode, false);
    expectFinalState(up, Action::Replace, b.expectedOutputScancode, false,
                     "up");
    break;
  }

  case Behavior::Kind::NoModCombo: {
    // No-mod combo fires with the configured output and shift-injection.
    auto down = h.drive(b.physicalScancode, true);
    expectFinalState(down, Action::Replace, b.expectedOutputScancode,
                     b.expectedInjectShift, "down");
    auto up = h.drive(b.physicalScancode, false);
    expectFinalState(up, Action::Replace, b.expectedOutputScancode,
                     b.expectedInjectShift, "up");
    break;
  }

  case Behavior::Kind::LayerMapping:
  case Behavior::Kind::LayerShiftMapping: {
    // Layer behavior: trigger Consume, then key Replace with layer output,
    // then key up Replace, then trigger up Consume.
    uint16_t triggerSc = resolveTriggerScancode(b, h.config());
    ASSERT_NE(triggerSc, 0u) << "Could not resolve trigger scancode";

    auto trigDown = h.drive(triggerSc, true);
    expectFinalState(trigDown, Action::Consume, 0, false, "trigger down");

    auto keyDown = h.drive(b.physicalScancode, true);
    expectFinalState(keyDown, Action::Replace, b.expectedOutputScancode,
                     b.expectedInjectShift, "layer key down");

    auto keyUp = h.drive(b.physicalScancode, false);
    expectFinalState(keyUp, Action::Replace, b.expectedOutputScancode,
                     b.expectedInjectShift, "layer key up");

    auto trigUp = h.drive(triggerSc, false);
    expectFinalState(trigUp, Action::Consume, 0, false, "trigger up");
    break;
  }

  case Behavior::Kind::CustomModifier: {
    // Custom modifier: down/up both produce Consume when blockOutput=true
    // (the common case). Forward if blockOutput=false.
    Action expected = b.blockOutput ? Action::Consume : Action::Forward;
    auto down = h.drive(b.physicalScancode, true);
    expectFinalState(down, expected, 0, false, "custom mod down");
    auto up = h.drive(b.physicalScancode, false);
    expectFinalState(up, expected, 0, false, "custom mod up");
    break;
  }
  }
}

} // namespace

// Drives every behavior defined in src/config.json through the pipeline and
// asserts the final (action, outputScancode, injectShift) matches what the
// config promises. Failures name the behavior id via SCOPED_TRACE.
TEST(ConfigRemapping, FinalStateMatchesConfig) {
  auto harness = PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  auto behaviors = enumerateBehaviors(harness.config(), "config.json");

  ASSERT_FALSE(behaviors.empty()) << "Enumerator produced no behaviors — is "
                                     "src/config.json empty or malformed?";

  for (const auto &behavior : behaviors) {
    verifyBehavior(harness, behavior);
  }
}
