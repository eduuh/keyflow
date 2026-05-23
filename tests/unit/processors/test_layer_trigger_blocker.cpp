#include "hardware/Scancodes.h"
#include "processors/LayerTriggerBlocker.h"
#include <gtest/gtest.h>

using namespace keyflow;

// The 4 tests below encode the physical-vs-output trigger distinction that
// fixed the customModifier-as-layer-trigger bug (era of commit d276816).
// The blocker has two paths:
//   - addTriggerByScancode / addPhysicalTrigger → matches ctx.scancode
//   - addTrigger (by name)                      → matches ctx.outputScancode
// These behaviors are subtle and the integration test won't pinpoint the
// processor if they regress.

// Physical-trigger path: a custom modifier registered by scancode blocks
// based on the physical key pressed, regardless of remapping.
TEST(LayerTriggerBlockerTest, PhysicalTriggerBlocksBeforeRemapping) {
  LayerTriggerBlocker blocker;
  blocker.addTriggerByScancode(SC_RALT);

  Context ctx;
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// Physical-trigger path must NOT block keys that happen to be remapped TO
// the trigger scancode. E.g. LeftWin remapped to RightAlt should pass even
// if RAlt is registered as a physical trigger, so Win+X-style shortcuts
// keep working.
TEST(LayerTriggerBlockerTest, RemappedKeyNotBlockedByPhysicalTrigger) {
  LayerTriggerBlocker blocker;
  blocker.addTriggerByScancode(SC_RALT);

  Context ctx;
  ctx.scancode = SC_LWIN;
  ctx.outputScancode = SC_RALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
}

// Output-trigger path: a standard layer trigger added by name blocks based
// on the post-Rewire scancode. So if CapsLock is remapped to LeftAlt and
// LAlt is a layer trigger, the resulting LAlt is consumed.
TEST(LayerTriggerBlockerTest, OutputTriggerBlocksAfterRemapping) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger("LALT");

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// Both kinds of triggers coexist in the same processor without interference.
TEST(LayerTriggerBlockerTest, PhysicalAndOutputTriggersBothWork) {
  LayerTriggerBlocker blocker;
  blocker.addTriggerByScancode(SC_RALT);
  blocker.addTrigger("LALT");

  Context ctx;
  ctx.isDown = true;

  // Physical RAlt → blocked.
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  ctx.action = Action::Forward;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // CapsLock remapped to LAlt → blocked (output trigger path).
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_LALT;
  ctx.action = Action::Forward;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // LWin remapped to RAlt → passes through (output doesn't match the
  // physical-trigger path, and LAlt isn't the output).
  ctx.scancode = SC_LWIN;
  ctx.outputScancode = SC_RALT;
  ctx.action = Action::Forward;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
}
