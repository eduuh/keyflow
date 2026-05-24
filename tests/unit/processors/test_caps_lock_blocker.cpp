#include "hardware/Scancodes.h"
#include "processors/CapsLockBlocker.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(CapsLockBlockerTest, DefaultConstruction) {
  CapsLockBlocker blocker;
  EXPECT_STREQ(blocker.name(), "CapsLockBlocker");
}

// ===== Basic Blocking Tests =====

TEST(CapsLockBlockerTest, BlocksCapsLockKey) {
  CapsLockBlocker blocker;

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = blocker.process(ctx);

  EXPECT_FALSE(continues); // Pipeline should stop
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(CapsLockBlockerTest, BlocksCapsLockKeyUp) {
  CapsLockBlocker blocker;

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  ctx.isDown = false; // Key up
  ctx.action = Action::Forward;

  bool continues = blocker.process(ctx);

  EXPECT_FALSE(continues); // Pipeline should stop
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(CapsLockBlockerTest, AllowsRemappedCapsLock) {
  CapsLockBlocker blocker;

  // Simulate CapsLock → LeftShift remapping (after Rewire processor)
  Context ctx;
  ctx.scancode = SC_CAPSLOCK;     // Physical key is CapsLock
  ctx.outputScancode = SC_LSHIFT; // Remapped to LeftShift
  ctx.isDown = true;
  ctx.action = Action::Replace;

  bool continues = blocker.process(ctx);

  EXPECT_TRUE(continues);                 // Should continue pipeline
  EXPECT_EQ(ctx.action, Action::Replace); // Should not change action
}

TEST(CapsLockBlockerTest, AllowsOtherKeys) {
  CapsLockBlocker blocker;

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = blocker.process(ctx);

  EXPECT_TRUE(continues);                 // Should continue pipeline
  EXPECT_EQ(ctx.action, Action::Forward); // Should not change action
}

// ===== Edge Cases =====

TEST(CapsLockBlockerTest, BlocksCapsLockEvenIfAlreadyReplaced) {
  CapsLockBlocker blocker;

  // Even if action is already Replace, if outputScancode is CapsLock, block it
  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  ctx.isDown = true;
  ctx.action = Action::Replace; // Already processed by previous processor

  bool continues = blocker.process(ctx);

  EXPECT_FALSE(continues); // Should still block
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(CapsLockBlockerTest, AllowsShiftKeys) {
  CapsLockBlocker blocker;

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // Test LeftShift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  bool continues = blocker.process(ctx);
  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Test RightShift
  ctx.scancode = SC_RSHIFT;
  ctx.outputScancode = SC_RSHIFT;
  continues = blocker.process(ctx);
  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(CapsLockBlockerTest, AllowsControlKeys) {
  CapsLockBlocker blocker;

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // Test LeftCtrl
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  bool continues = blocker.process(ctx);
  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Test RightCtrl
  ctx.scancode = SC_RCTRL;
  ctx.outputScancode = SC_RCTRL;
  continues = blocker.process(ctx);
  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== Real-world Scenario Tests =====

TEST(CapsLockBlockerTest, RealWorldScenario_CapsLockRemappedToShift) {
  CapsLockBlocker blocker;

  // Scenario: User has CapsLock → LeftShift in config
  // After Rewire processor, outputScancode should be LeftShift
  Context ctx;
  ctx.scancode = SC_CAPSLOCK;     // Physical CapsLock pressed
  ctx.outputScancode = SC_LSHIFT; // Remapped to LeftShift
  ctx.isDown = true;
  ctx.action = Action::Replace;

  bool continues = blocker.process(ctx);

  // Should allow the remapped key through
  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Replace);
}

TEST(CapsLockBlockerTest, RealWorldScenario_AccidentalCapsLockPressed) {
  CapsLockBlocker blocker;

  // Scenario: Somehow CapsLock key got through without remapping
  // (e.g., user edited config and removed CapsLock mapping)
  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK; // Not remapped
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = blocker.process(ctx);

  // Should block to prevent caps lock toggle
  EXPECT_FALSE(continues);
  EXPECT_EQ(ctx.action, Action::Consume);
}
