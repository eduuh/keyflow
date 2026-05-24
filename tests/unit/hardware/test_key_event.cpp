#include "hardware/KeyEvent.h"
#include "hardware/Scancodes.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Construction Tests =====

TEST(KeyEventTest, DefaultConstruction) {
  KeyEvent event;

  EXPECT_EQ(event.scancode, 0);
  EXPECT_FALSE(event.isDown);
}

TEST(KeyEventTest, ParameterizedConstruction) {
  KeyEvent event(SC_A, true);

  EXPECT_EQ(event.scancode, SC_A);
  EXPECT_TRUE(event.isDown);
}

TEST(KeyEventTest, ConstructionWithKeyUp) {
  KeyEvent event(SC_ESCAPE, false);

  EXPECT_EQ(event.scancode, SC_ESCAPE);
  EXPECT_FALSE(event.isDown);
}

TEST(KeyEventTest, ConstructionWithExtendedKey) {
  KeyEvent event(0x0200, true); // Extended scancode

  EXPECT_EQ(event.scancode, 0x0200);
  EXPECT_TRUE(event.isDown);
}

// ===== Constexpr Tests =====

TEST(KeyEventTest, ConstexprConstruction) {
  constexpr KeyEvent event(SC_SPACE, true);

  static_assert(event.scancode == SC_SPACE, "Constexpr construction failed");
  static_assert(event.isDown == true, "Constexpr construction failed");
}

TEST(KeyEventTest, ConstexprDefaultConstruction) {
  constexpr KeyEvent event;

  static_assert(event.scancode == 0, "Constexpr default construction failed");
  static_assert(event.isDown == false, "Constexpr default construction failed");
}

// ===== Field Access Tests =====

TEST(KeyEventTest, ScancodeField) {
  KeyEvent event;
  event.scancode = SC_ENTER;

  EXPECT_EQ(event.scancode, SC_ENTER);
}

TEST(KeyEventTest, IsDownField) {
  KeyEvent event;
  event.isDown = true;

  EXPECT_TRUE(event.isDown);

  event.isDown = false;
  EXPECT_FALSE(event.isDown);
}

// ===== Compile-Time Safety Tests =====

TEST(KeyEventTest, IsTriviallyCopyable) {
  EXPECT_TRUE(std::is_trivially_copyable_v<KeyEvent>);
}

TEST(KeyEventTest, SizeConstraint) { EXPECT_LE(sizeof(KeyEvent), 8u); }

TEST(KeyEventTest, StaticAsserts) {
  // These are compile-time checks
  // If they fail, compilation will fail
  SUCCEED();
}

// ===== Edge Cases =====

TEST(KeyEventTest, MaxScancode) {
  KeyEvent event(0xFFFF, true);

  EXPECT_EQ(event.scancode, 0xFFFF);
}

TEST(KeyEventTest, ZeroScancode) {
  KeyEvent event(0, false);

  EXPECT_EQ(event.scancode, 0);
}

// ===== Copy Tests =====

TEST(KeyEventTest, CopyConstruction) {
  KeyEvent original(SC_B, true);
  KeyEvent copy(original);

  EXPECT_EQ(copy.scancode, SC_B);
  EXPECT_TRUE(copy.isDown);
}

TEST(KeyEventTest, CopyAssignment) {
  KeyEvent original(SC_C, false);
  KeyEvent copy;

  copy = original;

  EXPECT_EQ(copy.scancode, SC_C);
  EXPECT_FALSE(copy.isDown);
}
