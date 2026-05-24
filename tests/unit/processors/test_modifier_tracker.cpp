#include "hardware/Scancodes.h"
#include "processors/ModifierTracker.h"
#include <gtest/gtest.h>

using namespace keyflow;

namespace {

void press(ModifierTracker &tracker, uint16_t scancode) {
  Context ctx;
  ctx.scancode = scancode;
  ctx.outputScancode = scancode;
  ctx.isDown = true;
  tracker.process(ctx);
}

void release(ModifierTracker &tracker, uint16_t scancode) {
  Context ctx;
  ctx.scancode = scancode;
  ctx.outputScancode = scancode;
  ctx.isDown = false;
  tracker.process(ctx);
}

struct ModifierCase {
  uint16_t scancode;
  ModifierBit bit;
};

class ModifierTrackerPressParam
    : public ::testing::TestWithParam<ModifierCase> {};

} // namespace

TEST_P(ModifierTrackerPressParam, PressTrackedAsBit) {
  ModifierTracker tracker;
  const auto &mod = GetParam();
  press(tracker, mod.scancode);
  EXPECT_TRUE(tracker.isModifierActive(mod.bit));
}

INSTANTIATE_TEST_SUITE_P(
    AllModifiers, ModifierTrackerPressParam,
    ::testing::Values(ModifierCase{SC_LSHIFT, ModifierBit::LeftShift},
                      ModifierCase{SC_RSHIFT, ModifierBit::RightShift},
                      ModifierCase{SC_LCTRL, ModifierBit::LeftCtrl},
                      ModifierCase{SC_RCTRL, ModifierBit::RightCtrl},
                      ModifierCase{SC_LALT, ModifierBit::LeftAlt},
                      ModifierCase{SC_RALT, ModifierBit::RightAlt},
                      ModifierCase{SC_LWIN, ModifierBit::LeftWin},
                      ModifierCase{SC_RWIN, ModifierBit::RightWin},
                      ModifierCase{SC_PRINTSCREEN, ModifierBit::PrintScreen}));

TEST(ModifierTrackerTest, ReleaseClearsBitButOthersPersist) {
  ModifierTracker tracker;
  press(tracker, SC_LSHIFT);
  press(tracker, SC_LCTRL);

  release(tracker, SC_LSHIFT);

  EXPECT_FALSE(tracker.isModifierActive(ModifierBit::LeftShift));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftCtrl));
}

TEST(ModifierTrackerTest, NonModifierKeyDoesNotAffectState) {
  ModifierTracker tracker;
  press(tracker, SC_LSHIFT);
  const uint32_t before = tracker.getModifiers();

  press(tracker, SC_A);

  EXPECT_EQ(tracker.getModifiers(), before);
}

TEST(ModifierTrackerTest, TracksOutputScancodeNotPhysical) {
  // Rewire upstream may have remapped A -> LeftShift. The tracker must
  // observe the post-rewire output, not the original physical key.
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
}

TEST(ModifierTrackerTest, AlwaysContinuesPipeline) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  EXPECT_TRUE(tracker.process(ctx));
}

TEST(ModifierTrackerTest, DoublePressIsIdempotent) {
  ModifierTracker tracker;
  press(tracker, SC_LSHIFT);
  press(tracker, SC_LSHIFT);
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
}

TEST(ModifierTrackerTest, ReleaseWithoutPressIsHarmless) {
  ModifierTracker tracker;
  release(tracker, SC_LSHIFT);
  EXPECT_EQ(tracker.getModifiers(), 0u);
}
