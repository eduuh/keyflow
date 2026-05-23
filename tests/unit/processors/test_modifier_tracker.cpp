#include "hardware/Scancodes.h"
#include "processors/ModifierTracker.h"
#include <gtest/gtest.h>

using namespace keyflow;

// Documents a non-obvious invariant the integration test can't catch directly:
// ModifierTracker reads ctx.outputScancode (post-Rewire), not ctx.scancode.
// This is what makes CapsLock → LeftShift remaps work — pressing CapsLock
// sets the LeftShift bit because the Rewire processor already swapped the
// scancode by the time ModifierTracker runs.
//
// If someone "simplifies" ModifierTracker to read ctx.scancode, this test
// fails and points right at the regression.
TEST(ModifierTrackerTest, TracksOutputScancodeNotPhysical) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
}
