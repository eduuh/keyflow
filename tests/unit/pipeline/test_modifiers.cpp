#include "pipeline/Modifiers.h"
#include <gtest/gtest.h>

using namespace keyflow;

// modifierNameToBit returns None for anything that isn't a known modifier
// name. Boundary behavior: ConfigBuilder relies on a None return to fall
// back to custom-modifier resolution. If this regresses to returning a
// non-zero default, ConfigBuilder would silently pick the wrong bit.
TEST(ModifierNameToBitTest, InvalidNames) {
  EXPECT_EQ(modifierNameToBit(""), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("INVALID"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("SHIFT"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("CTRL"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("ALT"), ModifierBit::None);
}

// modifierNameToBit is case-sensitive. Configs use "LALT" not "lalt" or
// "Lalt"; if this regressed to case-insensitive, a typo in config wouldn't
// fail loudly.
TEST(ModifierNameToBitTest, CaseSensitive) {
  EXPECT_EQ(modifierNameToBit("lalt"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("Lalt"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("LShift"), ModifierBit::None);
}
