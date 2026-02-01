#include "hardware/Scancodes.h"
#include "processors/LayerTriggerBlocker.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(LayerTriggerBlockerTest, DefaultConstruction) {
  LayerTriggerBlocker blocker;
  EXPECT_EQ(blocker.triggerCount(), 0u);
  EXPECT_STREQ(blocker.name(), "LayerTriggerBlocker");
}

// ===== Add Trigger Tests =====

TEST(LayerTriggerBlockerTest, AddTriggerByScancode) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);
  EXPECT_EQ(blocker.triggerCount(), 1u);
}

TEST(LayerTriggerBlockerTest, AddMultipleTriggers) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);
  blocker.addTrigger(SC_RALT);
  blocker.addTrigger(SC_LCTRL);
  EXPECT_EQ(blocker.triggerCount(), 3u);
}

TEST(LayerTriggerBlockerTest, AddTriggerByName) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger("LALT");
  EXPECT_EQ(blocker.triggerCount(), 1u);
}

TEST(LayerTriggerBlockerTest, AddTriggerByNameMultiple) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger("LALT");
  blocker.addTrigger("RALT");
  EXPECT_EQ(blocker.triggerCount(), 2u);
}

TEST(LayerTriggerBlockerTest, AddTriggerInvalidName) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger("INVALID");
  EXPECT_EQ(blocker.triggerCount(), 0u); // Should not add
}

// ===== Blocking Behavior Tests =====

TEST(LayerTriggerBlockerTest, BlocksSingleTrigger) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = blocker.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(LayerTriggerBlockerTest, BlocksMultipleTriggers) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);
  blocker.addTrigger(SC_RALT);

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // Test LALT
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // Reset and test RALT
  ctx.action = Action::Forward;
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(LayerTriggerBlockerTest, NonTriggerKeyPassesThrough) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(LayerTriggerBlockerTest, BlocksBasedOnOutputScancode) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;   // Physical key
  ctx.outputScancode = SC_LALT; // After remapping
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);

  // Should block based on outputScancode, not scancode
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(LayerTriggerBlockerTest, DoesNotBlockIfAlreadyProcessed) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Replace; // Already processed

  blocker.process(ctx);

  // Should not change action if already processed
  EXPECT_EQ(ctx.action, Action::Replace);
}

// ===== Key Up/Down Tests =====

TEST(LayerTriggerBlockerTest, BlocksKeyDown) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(LayerTriggerBlockerTest, BlocksKeyUp) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = false; // Key up
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// ===== All Modifier Tests =====

TEST(LayerTriggerBlockerTest, BlocksAllModifierTypes) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LSHIFT);
  blocker.addTrigger(SC_RSHIFT);
  blocker.addTrigger(SC_LCTRL);
  blocker.addTrigger(SC_RCTRL);
  blocker.addTrigger(SC_LALT);
  blocker.addTrigger(SC_RALT);
  blocker.addTrigger(SC_LWIN);
  blocker.addTrigger(SC_RWIN);

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  const uint16_t modifiers[] = {SC_LSHIFT, SC_RSHIFT, SC_LCTRL, SC_RCTRL,
                                SC_LALT,   SC_RALT,   SC_LWIN,  SC_RWIN};

  for (uint16_t mod : modifiers) {
    ctx.scancode = mod;
    ctx.outputScancode = mod;
    ctx.action = Action::Forward;
    blocker.process(ctx);
    EXPECT_EQ(ctx.action, Action::Consume);
  }
}

// ===== Always Continues Tests =====

TEST(LayerTriggerBlockerTest, AlwaysContinuesPipeline) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);

  Context ctx;
  ctx.isDown = true;

  // Test with trigger key
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.action = Action::Forward;
  EXPECT_TRUE(blocker.process(ctx));

  // Test with non-trigger key
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.action = Action::Forward;
  EXPECT_TRUE(blocker.process(ctx));
}

// ===== Edge Cases =====

TEST(LayerTriggerBlockerTest, EmptyBlockerPassesThrough) {
  LayerTriggerBlocker blocker; // No triggers added

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(LayerTriggerBlockerTest, DuplicateTriggersWork) {
  LayerTriggerBlocker blocker;
  blocker.addTrigger(SC_LALT);
  blocker.addTrigger(SC_LALT); // Duplicate
  EXPECT_EQ(blocker.triggerCount(), 2u);

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume); // Still works
}
