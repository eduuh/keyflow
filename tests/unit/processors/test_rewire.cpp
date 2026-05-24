#include "hardware/Scancodes.h"
#include "processors/Rewire.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(RewireTest, DefaultConstruction) {
  Rewire rewire;
  EXPECT_STREQ(rewire.name(), "Rewire");
}

TEST(RewireTest, NoMappingConstant) { EXPECT_EQ(Rewire::NO_MAPPING, 0xFFFF); }

// ===== Unmapped Keys Tests =====

TEST(RewireTest, UnmappedKeyForwardsUnchanged) {
  Rewire rewire;
  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = rewire.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_EQ(ctx.outputScancode, SC_A);
}

TEST(RewireTest, MultipleUnmappedKeys) {
  Rewire rewire;
  Context ctx;

  // Test several keys
  const uint16_t keys[] = {SC_A, SC_B, SC_C, SC_SPACE, SC_ENTER, SC_ESCAPE};

  for (uint16_t key : keys) {
    ctx.scancode = key;
    ctx.outputScancode = key;
    ctx.action = Action::Forward;

    rewire.process(ctx);

    EXPECT_EQ(ctx.action, Action::Forward);
    EXPECT_EQ(ctx.outputScancode, key);
  }
}

// ===== Simple Remapping Tests =====

TEST(RewireTest, SimpleRemapAtoB) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;

  bool continues = rewire.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_B);
}

TEST(RewireTest, CapsLockToEscape) {
  Rewire rewire;
  rewire.setMapping(SC_CAPSLOCK, SC_ESCAPE);

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  ctx.isDown = true;

  rewire.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_ESCAPE);
}

TEST(RewireTest, RemapWorksForKeyDown) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;

  rewire.process(ctx);

  EXPECT_EQ(ctx.outputScancode, SC_B);
}

TEST(RewireTest, RemapWorksForKeyUp) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = false;

  rewire.process(ctx);

  EXPECT_EQ(ctx.outputScancode, SC_B);
}

// ===== Bidirectional Remapping Tests =====

TEST(RewireTest, BidirectionalSwap) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);
  rewire.setMapping(SC_B, SC_A);

  Context ctx;

  // Test A -> B
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_B);

  // Reset context
  ctx.action = Action::Forward;

  // Test B -> A
  ctx.scancode = SC_B;
  ctx.outputScancode = SC_B;
  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_A);
}

// ===== Multiple Mappings Tests =====

TEST(RewireTest, MultipleMappings) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);
  rewire.setMapping(SC_C, SC_D);
  rewire.setMapping(SC_E, SC_F);

  Context ctx;

  // Test A -> B
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_B);

  ctx.action = Action::Forward;

  // Test C -> D
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_D);

  ctx.action = Action::Forward;

  // Test E -> F
  ctx.scancode = SC_E;
  ctx.outputScancode = SC_E;
  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_F);

  ctx.action = Action::Forward;

  // Test unmapped key
  ctx.scancode = SC_G;
  ctx.outputScancode = SC_G;
  rewire.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_EQ(ctx.outputScancode, SC_G);
}

// ===== Clear Mapping Tests =====

TEST(RewireTest, ClearMapping) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;

  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_B);

  // Clear the mapping
  rewire.clearMapping(SC_A);

  ctx.action = Action::Forward;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;

  rewire.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_EQ(ctx.outputScancode, SC_A);
}

TEST(RewireTest, ClearUnmappedKey) {
  Rewire rewire;
  rewire.clearMapping(SC_A); // Clear a key that was never mapped

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;

  rewire.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== Invalid Scancode Tests =====

TEST(RewireTest, OutOfRangeScancode) {
  Rewire rewire;
  Context ctx;
  ctx.scancode = 0xFFFF; // Way out of range
  ctx.outputScancode = 0xFFFF;
  ctx.action = Action::Forward;

  bool continues = rewire.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(RewireTest, MaxScancodeMapping) {
  // 0xFFFF is the max uint16_t value but is a valid index in the 65536-element
  // array
  Rewire rewire;
  rewire.setMapping(0xFFFF, SC_A);

  Context ctx;
  ctx.scancode = 0xFFFF;
  ctx.outputScancode = 0xFFFF;

  rewire.process(ctx);
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_A);
}

TEST(RewireTest, ClearMaxScancode) {
  Rewire rewire;
  rewire.setMapping(0xFFFF, SC_A);
  rewire.clearMapping(0xFFFF);

  Context ctx;
  ctx.scancode = 0xFFFF;
  ctx.outputScancode = 0xFFFF;

  rewire.process(ctx);
  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== Overwrite Mapping Tests =====

TEST(RewireTest, OverwriteMapping) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;

  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_B);

  // Overwrite with new mapping
  rewire.setMapping(SC_A, SC_C);

  ctx.action = Action::Forward;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;

  rewire.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_C);
}

// ===== Always Continues Tests =====

TEST(RewireTest, AlwaysContinuesPipeline) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_B);

  Context ctx;

  // Test with mapped key
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  EXPECT_TRUE(rewire.process(ctx));

  // Test with unmapped key
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  EXPECT_TRUE(rewire.process(ctx));
}

// ===== Edge Cases =====

TEST(RewireTest, RemapToSameKey) {
  Rewire rewire;
  rewire.setMapping(SC_A, SC_A);

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;

  rewire.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_A);
}

TEST(RewireTest, ZeroScancode) {
  Rewire rewire;
  rewire.setMapping(0, SC_A);

  Context ctx;
  ctx.scancode = 0;
  ctx.outputScancode = 0;

  rewire.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_A);
}
