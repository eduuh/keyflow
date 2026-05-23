#include "config/KeyNameMapper.h"
#include "hardware/Scancodes.h"
#include <gtest/gtest.h>

using namespace keyflow;

// Boundary: invalid/empty/wrong-case names return nullopt. ConfigLoader's
// validation depends on this — if a typo'd key name silently mapped to
// something, the validation would pass and the user would see surprising
// behavior at runtime.
TEST(KeyNameMapperTest, InvalidNames) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("InvalidKey"), std::nullopt);
  EXPECT_EQ(KeyNameMapper::nameToScancode(""), std::nullopt);
  EXPECT_EQ(KeyNameMapper::nameToScancode("a"), std::nullopt);
  EXPECT_EQ(KeyNameMapper::nameToScancode("ESCAPE"), std::nullopt);
}

// Non-trivial behavior: the same scancode has multiple accepted names.
// Grave/Backtick/Tilde all resolve to SC_GRAVE. If aliasing regresses,
// configs using the less-common spelling silently break.
TEST(KeyNameMapperTest, AliasNames) {
  EXPECT_EQ(KeyNameMapper::nameToScancode("Grave"), SC_GRAVE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Backtick"), SC_GRAVE);
  EXPECT_EQ(KeyNameMapper::nameToScancode("Tilde"), SC_GRAVE);
}
