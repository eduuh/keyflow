#include "hardware/Scancodes.h"
#include "processors/CapsLockBlocker.h"
#include <gtest/gtest.h>

using namespace keyflow;

// CapsLock should be consumed when it reaches this processor unremapped
// (config didn't map it, or no Rewire ran). Prevents the OS from toggling
// caps lock state.
TEST(CapsLockBlockerTest, BlocksUnremappedCapsLock) {
  CapsLockBlocker blocker;
  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_CAPSLOCK;
  ctx.isDown = true;
  ctx.action = Action::Forward;

  bool continues = blocker.process(ctx);

  EXPECT_FALSE(continues);
  EXPECT_EQ(ctx.action, Action::Consume);
}

// When config remaps CapsLock to something else (e.g. LeftShift), the
// blocker must let the remapped key through. The discriminator is
// outputScancode, not the physical scancode — this is the subtle part the
// integration test doesn't directly assert.
TEST(CapsLockBlockerTest, AllowsRemappedCapsLock) {
  CapsLockBlocker blocker;
  Context ctx;
  ctx.scancode = SC_CAPSLOCK;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  ctx.action = Action::Replace;

  bool continues = blocker.process(ctx);

  EXPECT_TRUE(continues);
  EXPECT_EQ(ctx.action, Action::Replace);
}
