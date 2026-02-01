#include "hardware/Scancodes.h"
#include "processors/Combo.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(ComboTest, DefaultConstruction) {
  Combo combo;
  EXPECT_EQ(combo.comboCount(), 0u);
  EXPECT_STREQ(combo.name(), "Combo");
}

// ===== Add Combo Tests =====

TEST(ComboTest, AddSingleCombo) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  EXPECT_EQ(combo.comboCount(), 1u);
}

TEST(ComboTest, AddMultipleCombos) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_V, SC_DELETE);

  EXPECT_EQ(combo.comboCount(), 2u);
}

TEST(ComboTest, AddComboWithModifierName) {
  Combo combo;
  combo.addCombo("LCTRL", SC_C, SC_INSERT);

  EXPECT_EQ(combo.comboCount(), 1u);
}

TEST(ComboTest, AddComboWithInvalidModifierName) {
  Combo combo;
  combo.addCombo("INVALID", SC_C, SC_INSERT);

  EXPECT_EQ(combo.comboCount(), 0u); // Should not be added
}

// ===== Simple Combo Matching Tests =====

TEST(ComboTest, SimpleComboMatch) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  bool continues = combo.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
}

TEST(ComboTest, NoMatchWrongKey) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_V;
  ctx.outputScancode = SC_V;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_EQ(ctx.outputScancode, SC_V);
}

TEST(ComboTest, NoMatchWrongModifier) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(ComboTest, NoMatchNoModifier) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = 0;
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== Key Up Tests =====

TEST(ComboTest, KeyUpPassesThrough) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = false; // Key up
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);
  ctx.action = Action::Forward;

  bool continues = combo.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== Multiple Modifiers Tests =====

TEST(ComboTest, MultipleModifiersRequired) {
  Combo combo;
  uint32_t shiftCtrl = static_cast<uint32_t>(ModifierBit::LeftShift) |
                       static_cast<uint32_t>(ModifierBit::LeftCtrl);
  combo.addCombo(shiftCtrl, SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = shiftCtrl;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
}

TEST(ComboTest, MultipleModifiersPartialMatch) {
  Combo combo;
  uint32_t shiftCtrl = static_cast<uint32_t>(ModifierBit::LeftShift) |
                       static_cast<uint32_t>(ModifierBit::LeftCtrl);
  combo.addCombo(shiftCtrl, SC_C, SC_INSERT);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers =
      static_cast<uint32_t>(ModifierBit::LeftCtrl); // Only Ctrl, missing Shift
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward); // Should not match
}

// ===== Modifier-Only Trigger Tests =====

TEST(ComboTest, ModifierOnlyTrigger) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftAlt), SC_K, SC_LEFT);

  Context ctx;
  ctx.scancode = SC_K;
  ctx.outputScancode = SC_K;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_LEFT);
}

// ===== Multiple Combo Selection Tests =====

TEST(ComboTest, FirstMatchingComboWins) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C,
                 SC_HOME); // Same combo, different output

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  EXPECT_EQ(ctx.outputScancode, SC_INSERT); // First one wins
}

TEST(ComboTest, DifferentCombosWork) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_V, SC_DELETE);

  Context ctx;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  // Test first combo
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.action = Action::Forward;
  combo.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);

  // Test second combo
  ctx.scancode = SC_V;
  ctx.outputScancode = SC_V;
  ctx.action = Action::Forward;
  combo.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_DELETE);
}

// ===== Always Continues Tests =====

TEST(ComboTest, AlwaysContinuesPipeline) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);

  Context ctx;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  // Test with match
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  EXPECT_TRUE(combo.process(ctx));

  // Test without match
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.action = Action::Forward;
  EXPECT_TRUE(combo.process(ctx));
}

// ===== Uses outputScancode Tests =====

TEST(ComboTest, UsesOutputScancodeAfterRewire) {
  Combo combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_ESCAPE,
                 SC_INSERT);

  Context ctx;
  ctx.scancode = SC_CAPSLOCK;     // Original key
  ctx.outputScancode = SC_ESCAPE; // After Rewire (CapsLock → Escape)
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  // Should match based on outputScancode (ESCAPE), not scancode (CAPSLOCK)
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
}

// ===== Edge Cases =====

TEST(ComboTest, EmptyCombosPassThrough) {
  Combo combo; // No combos added

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}
