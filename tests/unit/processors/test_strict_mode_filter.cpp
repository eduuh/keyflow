#include "hardware/Scancodes.h"
#include "pipeline/Modifiers.h"
#include "processors/StrictModeFilter.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(StrictModeFilterTest, DefaultConstruction) {
  StrictModeFilter filter;
  EXPECT_EQ(filter.allowedKeyCount(), 0u);
  EXPECT_STREQ(filter.name(), "StrictModeFilter");
}

// ===== Add Allowed Key Tests =====

TEST(StrictModeFilterTest, AddSingleAllowedKey) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  EXPECT_EQ(filter.allowedKeyCount(), 1u);
  EXPECT_TRUE(filter.isAllowed(SC_A));
}

TEST(StrictModeFilterTest, AddMultipleAllowedKeys) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  filter.addAllowedKey(SC_B);
  filter.addAllowedKey(SC_C);

  EXPECT_EQ(filter.allowedKeyCount(), 3u);
  EXPECT_TRUE(filter.isAllowed(SC_A));
  EXPECT_TRUE(filter.isAllowed(SC_B));
  EXPECT_TRUE(filter.isAllowed(SC_C));
}

TEST(StrictModeFilterTest, AddAllowedKeysVector) {
  StrictModeFilter filter;
  std::vector<uint16_t> keys = {SC_A, SC_B, SC_C, SC_D};
  filter.addAllowedKeys(keys);

  EXPECT_EQ(filter.allowedKeyCount(), 4u);
  for (uint16_t key : keys) {
    EXPECT_TRUE(filter.isAllowed(key));
  }
}

TEST(StrictModeFilterTest, DuplicateKeysNotCounted) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  filter.addAllowedKey(SC_A); // Duplicate
  filter.addAllowedKey(SC_A); // Duplicate

  EXPECT_EQ(filter.allowedKeyCount(), 1u); // Still just 1
  EXPECT_TRUE(filter.isAllowed(SC_A));
}

// ===== Filtering Behavior Tests =====

TEST(StrictModeFilterTest, AllowedKeyPassesThrough) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = filter.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward); // Not consumed
}

TEST(StrictModeFilterTest, DisallowedKeyBlocked) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.scancode = SC_B; // Not in allowed list
  ctx.outputScancode = SC_B;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  filter.process(ctx);

  EXPECT_EQ(ctx.action, Action::Consume); // Blocked!
}

TEST(StrictModeFilterTest, MultipleAllowedKeysWork) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  filter.addAllowedKey(SC_B);
  filter.addAllowedKey(SC_C);

  Context ctx;
  ctx.isDown = true;

  // Test A passes
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Test B passes
  ctx.scancode = SC_B;
  ctx.outputScancode = SC_B;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Test D blocked
  ctx.scancode = SC_D;
  ctx.outputScancode = SC_D;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// ===== Only Filters Forward Actions =====

TEST(StrictModeFilterTest, DoesNotFilterAlreadyProcessedKeys) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.scancode = SC_B;       // Not allowed
  ctx.outputScancode = SC_B; // Not allowed
  ctx.isDown = true;
  ctx.action = Action::Replace; // Already processed by another processor

  filter.process(ctx);

  // Should NOT change action if already processed
  EXPECT_EQ(ctx.action, Action::Replace);
}

TEST(StrictModeFilterTest, DoesNotFilterConsumedKeys) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.scancode = SC_B;
  ctx.outputScancode = SC_B;
  ctx.isDown = true;
  ctx.action = Action::Consume; // Already consumed

  filter.process(ctx);

  EXPECT_EQ(ctx.action, Action::Consume); // Still consumed
}

// ===== Empty Filter Blocks Everything =====

TEST(StrictModeFilterTest, EmptyFilterBlocksAllKeys) {
  StrictModeFilter filter; // No allowed keys

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  filter.process(ctx);

  EXPECT_EQ(ctx.action, Action::Consume); // Blocked
}

// ===== Uses outputScancode =====

TEST(StrictModeFilterTest, FiltersBasedOnOutputScancode) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_B); // Allow B

  Context ctx;
  ctx.scancode = SC_A;       // Physical key
  ctx.outputScancode = SC_B; // After remapping
  ctx.isDown = true;
  ctx.action = Action::Forward;

  filter.process(ctx);

  // Should check outputScancode (B), which is allowed
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(StrictModeFilterTest, BlocksBasedOnOutputScancode) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A); // Allow A

  Context ctx;
  ctx.scancode = SC_A;       // Physical key (allowed)
  ctx.outputScancode = SC_B; // After remapping (NOT allowed)
  ctx.isDown = true;
  ctx.action = Action::Forward;

  filter.process(ctx);

  // Should check outputScancode (B), which is NOT allowed
  EXPECT_EQ(ctx.action, Action::Consume);
}

