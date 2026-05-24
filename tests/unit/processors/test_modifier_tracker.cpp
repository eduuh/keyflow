#include "hardware/Scancodes.h"
#include "processors/ModifierTracker.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(ModifierTrackerTest, DefaultConstruction) {
  ModifierTracker tracker;
  EXPECT_EQ(tracker.getModifiers(), 0u);
  EXPECT_STREQ(tracker.name(), "ModifierTracker");
}

// ===== Single Modifier Press Tests =====

TEST(ModifierTrackerTest, LeftShiftPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::LeftShift));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
}

TEST(ModifierTrackerTest, RightShiftPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_RSHIFT;
  ctx.outputScancode = SC_RSHIFT;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::RightShift));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::RightShift));
}

TEST(ModifierTrackerTest, LeftCtrlPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::LeftCtrl));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftCtrl));
}

TEST(ModifierTrackerTest, RightCtrlPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_RCTRL;
  ctx.outputScancode = SC_RCTRL;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::RightCtrl));
}

TEST(ModifierTrackerTest, LeftAltPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::LeftAlt));
}

TEST(ModifierTrackerTest, RightAltPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::RightAlt));
}

TEST(ModifierTrackerTest, LeftWinPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_LWIN;
  ctx.outputScancode = SC_LWIN;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::LeftWin));
}

TEST(ModifierTrackerTest, RightWinPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_RWIN;
  ctx.outputScancode = SC_RWIN;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::RightWin));
}

TEST(ModifierTrackerTest, PrintScreenPress) {
  ModifierTracker tracker;
  Context ctx;
  ctx.scancode = SC_PRINTSCREEN;
  ctx.outputScancode = SC_PRINTSCREEN;
  ctx.isDown = true;

  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::PrintScreen));
}

// ===== Modifier Release Tests =====

TEST(ModifierTrackerTest, ModifierPressAndRelease) {
  ModifierTracker tracker;
  Context ctx;

  // Press
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  tracker.process(ctx);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);

  // Release
  ctx.isDown = false;
  tracker.process(ctx);
  EXPECT_EQ(ctx.modifiers, 0u);
  EXPECT_FALSE(tracker.isModifierActive(ModifierBit::LeftShift));
}

TEST(ModifierTrackerTest, MultipleModifierReleases) {
  ModifierTracker tracker;
  Context ctx;

  // Press Shift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  tracker.process(ctx);

  // Press Ctrl
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  tracker.process(ctx);

  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftCtrl), 0u);

  // Release Shift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = false;
  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftCtrl), 0u);

  // Release Ctrl
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, 0u);
}

// ===== Multiple Modifiers Tests =====

TEST(ModifierTrackerTest, TwoModifiersPressed) {
  ModifierTracker tracker;
  Context ctx;

  // Press Shift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  tracker.process(ctx);

  // Press Ctrl
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  tracker.process(ctx);

  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftCtrl), 0u);
}

TEST(ModifierTrackerTest, ThreeModifiersPressed) {
  ModifierTracker tracker;
  Context ctx;

  ctx.isDown = true;

  // Press Shift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  tracker.process(ctx);

  // Press Ctrl
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  tracker.process(ctx);

  // Press Alt
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  tracker.process(ctx);

  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftCtrl), 0u);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftAlt), 0u);
}

TEST(ModifierTrackerTest, AllModifiersPressed) {
  ModifierTracker tracker;
  Context ctx;
  ctx.isDown = true;

  const uint16_t modifiers[] = {SC_LSHIFT, SC_RSHIFT, SC_LCTRL,
                                SC_RCTRL,  SC_LALT,   SC_RALT,
                                SC_LWIN,   SC_RWIN,   SC_PRINTSCREEN};

  for (uint16_t mod : modifiers) {
    ctx.scancode = mod;
    ctx.outputScancode = mod;
    tracker.process(ctx);
  }

  EXPECT_NE(ctx.modifiers, 0u);
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::RightShift));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftCtrl));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::RightCtrl));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftAlt));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::RightAlt));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftWin));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::RightWin));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::PrintScreen));
}

// ===== Non-Modifier Keys Tests =====

TEST(ModifierTrackerTest, NonModifierKeyDoesNotAffectState) {
  ModifierTracker tracker;
  Context ctx;

  // Press a regular key
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, 0u);
}

TEST(ModifierTrackerTest, NonModifierKeyWithModifierPressed) {
  ModifierTracker tracker;
  Context ctx;

  // Press Shift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  tracker.process(ctx);

  uint32_t modifiersAfterShift = ctx.modifiers;

  // Press A (non-modifier)
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  tracker.process(ctx);

  // Modifiers should be unchanged
  EXPECT_EQ(ctx.modifiers, modifiersAfterShift);
}

// ===== Uses outputScancode Tests =====

TEST(ModifierTrackerTest, UsesOutputScancodeAfterRewire) {
  ModifierTracker tracker;
  Context ctx;

  // Simulate a remapped key: physical A remapped to Shift
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_LSHIFT; // After Rewire processor
  ctx.isDown = true;

  tracker.process(ctx);

  // Should track based on outputScancode (SC_LSHIFT), not scancode (SC_A)
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);
}

// ===== Always Continues Tests =====

TEST(ModifierTrackerTest, AlwaysContinuesPipeline) {
  ModifierTracker tracker;
  Context ctx;

  // Test with modifier
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  EXPECT_TRUE(tracker.process(ctx));

  // Test with non-modifier
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  EXPECT_TRUE(tracker.process(ctx));
}

// ===== Edge Cases =====

TEST(ModifierTrackerTest, DoublePress) {
  ModifierTracker tracker;
  Context ctx;

  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;

  // First press
  tracker.process(ctx);
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));

  // Second press (hardware repeat or double press)
  tracker.process(ctx);
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
}

TEST(ModifierTrackerTest, ReleaseWithoutPress) {
  ModifierTracker tracker;
  Context ctx;

  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = false;

  // Release without press
  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, 0u);
  EXPECT_FALSE(tracker.isModifierActive(ModifierBit::LeftShift));
}

TEST(ModifierTrackerTest, BothShiftKeysPressed) {
  ModifierTracker tracker;
  Context ctx;
  ctx.isDown = true;

  // Press left shift
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  tracker.process(ctx);

  // Press right shift
  ctx.scancode = SC_RSHIFT;
  ctx.outputScancode = SC_RSHIFT;
  tracker.process(ctx);

  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::LeftShift));
  EXPECT_TRUE(tracker.isModifierActive(ModifierBit::RightShift));
}

TEST(ModifierTrackerTest, ContextModifiersMatchGetModifiers) {
  ModifierTracker tracker;
  Context ctx;

  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  ctx.isDown = true;
  tracker.process(ctx);

  EXPECT_EQ(ctx.modifiers, tracker.getModifiers());
}
