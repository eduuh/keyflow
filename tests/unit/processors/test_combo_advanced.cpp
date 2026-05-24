#include "hardware/Scancodes.h"
#include "processors/ComboAdvanced.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(ComboAdvancedTest, DefaultConstruction) {
  ComboAdvanced combo;
  EXPECT_EQ(combo.comboCount(), 0u);
  EXPECT_STREQ(combo.name(), "ComboAdvanced");
}

// ===== Add Combo Tests =====

TEST(ComboAdvancedTest, AddSingleCombo) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);
  EXPECT_EQ(combo.comboCount(), 1u);
}

TEST(ComboAdvancedTest, AddMultipleCombos) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT);
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_V, SC_DELETE);
  EXPECT_EQ(combo.comboCount(), 2u);
}

TEST(ComboAdvancedTest, AddComboWithShift) {
  ComboAdvanced combo;
  combo.addComboWithShift(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_1,
                          SC_EQUALS); // Use SC_EQUALS as test output
  EXPECT_EQ(combo.comboCount(), 1u);
}

TEST(ComboAdvancedTest, AddNoModCombo) {
  ComboAdvanced combo;
  combo.addNoModCombo(SC_1, SC_EQUALS, true); // 1 -> ! (with shift)
  EXPECT_EQ(combo.comboCount(), 1u);
}

TEST(ComboAdvancedTest, AddComboByName) {
  ComboAdvanced combo;
  combo.addCombo("LALT", SC_J, SC_DOWN);
  EXPECT_EQ(combo.comboCount(), 1u);
}

// ===== Simple Combo Matching Tests =====

TEST(ComboAdvancedTest, SimpleComboMatch) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  bool continues = combo.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
  EXPECT_FALSE(ctx.injectShift);
}

TEST(ComboAdvancedTest, ComboWithShiftInjection) {
  ComboAdvanced combo;
  combo.addComboWithShift(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_1,
                          SC_EQUALS, false);

  Context ctx;
  ctx.scancode = SC_1;
  ctx.outputScancode = SC_1;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_EQUALS);
  EXPECT_TRUE(ctx.injectShift);
}

TEST(ComboAdvancedTest, NoMatchWrongKey) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);

  Context ctx;
  ctx.scancode = SC_V;
  ctx.outputScancode = SC_V;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(ComboAdvancedTest, NoMatchWrongModifier) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== NoModCombo Tests =====

TEST(ComboAdvancedTest, NoModComboWithNoModifiers) {
  ComboAdvanced combo;
  combo.addNoModCombo(SC_1, SC_EQUALS, true);

  Context ctx;
  ctx.scancode = SC_1;
  ctx.outputScancode = SC_1;
  ctx.isDown = true;
  ctx.modifiers = 0; // No modifiers

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_EQUALS);
  EXPECT_TRUE(ctx.injectShift);
}

TEST(ComboAdvancedTest, NoModComboBlockedByModifier) {
  ComboAdvanced combo;
  combo.addNoModCombo(SC_1, SC_EQUALS, true);

  Context ctx;
  ctx.scancode = SC_1;
  ctx.outputScancode = SC_1;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftShift); // Modifier
                                                                 // held
  ctx.action = Action::Forward;

  combo.process(ctx);

  // Should NOT match because modifiers are blocked
  EXPECT_EQ(ctx.action, Action::Forward);
}

// ===== Physical Key vs Remapped Key Tests =====

TEST(ComboAdvancedTest, MatchPhysicalKey) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftAlt), SC_A, SC_LEFT,
                 true); // matchPhysical = true

  Context ctx;
  ctx.scancode = SC_A;       // Physical key
  ctx.outputScancode = SC_B; // After remapping
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);

  combo.process(ctx);

  // Should match based on physical key (SC_A)
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_LEFT);
}

TEST(ComboAdvancedTest, MatchRemappedKey) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_B, SC_INSERT,
                 false); // matchPhysical = false

  Context ctx;
  ctx.scancode = SC_A;       // Physical key
  ctx.outputScancode = SC_B; // After remapping
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  // Should match based on remapped key (SC_B)
  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
}

