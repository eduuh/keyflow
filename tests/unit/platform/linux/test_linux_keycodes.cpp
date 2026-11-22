#ifdef __linux__

#    include "platform/linux/LinuxKeycodes.h"

#    include <gtest/gtest.h>

namespace keyflow::linux_backend {
namespace {

TEST(LinuxKeycodes, KeypadAndNavigationRemainDistinctInBothDirections) {
    EXPECT_EQ(toEvdev(SC_KP_7), KEY_KP7);
    EXPECT_EQ(toEvdev(SC_HOME), KEY_HOME);
    EXPECT_EQ(toCanonical(KEY_KP7), 0x0047);
    EXPECT_EQ(toCanonical(KEY_HOME), 0xe047);
    EXPECT_EQ(toEvdev(SC_KP_ENTER), KEY_KPENTER);
    EXPECT_EQ(toEvdev(SC_ENTER), KEY_ENTER);
    EXPECT_EQ(toCanonical(KEY_KPENTER), 0xe01c);
    EXPECT_EQ(toCanonical(KEY_ENTER), 0x001c);
}

TEST(LinuxKeycodes, SidedControlPauseAndPrintScreenKeepTheirFullIdentity) {
    EXPECT_EQ(toCanonical(KEY_LEFTCTRL), 0x001d);
    EXPECT_EQ(toCanonical(KEY_RIGHTCTRL), 0xe01d);
    EXPECT_EQ(toEvdev(SC_LCTRL), KEY_LEFTCTRL);
    EXPECT_EQ(toEvdev(SC_RCTRL), KEY_RIGHTCTRL);
    EXPECT_EQ(toCanonical(KEY_PAUSE), 0xe11d);
    EXPECT_EQ(toEvdev(SC_PAUSE), KEY_PAUSE);
    EXPECT_EQ(toCanonical(KEY_SYSRQ), 0xe037);
    EXPECT_EQ(toEvdev(SC_PRINTSCREEN), KEY_SYSRQ);
}

TEST(LinuxKeycodes, MediaAndBrowserAreCanonicalNotNative) {
    EXPECT_EQ(toCanonical(KEY_PLAYPAUSE), 0xe022);
    EXPECT_EQ(toEvdev(SC_MEDIA_PLAY), KEY_PLAYPAUSE);
    EXPECT_EQ(toCanonical(KEY_BOOKMARKS), 0xe066);
    EXPECT_EQ(toEvdev(SC_BROWSER_FAVORITES), KEY_BOOKMARKS);
    EXPECT_EQ(toCanonical(KEY_BACK), 0xe06a);
    EXPECT_EQ(toEvdev(SC_BROWSER_BACK), KEY_BACK);
}

TEST(LinuxKeycodes, EveryCanonicalMappingIsUniqueAndRoundTripsWithoutMasking) {
    for (const auto& mapping : keycodeMappings) {
        EXPECT_EQ(toEvdev(mapping.canonical), mapping.evdev);
        EXPECT_EQ(toCanonical(mapping.evdev), mapping.canonical);
        EXPECT_TRUE(isKeyboardCode(mapping.evdev));
    }
}

TEST(LinuxKeycodes, UnknownAndSentinelsNeverBecomeAnEmittableCanonicalKey) {
    EXPECT_FALSE(toEvdev(SC_NOP));
    EXPECT_FALSE(toEvdev(SC_UNMAPPED));
    EXPECT_FALSE(toEvdev(0xe147));
    EXPECT_FALSE(toCanonical(KEY_F13));
    EXPECT_FALSE(toCanonical(KEY_RESERVED));
    EXPECT_FALSE(toCanonical(BTN_LEFT));
}

TEST(LinuxKeycodes, NativePolicyAcceptsKeyboardExtensionsButNotAnyButtonRange) {
    EXPECT_TRUE(isKeyboardCode(KEY_F13));
    EXPECT_TRUE(isKeyboardCode(KEY_102ND));
    EXPECT_TRUE(isKeyboardCode(KEY_BRIGHTNESSUP));
    EXPECT_FALSE(isKeyboardCode(KEY_RESERVED));
    EXPECT_FALSE(isKeyboardCode(BTN_LEFT));
    EXPECT_FALSE(isKeyboardCode(BTN_GAMEPAD));
    EXPECT_FALSE(isKeyboardCode(BTN_DPAD_UP));
    EXPECT_FALSE(isKeyboardCode(BTN_TRIGGER_HAPPY1));
    EXPECT_FALSE(isKeyboardCode(0xffff));
}

} // namespace
} // namespace keyflow::linux_backend

#endif
