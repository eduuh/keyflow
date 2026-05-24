#include "config/KeyNameMapper.h"
#include "hardware/Scancodes.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== Name to Scancode Tests =====

TEST(KeyNameMapperTest, FunctionKeys) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("Escape"), SC_ESCAPE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F1"), SC_F1);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F2"), SC_F2);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F3"), SC_F3);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F4"), SC_F4);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F5"), SC_F5);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F6"), SC_F6);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F7"), SC_F7);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F8"), SC_F8);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F9"), SC_F9);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F10"), SC_F10);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F11"), SC_F11);
  EXPECT_EQ(KeyNameMapper::nameToScancode("F12"), SC_F12);
}

TEST(KeyNameMapperTest, NumberRow) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("1"), SC_1);
  EXPECT_EQ(KeyNameMapper::nameToScancode("2"), SC_2);
  EXPECT_EQ(KeyNameMapper::nameToScancode("3"), SC_3);
  EXPECT_EQ(KeyNameMapper::nameToScancode("4"), SC_4);
  EXPECT_EQ(KeyNameMapper::nameToScancode("5"), SC_5);
  EXPECT_EQ(KeyNameMapper::nameToScancode("6"), SC_6);
  EXPECT_EQ(KeyNameMapper::nameToScancode("7"), SC_7);
  EXPECT_EQ(KeyNameMapper::nameToScancode("8"), SC_8);
  EXPECT_EQ(KeyNameMapper::nameToScancode("9"), SC_9);
  EXPECT_EQ(KeyNameMapper::nameToScancode("0"), SC_0);
}

TEST(KeyNameMapperTest, LetterKeys) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("A"), SC_A);
  EXPECT_EQ(KeyNameMapper::nameToScancode("B"), SC_B);
  EXPECT_EQ(KeyNameMapper::nameToScancode("C"), SC_C);
  EXPECT_EQ(KeyNameMapper::nameToScancode("D"), SC_D);
  EXPECT_EQ(KeyNameMapper::nameToScancode("E"), SC_E);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Q"), SC_Q);
  EXPECT_EQ(KeyNameMapper::nameToScancode("W"), SC_W);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Z"), SC_Z);
}

TEST(KeyNameMapperTest, ModifierKeys) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("LeftShift"), SC_LSHIFT);
  EXPECT_EQ(KeyNameMapper::nameToScancode("RightShift"), SC_RSHIFT);
  EXPECT_EQ(KeyNameMapper::nameToScancode("LeftCtrl"), SC_LCTRL);
  EXPECT_EQ(KeyNameMapper::nameToScancode("RightCtrl"), SC_RCTRL);
  EXPECT_EQ(KeyNameMapper::nameToScancode("LeftAlt"), SC_LALT);
  EXPECT_EQ(KeyNameMapper::nameToScancode("RightAlt"), SC_RALT);
}

TEST(KeyNameMapperTest, SpecialKeys) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("Space"), SC_SPACE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Enter"), SC_ENTER);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Backspace"), SC_BACKSPACE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Tab"), SC_TAB);
  EXPECT_EQ(KeyNameMapper::nameToScancode("CapsLock"), SC_CAPSLOCK);
}

TEST(KeyNameMapperTest, ArrowKeys) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("Up"), SC_UP);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Down"), SC_DOWN);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Left"), SC_LEFT);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Right"), SC_RIGHT);
}

TEST(KeyNameMapperTest, InvalidNames) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("InvalidKey"), std::nullopt);
  EXPECT_EQ(KeyNameMapper::nameToScancode(""), std::nullopt);
  EXPECT_EQ(KeyNameMapper::nameToScancode("a"), std::nullopt); // lowercase
  EXPECT_EQ(KeyNameMapper::nameToScancode("ESCAPE"),
            std::nullopt); // wrong case
}

TEST(KeyNameMapperTest, AliasNames) {
  // Grave aliases
  EXPECT_EQ(KeyNameMapper::nameToScancode("Grave"), SC_GRAVE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Backtick"), SC_GRAVE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Tilde"), SC_GRAVE);
}

// ===== Scancode to Name Tests =====

TEST(KeyNameMapperTest, ScancodeToNameFunctionKeys) {
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_ESCAPE), "Escape");
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_F1), "F1");
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_F12), "F12");
}

TEST(KeyNameMapperTest, ScancodeToNameLetters) {
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_A), "A");
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_Z), "Z");
}

TEST(KeyNameMapperTest, ScancodeToNameModifiers) {
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_LSHIFT), "LeftShift");
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_LCTRL), "LeftCtrl");
  EXPECT_EQ(KeyNameMapper::scancodeToName(SC_LALT), "LeftAlt");
}

TEST(KeyNameMapperTest, ScancodeToNameInvalid) {
  EXPECT_EQ(KeyNameMapper::scancodeToName(0xFFFF), std::nullopt);
  EXPECT_EQ(KeyNameMapper::scancodeToName(0x9999), std::nullopt);
}

// ===== Round Trip Tests =====

TEST(KeyNameMapperTest, RoundTripNameToScancodeToName) {
  const char *testKeys[] = {"A", "Escape", "LeftShift", "Space", "Enter"};

  for (const char *keyName : testKeys) {
    auto scancode = KeyNameMapper::nameToScancode(keyName);
    ASSERT_TRUE(scancode.has_value()) << "Failed to convert name: " << keyName;

    auto name = KeyNameMapper::scancodeToName(*scancode);
    ASSERT_TRUE(name.has_value()) << "Failed to convert scancode back to name";
    EXPECT_EQ(*name, std::string(keyName));
  }
}

// ===== Coverage Tests =====

TEST(KeyNameMapperTest, CommonlyUsedKeys) {
  // Ensure commonly used keys are mappable
  const char *commonKeys[] = {"A",         "B",         "C",        "D",
                              "E",         "Space",     "Enter",    "Escape",
                              "Tab",       "LeftShift", "LeftCtrl", "LeftAlt",
                              "Backspace", "CapsLock"};

  for (const char *key : commonKeys) {
    auto result = KeyNameMapper::nameToScancode(key);
    EXPECT_TRUE(result.has_value()) << "Key not found: " << key;
  }
}

TEST(KeyNameMapperTest, ExtendedKeysIfSupported) {
  // Test if extended keys (E0-prefixed) are supported
  // This depends on the KeyNameMapper implementation
  auto home = KeyNameMapper::nameToScancode("Home");
  auto end = KeyNameMapper::nameToScancode("End");
  auto pgup = KeyNameMapper::nameToScancode("PageUp");
  auto pgdn = KeyNameMapper::nameToScancode("PageDown");

  // These may or may not be implemented - test gracefully
  if (home.has_value()) {
    EXPECT_NE(*home, 0);
  }
  if (end.has_value()) {
    EXPECT_NE(*end, 0);
  }
  if (pgup.has_value()) {
    EXPECT_NE(*pgup, 0);
  }
  if (pgdn.has_value()) {
    EXPECT_NE(*pgdn, 0);
  }
}
