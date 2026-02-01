#include "hardware/KeyEvent.h"
#include "hardware/Scancodes.h"
#include "pipeline/Context.h"
#include "pipeline/Modifiers.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Context Construction Tests =====

TEST(ContextTest, DefaultConstruction) {
  Context ctx;

  // Check defaults for output fields
  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_EQ(ctx.outputScancode, 0);
  EXPECT_FALSE(ctx.injectShift);

  // Check defaults for state fields
  EXPECT_FALSE(ctx.tapped);
  EXPECT_FALSE(ctx.tappedSlow);
  EXPECT_FALSE(ctx.tapHoldMake);
  EXPECT_FALSE(ctx.repeat);
  EXPECT_EQ(ctx.modifiers, 0u);
}

// ===== initialize() Method Tests =====

TEST(ContextTest, InitializeFromKeyEvent) {
  Context ctx;
  KeyEvent event(SC_A, true);

  ctx.initialize(event);

  EXPECT_EQ(ctx.scancode, SC_A);
  EXPECT_TRUE(ctx.isDown);
  EXPECT_EQ(ctx.outputScancode, SC_A);
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(ContextTest, InitializeResetsStateFields) {
  Context ctx;
  // Set some non-default values
  ctx.tapped = true;
  ctx.tappedSlow = true;
  ctx.tapHoldMake = true;
  ctx.repeat = true;
  ctx.modifiers = 0xFFFFFFFF;
  ctx.action = Action::Consume;
  ctx.injectShift = true;

  KeyEvent event(SC_B, false);
  ctx.initialize(event);

  // Verify all state is reset
  EXPECT_FALSE(ctx.tapped);
  EXPECT_FALSE(ctx.tappedSlow);
  EXPECT_FALSE(ctx.tapHoldMake);
  EXPECT_FALSE(ctx.repeat);
  EXPECT_EQ(ctx.modifiers, 0u);
  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_FALSE(ctx.injectShift);
}

TEST(ContextTest, InitializeWithKeyDown) {
  Context ctx;
  KeyEvent event(SC_SPACE, true);

  ctx.initialize(event);

  EXPECT_EQ(ctx.scancode, SC_SPACE);
  EXPECT_TRUE(ctx.isDown);
}

TEST(ContextTest, InitializeWithKeyUp) {
  Context ctx;
  KeyEvent event(SC_ENTER, false);

  ctx.initialize(event);

  EXPECT_EQ(ctx.scancode, SC_ENTER);
  EXPECT_FALSE(ctx.isDown);
}

TEST(ContextTest, InitializeMultipleTimes) {
  Context ctx;

  KeyEvent event1(SC_A, true);
  ctx.initialize(event1);
  EXPECT_EQ(ctx.scancode, SC_A);
  EXPECT_TRUE(ctx.isDown);

  KeyEvent event2(SC_B, false);
  ctx.initialize(event2);
  EXPECT_EQ(ctx.scancode, SC_B);
  EXPECT_FALSE(ctx.isDown);
}

// ===== Helper Methods Tests =====

TEST(ContextTest, IsKeyDown) {
  Context ctx;
  KeyEvent downEvent(SC_A, true);
  ctx.initialize(downEvent);
  EXPECT_TRUE(ctx.isKeyDown());

  KeyEvent upEvent(SC_B, false);
  ctx.initialize(upEvent);
  EXPECT_FALSE(ctx.isKeyDown());
}

TEST(ContextTest, IsKeyUp) {
  Context ctx;
  KeyEvent downEvent(SC_A, true);
  ctx.initialize(downEvent);
  EXPECT_FALSE(ctx.isKeyUp());

  KeyEvent upEvent(SC_B, false);
  ctx.initialize(upEvent);
  EXPECT_TRUE(ctx.isKeyUp());
}

TEST(ContextTest, HasModifier) {
  Context ctx;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftShift) |
                  static_cast<uint32_t>(ModifierBit::LeftCtrl);

  EXPECT_TRUE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_TRUE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
  EXPECT_FALSE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftAlt)));
  EXPECT_FALSE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::RightShift)));
}

TEST(ContextTest, HasModifierWithNoModifiers) {
  Context ctx;
  ctx.modifiers = 0;

  EXPECT_FALSE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_FALSE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
}

TEST(ContextTest, HasModifierConstexpr) {
  Context ctx;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt);

  constexpr uint32_t altBit = static_cast<uint32_t>(ModifierBit::LeftAlt);
  EXPECT_TRUE(ctx.hasModifier(altBit));
}

// ===== Action Enum Tests =====

TEST(ActionEnumTest, EnumValues) {
  EXPECT_NE(Action::Forward, Action::Replace);
  EXPECT_NE(Action::Forward, Action::Consume);
  EXPECT_NE(Action::Replace, Action::Consume);
}

// ===== State Field Tests =====

TEST(ContextStateTest, TappedField) {
  Context ctx;
  EXPECT_FALSE(ctx.tapped);

  ctx.tapped = true;
  EXPECT_TRUE(ctx.tapped);
}

TEST(ContextStateTest, ModifiersField) {
  Context ctx;
  EXPECT_EQ(ctx.modifiers, 0u);

  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftShift);
  EXPECT_EQ(ctx.modifiers, static_cast<uint32_t>(ModifierBit::LeftShift));

  ctx.modifiers |= static_cast<uint32_t>(ModifierBit::LeftCtrl);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftShift), 0u);
  EXPECT_NE(ctx.modifiers & static_cast<uint32_t>(ModifierBit::LeftCtrl), 0u);
}

// ===== Output Field Tests =====

TEST(ContextOutputTest, ActionField) {
  Context ctx;
  EXPECT_EQ(ctx.action, Action::Forward);

  ctx.action = Action::Replace;
  EXPECT_EQ(ctx.action, Action::Replace);

  ctx.action = Action::Consume;
  EXPECT_EQ(ctx.action, Action::Consume);
}

TEST(ContextOutputTest, OutputScancodeField) {
  Context ctx;
  KeyEvent event(SC_A, true);
  ctx.initialize(event);

  EXPECT_EQ(ctx.outputScancode, SC_A);

  ctx.outputScancode = SC_B;
  EXPECT_EQ(ctx.outputScancode, SC_B);
}

TEST(ContextOutputTest, InjectShiftField) {
  Context ctx;
  EXPECT_FALSE(ctx.injectShift);

  ctx.injectShift = true;
  EXPECT_TRUE(ctx.injectShift);
}

// ===== Compile-Time Safety Tests =====

TEST(ContextCompileTimeTest, IsTriviallyCopyable) {
  EXPECT_TRUE(std::is_trivially_copyable_v<Context>);
}

TEST(ContextCompileTimeTest, SizeFitsInCacheLine) {
  EXPECT_LE(sizeof(Context), 64u);
}

TEST(ContextCompileTimeTest, StaticAsserts) {
  // These are compile-time checks, if they fail, compilation will fail
  // Just ensuring the test file compiles confirms these work
  SUCCEED();
}