// ===== Always Continues Pipeline =====

TEST(StrictModeFilterTest, AlwaysContinuesPipeline) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.isDown = true;

  // Test with allowed key
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.action = Action::Forward;
  EXPECT_TRUE(filter.process(ctx));

  // Test with blocked key
  ctx.scancode = SC_B;
  ctx.outputScancode = SC_B;
  ctx.action = Action::Forward;
  EXPECT_TRUE(filter.process(ctx));
}

// ===== Key Up/Down Tests =====

TEST(StrictModeFilterTest, FiltersKeyDown) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.scancode = SC_B;
  ctx.outputScancode = SC_B;
  ctx.isDown = true; // Key down
  ctx.action = Action::Forward;

  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, FiltersKeyUp) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);

  Context ctx;
  ctx.scancode = SC_B;
  ctx.outputScancode = SC_B;
  ctx.isDown = false; // Key up
  ctx.action = Action::Forward;

  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// ===== Practical Use Case Tests =====

TEST(StrictModeFilterTest, FullKeyboardRemapScenario) {
  StrictModeFilter filter;

  // Simulate a QWERTY -> Dvorak style full remap
  // Only add the keys we explicitly mapped
  std::vector<uint16_t> mappedKeys = {SC_A,     SC_B,     SC_C,         SC_D,
                                      SC_E,     SC_F,     SC_G,         SC_H,
                                      SC_SPACE, SC_ENTER, SC_BACKSPACE, SC_TAB};

  filter.addAllowedKeys(mappedKeys);

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // Mapped key passes
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Unmapped key blocked
  ctx.scancode = SC_Z; // Not in our mapping
  ctx.outputScancode = SC_Z;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, WorksWithModifierTracking) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  filter.addAllowedKey(SC_LSHIFT); // Allow shift

  Context ctx;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftShift);

  // Allowed key with modifier passes
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Shift key itself passes
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Disallowed key blocked even with modifier
  ctx.scancode = SC_Z;
  ctx.outputScancode = SC_Z;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// ===== Strict Mode Enhancement Tests =====

TEST(StrictModeFilterTest, OutputKeysAreBlocked) {
  StrictModeFilter filter;
  // Simulate CapsLock -> Escape remapping
  // Only add INPUT key (CapsLock), NOT output key (Escape)
  filter.addAllowedKey(SC_CAPSLOCK);

  Context ctx;
  ctx.isDown = true;

  // CapsLock (input) is allowed
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Escape (output) is blocked when pressed directly
  ctx.scancode = SC_ESCAPE;
  ctx.outputScancode = SC_ESCAPE;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, ModifiersNotInConfigAreBlocked) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  filter.addAllowedKey(SC_LALT); // Only LALT is allowed

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // LALT is allowed
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // LSHIFT is NOT allowed (not in config)
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // LCTRL is NOT allowed (not in config)
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, LayerTriggerWorkButAreConsumed) {
  StrictModeFilter filter;
  // Layer trigger (LALT) should be allowed as an input key
  filter.addAllowedKey(SC_LALT);
  filter.addAllowedKey(SC_J); // Layer mapping input

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // LALT trigger is allowed through the filter
  ctx.scancode = SC_LALT;
  ctx.outputScancode = SC_LALT;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // J (layer input key) is allowed
  ctx.scancode = SC_J;
  ctx.outputScancode = SC_J;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // K (layer output key) is blocked
  ctx.scancode = SC_K;
  ctx.outputScancode = SC_K;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, ModifierPlusUnmappedKeyIsBlocked) {
  StrictModeFilter filter;
  filter.addAllowedKey(SC_A);
  // Note: LCTRL is NOT in the allowed list

  Context ctx;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  // A is allowed
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // Z is blocked (unmapped key)
  ctx.scancode = SC_Z;
  ctx.outputScancode = SC_Z;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  // LCTRL itself is blocked
  ctx.scancode = SC_LCTRL;
  ctx.outputScancode = SC_LCTRL;
  ctx.action = Action::Forward;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, RemappingScenarioInputVsOutput) {
  StrictModeFilter filter;
  // CapsLock -> LeftShift: Only add CapsLock (input), NOT LeftShift (output)
  filter.addAllowedKey(SC_CAPSLOCK);

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // CapsLock input is allowed
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);

  // LeftShift output is blocked when pressed directly
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(StrictModeFilterTest, EmptyConfigBlocksEverything) {
  StrictModeFilter filter; // No keys added

  Context ctx;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  // All keys blocked
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);

  ctx.scancode = SC_SPACE;
  ctx.outputScancode = SC_SPACE;
  filter.process(ctx);
  EXPECT_EQ(ctx.action, Action::Consume);
}
