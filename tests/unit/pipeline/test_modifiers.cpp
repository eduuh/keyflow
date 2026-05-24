#include "hardware/Scancodes.h"
#include "pipeline/Modifiers.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== ModifierBit Enum Tests =====

TEST(ModifierBitTest, EnumValues) {
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::LeftShift), 1u << 0);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::RightShift), 1u << 1);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::LeftCtrl), 1u << 2);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::RightCtrl), 1u << 3);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::LeftAlt), 1u << 4);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::RightAlt), 1u << 5);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::LeftWin), 1u << 6);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::RightWin), 1u << 7);
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::PrintScreen), 1u << 8);
}

TEST(ModifierBitTest, NoneValue) {
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::None), 0u);
}

TEST(ModifierBitTest, ConvenienceAliases) {
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::AnyShift),
            static_cast<uint32_t>(ModifierBit::LeftShift) |
                static_cast<uint32_t>(ModifierBit::RightShift));
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::AnyCtrl),
            static_cast<uint32_t>(ModifierBit::LeftCtrl) |
                static_cast<uint32_t>(ModifierBit::RightCtrl));
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::AnyAlt),
            static_cast<uint32_t>(ModifierBit::LeftAlt) |
                static_cast<uint32_t>(ModifierBit::RightAlt));
  EXPECT_EQ(static_cast<uint32_t>(ModifierBit::AnyWin),
            static_cast<uint32_t>(ModifierBit::LeftWin) |
                static_cast<uint32_t>(ModifierBit::RightWin));
}

// ===== Bitwise Operators Tests =====

TEST(ModifierBitOperatorsTest, BitwiseOr) {
  uint32_t result = ModifierBit::LeftShift | ModifierBit::LeftCtrl;
  EXPECT_EQ(result, (1u << 0) | (1u << 2));

  result = ModifierBit::LeftAlt | ModifierBit::RightAlt;
  EXPECT_EQ(result, (1u << 4) | (1u << 5));
}

TEST(ModifierBitOperatorsTest, BitwiseAnd) {
  uint32_t mask = (1u << 0) | (1u << 2);
  EXPECT_EQ(mask & ModifierBit::LeftShift, 1u << 0);
  EXPECT_EQ(mask & ModifierBit::LeftCtrl, 1u << 2);
  EXPECT_EQ(mask & ModifierBit::LeftAlt, 0u);
}

TEST(ModifierBitOperatorsTest, BitwiseOrAssignment) {
  uint32_t mask = 0;
  mask |= ModifierBit::LeftShift;
  EXPECT_EQ(mask, 1u << 0);

  mask |= ModifierBit::LeftCtrl;
  EXPECT_EQ(mask, (1u << 0) | (1u << 2));

  mask |= ModifierBit::RightShift;
  EXPECT_EQ(mask, (1u << 0) | (1u << 1) | (1u << 2));
}

TEST(ModifierBitOperatorsTest, BitwiseAndAssignment) {
  uint32_t mask = (1u << 0) | (1u << 2) | (1u << 4);
  mask &= ModifierBit::LeftShift;
  EXPECT_EQ(mask, 1u << 0);

  mask = (1u << 0) | (1u << 2);
  mask &= ModifierBit::LeftCtrl;
  EXPECT_EQ(mask, 1u << 2);
}

TEST(ModifierBitOperatorsTest, ComplexCombinations) {
  uint32_t mask = ModifierBit::LeftShift | ModifierBit::LeftCtrl;
  mask |= ModifierBit::LeftAlt;
  EXPECT_EQ(mask, (1u << 0) | (1u << 2) | (1u << 4));

  EXPECT_NE(mask & ModifierBit::LeftShift, 0u);
  EXPECT_NE(mask & ModifierBit::LeftCtrl, 0u);
  EXPECT_NE(mask & ModifierBit::LeftAlt, 0u);
  EXPECT_EQ(mask & ModifierBit::RightShift, 0u);
}

TEST(ModifierBitOperatorsTest, ConstexprEvaluation) {
  constexpr uint32_t result = ModifierBit::LeftShift | ModifierBit::LeftCtrl;
  static_assert(result == ((1u << 0) | (1u << 2)),
                "Constexpr evaluation failed");
}

// ===== isModifierKey Tests =====

TEST(IsModifierKeyTest, AllModifierKeys) {
  EXPECT_TRUE(isModifierKey(SC_LSHIFT));
  EXPECT_TRUE(isModifierKey(SC_RSHIFT));
  EXPECT_TRUE(isModifierKey(SC_LCTRL));
  EXPECT_TRUE(isModifierKey(SC_RCTRL));
  EXPECT_TRUE(isModifierKey(SC_LALT));
  EXPECT_TRUE(isModifierKey(SC_RALT));
  EXPECT_TRUE(isModifierKey(SC_LWIN));
  EXPECT_TRUE(isModifierKey(SC_RWIN));
  EXPECT_TRUE(isModifierKey(SC_PRINTSCREEN));
}