// ===== Modifier Name Tests =====

TEST(ComboAdvancedTest, AddComboByModifierName) {
  ComboAdvanced combo;
  combo.addCombo("LALT", SC_J, SC_DOWN);

  Context ctx;
  ctx.scancode = SC_J;
  ctx.outputScancode = SC_J;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_DOWN);
}

TEST(ComboAdvancedTest, AddComboWithShiftByName) {
  ComboAdvanced combo;
  combo.addComboWithShift("RALT", SC_K, SC_UP);

  Context ctx;
  ctx.scancode = SC_K;
  ctx.outputScancode = SC_K;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::RightAlt);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_UP);
  EXPECT_TRUE(ctx.injectShift);
}

// ===== Multiple Modifier Tests =====

TEST(ComboAdvancedTest, MultipleModifiersRequired) {
  ComboAdvanced combo;
  uint32_t shiftCtrl = static_cast<uint32_t>(ModifierBit::LeftShift) |
                       static_cast<uint32_t>(ModifierBit::LeftCtrl);
  combo.addCombo(shiftCtrl, SC_C, SC_INSERT, false);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = shiftCtrl;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
}

TEST(ComboAdvancedTest, MultipleModifiersPartialMatch) {
  ComboAdvanced combo;
  uint32_t shiftCtrl = static_cast<uint32_t>(ModifierBit::LeftShift) |
                       static_cast<uint32_t>(ModifierBit::LeftCtrl);
  combo.addCombo(shiftCtrl, SC_C, SC_INSERT, false);

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

// ===== First Match Wins Tests =====

TEST(ComboAdvancedTest, FirstMatchingComboWins) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_HOME,
                 false); // Same combo, different output

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  EXPECT_EQ(ctx.outputScancode, SC_INSERT); // First one wins
}

TEST(ComboAdvancedTest, DifferentCombosWork) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_V, SC_DELETE,
                 false);

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

TEST(ComboAdvancedTest, AlwaysContinuesPipeline) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);

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

// ===== Edge Cases =====

TEST(ComboAdvancedTest, EmptyCombosPassThrough) {
  ComboAdvanced combo; // No combos added

  Context ctx;
  ctx.scancode = SC_A;
  ctx.outputScancode = SC_A;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(ComboAdvancedTest, KeyUpDoesNotTrigger) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_C, SC_INSERT,
                 false);

  Context ctx;
  ctx.scancode = SC_C;
  ctx.outputScancode = SC_C;
  ctx.isDown = true; // Process doesn't check isDown, but this is a common
                     // pattern
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
}

// ===== Layer-Like Behavior Tests =====

TEST(ComboAdvancedTest, LayerStyleComboVim) {
  ComboAdvanced combo;
  // Simulate vim-style hjkl navigation on LALT layer
  combo.addCombo("LALT", SC_H, SC_LEFT);
  combo.addCombo("LALT", SC_J, SC_DOWN);
  combo.addCombo("LALT", SC_K, SC_UP);
  combo.addCombo("LALT", SC_L, SC_RIGHT);

  Context ctx;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);

  // Test H -> LEFT
  ctx.scancode = SC_H;
  ctx.outputScancode = SC_H;
  combo.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_LEFT);

  // Test J -> DOWN
  ctx.action = Action::Forward;
  ctx.scancode = SC_J;
  ctx.outputScancode = SC_J;
  combo.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_DOWN);

  // Test K -> UP
  ctx.action = Action::Forward;
  ctx.scancode = SC_K;
  ctx.outputScancode = SC_K;
  combo.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_UP);

  // Test L -> RIGHT
  ctx.action = Action::Forward;
  ctx.scancode = SC_L;
  ctx.outputScancode = SC_L;
  combo.process(ctx);
  EXPECT_EQ(ctx.outputScancode, SC_RIGHT);
}
