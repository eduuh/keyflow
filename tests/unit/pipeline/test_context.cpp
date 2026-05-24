#include "hardware/KeyEvent.h"
#include "hardware/Scancodes.h"
#include "pipeline/Context.h"
#include "pipeline/Modifiers.h"
#include <gtest/gtest.h>

using namespace keyflow;

TEST(ContextTest, InitializeCopiesEventAndDefaultsOutput) {
  Context ctx;
  KeyEvent event(SC_A, true);

  ctx.initialize(event);

  EXPECT_EQ(ctx.scancode, SC_A);
  EXPECT_TRUE(ctx.isDown);
  EXPECT_EQ(ctx.outputScancode, SC_A);
  EXPECT_EQ(ctx.action, Action::Forward);
}

TEST(ContextTest, InitializeResetsPipelineState) {
  Context ctx;
  ctx.tapped = true;
  ctx.tappedSlow = true;
  ctx.tapHoldMake = true;
  ctx.repeat = true;
  ctx.modifiers = 0xFFFFFFFF;
  ctx.action = Action::Consume;
  ctx.injectShift = true;

  ctx.initialize(KeyEvent(SC_B, false));

  EXPECT_FALSE(ctx.tapped);
  EXPECT_FALSE(ctx.tappedSlow);
  EXPECT_FALSE(ctx.tapHoldMake);
  EXPECT_FALSE(ctx.repeat);
  EXPECT_EQ(ctx.modifiers, 0u);
  EXPECT_EQ(ctx.action, Action::Forward);
  EXPECT_FALSE(ctx.injectShift);
}

TEST(ContextTest, HasModifierMatchesExactBits) {
  Context ctx;
  ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftShift) |
                  static_cast<uint32_t>(ModifierBit::LeftCtrl);

  EXPECT_TRUE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftShift)));
  EXPECT_TRUE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftCtrl)));
  EXPECT_FALSE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::LeftAlt)));
  EXPECT_FALSE(ctx.hasModifier(static_cast<uint32_t>(ModifierBit::RightShift)));
}

TEST(ContextTest, IsKeyDownReflectsEvent) {
  Context ctx;
  ctx.initialize(KeyEvent(SC_A, true));
  EXPECT_TRUE(ctx.isKeyDown());
  EXPECT_FALSE(ctx.isKeyUp());

  ctx.initialize(KeyEvent(SC_B, false));
  EXPECT_FALSE(ctx.isKeyDown());
  EXPECT_TRUE(ctx.isKeyUp());
}
