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
  // Use string overload to add as output trigger (standard layer)
  blocker.addTrigger("LALT");

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
  blocker.addTrigger(
      SC_LALT); // Duplicate - automatically deduplicated by unordered_set
  EXPECT_EQ(blocker.triggerCount(), 1u); // Deduplicated to 1

  Context ctx;
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume); // Still works correctly
}

// ===== Custom Modifier Physical vs Output Tests =====

TEST(LayerTriggerBlockerTest, PhysicalTriggerBlocksBeforeRemapping) {
  LayerTriggerBlocker blocker;
  // Add RightAlt as a physical trigger (custom modifier)
  blocker.addTriggerByScancode(SC_RALT);

  Context ctx;
  ctx.scancode = SC_RALT;       // Physical RightAlt
  ctx.outputScancode = SC_RALT; // Not remapped
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);

  // Should be blocked because physical scancode matches
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(LayerTriggerBlockerTest, RemappedKeyNotBlockedByPhysicalTrigger) {
  LayerTriggerBlocker blocker;
  // Add RightAlt as a physical trigger (custom modifier with blockOutput=true)
  blocker.addTriggerByScancode(SC_RALT);

  Context ctx;
  ctx.scancode = SC_LWIN;       // Physical LeftWin
  ctx.outputScancode = SC_RALT; // Remapped to RightAlt
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);

  // Should NOT be blocked - physical scancode doesn't match
  // This is the key fix: remapped keys pass through
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(LayerTriggerBlockerTest, OutputTriggerBlocksAfterRemapping) {
  LayerTriggerBlocker blocker;
  // Add LALT as an output trigger (standard layer)
  blocker.addTrigger("LALT");

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;   // Physical CapsLock
  ctx.outputScancode = SC_LALT; // Remapped to LeftAlt
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);

  // Should be blocked because outputScancode matches
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(LayerTriggerBlockerTest, PhysicalAndOutputTriggersBothWork) {
  LayerTriggerBlocker blocker;
  // Add RightAlt as physical trigger (custom modifier)
  blocker.addTriggerByScancode(SC_RALT);
  // Add LeftAlt as output trigger (standard layer)
  blocker.addTrigger("LALT");

  EXPECT_EQ(blocker.triggerCount(), 2u);

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // Test 1: Physical RightAlt should be blocked
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // Test 2: Remapped to LeftAlt should be blocked (output trigger)
  ctx.action = Action::Forward;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_LALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // Test 3: LeftWin remapped to RightAlt should NOT be blocked
  ctx.action = Action::Forward;
  ctx.scancode = SC_LWIN;
  ctx.outputScancode = SC_RALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(LayerTriggerBlockerTest,
     RealWorldScenario_WinKeyRemappedToCustomModifier) {
  // Scenario: User has LeftWin → RightAlt remapping
  // RightAlt is a custom modifier with blockOutput: true
  LayerTriggerBlocker blocker;
  blocker.addTriggerByScancode(SC_RALT);

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // Press physical RightAlt → should be blocked
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // Press LeftWin (remapped to RightAlt) → should pass through
  ctx.action = Action::Forward;
  ctx.scancode = SC_LWIN;
  ctx.outputScancode = SC_RALT;
  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // This allows Win+C shortcuts to work even though
  // RightAlt is a custom modifier with blockOutput: true
}

TEST(LayerTriggerBlockerTest, AddPhysicalTriggerMethod) {
  LayerTriggerBlocker blocker;
  blocker.addPhysicalTrigger(SC_RALT);
  EXPECT_EQ(blocker.triggerCount(), 1u);

  Context ctx;
  ctx.scancode = SC_RALT;
  ctx.outputScancode = SC_RALT;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  blocker.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}