TEST(IsModifierKeyTest, NonModifierKeys) {
  EXPECT_FALSE(isModifierKey(SC_A));
  EXPECT_FALSE(isModifierKey(SC_ESCAPE));
  EXPECT_FALSE(isModifierKey(SC_SPACE));
  EXPECT_FALSE(isModifierKey(SC_ENTER));
  EXPECT_FALSE(isModifierKey(SC_CAPSLOCK));
  EXPECT_FALSE(isModifierKey(0));
  EXPECT_FALSE(isModifierKey(0xFFFF));
}

TEST(IsModifierKeyTest, Constexpr) {
  static_assert(isModifierKey(SC_LSHIFT), "Constexpr evaluation failed");
  static_assert(!isModifierKey(SC_A), "Constexpr evaluation failed");
}

// ===== getModifierBit Tests =====

TEST(GetModifierBitTest, AllModifiers) {
  EXPECT_EQ(getModifierBit(SC_LSHIFT), ModifierBit::LeftShift);
  EXPECT_EQ(getModifierBit(SC_RSHIFT), ModifierBit::RightShift);
  EXPECT_EQ(getModifierBit(SC_LCTRL), ModifierBit::LeftCtrl);
  EXPECT_EQ(getModifierBit(SC_RCTRL), ModifierBit::RightCtrl);
  EXPECT_EQ(getModifierBit(SC_LALT), ModifierBit::LeftAlt);
  EXPECT_EQ(getModifierBit(SC_RALT), ModifierBit::RightAlt);
  EXPECT_EQ(getModifierBit(SC_LWIN), ModifierBit::LeftWin);
  EXPECT_EQ(getModifierBit(SC_RWIN), ModifierBit::RightWin);
  EXPECT_EQ(getModifierBit(SC_PRINTSCREEN), ModifierBit::PrintScreen);
}

TEST(GetModifierBitTest, NonModifierKeys) {
  EXPECT_EQ(getModifierBit(SC_A), ModifierBit::None);
  EXPECT_EQ(getModifierBit(SC_ESCAPE), ModifierBit::None);
  EXPECT_EQ(getModifierBit(SC_SPACE), ModifierBit::None);
  EXPECT_EQ(getModifierBit(0), ModifierBit::None);
  EXPECT_EQ(getModifierBit(0xFFFF), ModifierBit::None);
}

TEST(GetModifierBitTest, Constexpr) {
  static_assert(getModifierBit(SC_LSHIFT) == ModifierBit::LeftShift,
                "Constexpr evaluation failed");
  static_assert(getModifierBit(SC_A) == ModifierBit::None,
                "Constexpr evaluation failed");
}

// ===== modifierNameToBit Tests =====

TEST(ModifierNameToBitTest, LeftAltVariants) {
  EXPECT_EQ(modifierNameToBit("LALT"), ModifierBit::LeftAlt);
  EXPECT_EQ(modifierNameToBit("MOD12"), ModifierBit::LeftAlt);
}

TEST(ModifierNameToBitTest, RightAltVariants) {
  EXPECT_EQ(modifierNameToBit("RALT"), ModifierBit::RightAlt);
  EXPECT_EQ(modifierNameToBit("MOD11"), ModifierBit::RightAlt);
}

TEST(ModifierNameToBitTest, LeftCtrlVariants) {
  EXPECT_EQ(modifierNameToBit("LCTRL"), ModifierBit::LeftCtrl);
  EXPECT_EQ(modifierNameToBit("MOD13"), ModifierBit::LeftCtrl);
}

TEST(ModifierNameToBitTest, AllStandardNames) {
  EXPECT_EQ(modifierNameToBit("LSHIFT"), ModifierBit::LeftShift);
  EXPECT_EQ(modifierNameToBit("RSHIFT"), ModifierBit::RightShift);
  EXPECT_EQ(modifierNameToBit("RCTRL"), ModifierBit::RightCtrl);
  EXPECT_EQ(modifierNameToBit("LWIN"), ModifierBit::LeftWin);
  EXPECT_EQ(modifierNameToBit("RWIN"), ModifierBit::RightWin);
  EXPECT_EQ(modifierNameToBit("PRINT"), ModifierBit::PrintScreen);
}

