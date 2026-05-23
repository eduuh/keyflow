#include "hardware/Scancodes.h"
#include "processors/ComboAdvanced.h"
#include <gtest/gtest.h>

using namespace keyflow;

// Documents the two-bucket invariant: ComboAdvanced stores combos in either
// physicalCombos_ (matched against ctx.scancode) or remappedCombos_ (matched
// against ctx.outputScancode). The matchPhysical flag picks the bucket.
// Layer mappings and noModCombos go to physical (the bug-fix from d276816
// for noModCombo double-transformation).

// Physical-key bucket: combo registered with matchPhysical=true matches on
// the pre-Rewire scancode, ignoring whatever Rewire produced.
TEST(ComboAdvancedTest, MatchPhysicalKey) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftAlt), SC_A, SC_LEFT,
                 true);

  Context ctx;
  ctx.scancode = SC_A;       // Physical key
  ctx.outputScancode = SC_B; // After Rewire (different)
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_LEFT);
}

// Remapped-key bucket: combo registered with matchPhysical=false matches on
// the post-Rewire scancode. Used for combos that should fire on the user's
// logical key, not their physical key.
TEST(ComboAdvancedTest, MatchRemappedKey) {
  ComboAdvanced combo;
  combo.addCombo(static_cast<uint32_t>(ModifierBit::LeftCtrl), SC_B, SC_INSERT,
                 false);

  Context ctx;
  ctx.scancode = SC_A;       // Physical
  ctx.outputScancode = SC_B; // After Rewire
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftCtrl);

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Replace);
  EXPECT_EQ(ctx.outputScancode, SC_INSERT);
}

// NoModCombo's blockedModifiers=0xFFFFFFFF semantics: the combo fires ONLY
// when no modifiers are held. If Shift is down, the combo must NOT fire so
// Shift+1 produces ! normally (not whatever the noModCombo says).
TEST(ComboAdvancedTest, NoModComboBlockedByModifier) {
  ComboAdvanced combo;
  combo.addNoModCombo(SC_1, SC_EQUALS, true);

  Context ctx;
  ctx.scancode = SC_1;
  ctx.outputScancode = SC_1;
  ctx.isDown = true;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftShift);
  ctx.action = Action::Forward;

  combo.process(ctx);

  EXPECT_EQ(ctx.action, Action::Forward);
}