TEST(ModifierNameToBitTest, InvalidNames) {
  EXPECT_EQ(modifierNameToBit(""), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("INVALID"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("SHIFT"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("CTRL"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("ALT"), ModifierBit::None);
}

TEST(ModifierNameToBitTest, CaseSensitive) {
  EXPECT_EQ(modifierNameToBit("lalt"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("Lalt"), ModifierBit::None);
  EXPECT_EQ(modifierNameToBit("LShift"), ModifierBit::None);
}

TEST(ModifierNameToBitTest, Constexpr) {
  static_assert(modifierNameToBit("LALT") == ModifierBit::LeftAlt,
                "Constexpr evaluation failed");
  static_assert(modifierNameToBit("INVALID") == ModifierBit::None,
                "Constexpr evaluation failed");
}

// ===== Helper Functions Tests =====

TEST(HasAnyShiftTest, SingleShiftKeys) {
  uint32_t leftShift = static_cast<uint32_t>(ModifierBit::LeftShift);
  uint32_t rightShift = static_cast<uint32_t>(ModifierBit::RightShift);

  EXPECT_TRUE(hasAnyShift(leftShift));
  EXPECT_TRUE(hasAnyShift(rightShift));
  EXPECT_TRUE(hasAnyShift(leftShift | rightShift));
}

TEST(HasAnyShiftTest, NoShift) {
  EXPECT_FALSE(hasAnyShift(0));
  EXPECT_FALSE(hasAnyShift(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
  EXPECT_FALSE(hasAnyShift(static_cast<uint32_t>(ModifierBit::LeftAlt)));
}

TEST(HasAnyShiftTest, ShiftWithOtherModifiers) {
  uint32_t shiftCtrl = static_cast<uint32_t>(ModifierBit::LeftShift) |
                       static_cast<uint32_t>(ModifierBit::LeftCtrl);
  EXPECT_TRUE(hasAnyShift(shiftCtrl));
}

TEST(HasAnyShiftTest, Constexpr) {
  static_assert(hasAnyShift(static_cast<uint32_t>(ModifierBit::LeftShift)),
                "Constexpr evaluation failed");
  static_assert(!hasAnyShift(0), "Constexpr evaluation failed");
}

TEST(HasAnyCtrlTest, SingleCtrlKeys) {
  uint32_t leftCtrl = static_cast<uint32_t>(ModifierBit::LeftCtrl);
  uint32_t rightCtrl = static_cast<uint32_t>(ModifierBit::RightCtrl);

  EXPECT_TRUE(hasAnyCtrl(leftCtrl));
  EXPECT_TRUE(hasAnyCtrl(rightCtrl));
  EXPECT_TRUE(hasAnyCtrl(leftCtrl | rightCtrl));
}

TEST(HasAnyCtrlTest, NoCtrl) {
  EXPECT_FALSE(hasAnyCtrl(0));
  EXPECT_FALSE(hasAnyCtrl(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_FALSE(hasAnyCtrl(static_cast<uint32_t>(ModifierBit::LeftAlt)));
}

TEST(HasAnyCtrlTest, Constexpr) {
  static_assert(hasAnyCtrl(static_cast<uint32_t>(ModifierBit::LeftCtrl)),
                "Constexpr evaluation failed");
  static_assert(!hasAnyCtrl(0), "Constexpr evaluation failed");
}

TEST(HasAnyAltTest, SingleAltKeys) {
  uint32_t leftAlt = static_cast<uint32_t>(ModifierBit::LeftAlt);
  uint32_t rightAlt = static_cast<uint32_t>(ModifierBit::RightAlt);

  EXPECT_TRUE(hasAnyAlt(leftAlt));
  EXPECT_TRUE(hasAnyAlt(rightAlt));
  EXPECT_TRUE(hasAnyAlt(leftAlt | rightAlt));
}

TEST(HasAnyAltTest, NoAlt) {
  EXPECT_FALSE(hasAnyAlt(0));
  EXPECT_FALSE(hasAnyAlt(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_FALSE(hasAnyAlt(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
}

TEST(HasAnyAltTest, Constexpr) {
  static_assert(hasAnyAlt(static_cast<uint32_t>(ModifierBit::LeftAlt)),
                "Constexpr evaluation failed");
  static_assert(!hasAnyAlt(0), "Constexpr evaluation failed");
}

TEST(HasAnyWinTest, SingleWinKeys) {
  uint32_t leftWin = static_cast<uint32_t>(ModifierBit::LeftWin);
  uint32_t rightWin = static_cast<uint32_t>(ModifierBit::RightWin);

  EXPECT_TRUE(hasAnyWin(leftWin));
  EXPECT_TRUE(hasAnyWin(rightWin));
  EXPECT_TRUE(hasAnyWin(leftWin | rightWin));
}

TEST(HasAnyWinTest, NoWin) {
  EXPECT_FALSE(hasAnyWin(0));
  EXPECT_FALSE(hasAnyWin(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_FALSE(hasAnyWin(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
}

TEST(HasAnyWinTest, Constexpr) {
  static_assert(hasAnyWin(static_cast<uint32_t>(ModifierBit::LeftWin)),
                "Constexpr evaluation failed");
  static_assert(!hasAnyWin(0), "Constexpr evaluation failed");
}

TEST(HasNoModifiersTest, NoModifiers) {
  EXPECT_TRUE(hasNoModifiers(0));
  EXPECT_TRUE(hasNoModifiers(static_cast<uint32_t>(ModifierBit::None)));
}

TEST(HasNoModifiersTest, WithModifiers) {
  EXPECT_FALSE(hasNoModifiers(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_FALSE(hasNoModifiers(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
  EXPECT_FALSE(hasNoModifiers(static_cast<uint32_t>(ModifierBit::LeftAlt)));
  EXPECT_FALSE(hasNoModifiers(ModifierBit::LeftShift | ModifierBit::LeftCtrl));
}

TEST(HasNoModifiersTest, Constexpr) {
  static_assert(hasNoModifiers(0), "Constexpr evaluation failed");
  static_assert(!hasNoModifiers(static_cast<uint32_t>(ModifierBit::LeftShift)),
                "Constexpr evaluation failed");
}
