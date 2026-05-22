#include "hardware/Scancodes.h"
#include "processors/ComboAdvanced.h"
#include "processors/ModifierTracker.h"
#include "processors/Rewire.h"
#include "pipeline/Context.h"
#include <gtest/gtest.h>

using namespace keyflow;

/**
 * @brief Test for shiftMappings behavior: inject Shift without requiring it
 *
 * shiftMappings allow Layer+Key → Shift+Output
 * Example: RALT+J → + (outputs Shift+Equals)
 * The user doesn't hold Shift, but the system injects it
 */
TEST(BugFixTest, ShiftMappingsRequireShiftModifier) {
    ComboAdvanced combo;

    // Simulate config: RALT+J → Shift+Equals (+)
    // User presses RALT+J (no Shift), system outputs + (with injected Shift)
    combo.addComboWithShift(static_cast<uint32_t>(ModifierBit::RightAlt), SC_J, SC_EQUALS, true);

    // Test 1: RALT+J (without holding Shift) should output + with Shift injection
    {
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = static_cast<uint32_t>(ModifierBit::RightAlt); // Only RALT

        combo.process(ctx);

        EXPECT_EQ(ctx.action, Action::Replace);
        EXPECT_EQ(ctx.outputScancode, SC_EQUALS);
        EXPECT_TRUE(ctx.injectShift); // Should inject Shift for + symbol
    }

    // Test 2: LALT+J (different trigger) should not match
    {
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = static_cast<uint32_t>(ModifierBit::LeftAlt); // LALT instead
        ctx.action = Action::Forward;

        combo.process(ctx);

        // Should NOT match - wrong trigger
        EXPECT_EQ(ctx.action, Action::Forward);
        EXPECT_EQ(ctx.outputScancode, SC_J);
    }

    // Test 3: RALT+Shift+J should also work (Shift doesn't block it)
    {
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = static_cast<uint32_t>(ModifierBit::RightAlt) |
                        static_cast<uint32_t>(ModifierBit::LeftShift);

        combo.process(ctx);

        EXPECT_EQ(ctx.action, Action::Replace);
        EXPECT_EQ(ctx.outputScancode, SC_EQUALS);
        EXPECT_TRUE(ctx.injectShift); // Still injects Shift
    }
}

/**
 * @brief Test for Bug Fix #2: noModCombos should not interfere with remappings
 *
 * Before fix: Backslash → Equals remapping would trigger Equals noModCombo, outputting +
 * After fix: Backslash → Equals should output = correctly
 */
TEST(BugFixTest, NoModCombosMatchPhysicalKeys) {
    Rewire rewire;
    ComboAdvanced combo;

    // Simulate config:
    // 1. Remapping: Backslash → Equals
    // 2. noModCombo: Equals → Shift+Equals (+)
    rewire.setMapping(SC_BACKSLASH, SC_EQUALS);
    combo.addNoModCombo(SC_EQUALS, SC_EQUALS, true); // Physical Equals key → +

    // Test 1: Press physical Backslash key
    Context ctx;
    ctx.scancode = SC_BACKSLASH;
    ctx.outputScancode = SC_BACKSLASH;
    ctx.isDown = true;
    ctx.modifiers = 0;
    ctx.action = Action::Forward;

    // Step 1: Rewire processes it
    rewire.process(ctx);
    EXPECT_EQ(ctx.outputScancode, SC_EQUALS); // Remapped to Equals
    EXPECT_EQ(ctx.action, Action::Replace);

    // Step 2: ComboAdvanced processes it
    combo.process(ctx);

    // BUG FIX: noModCombo should NOT match because it checks physical key (SC_BACKSLASH)
    // not remapped key (SC_EQUALS)
    EXPECT_EQ(ctx.outputScancode, SC_EQUALS); // Should stay as Equals (=)
    EXPECT_FALSE(ctx.injectShift);            // Should NOT inject Shift

    // Test 2: Press physical Equals key (should trigger noModCombo)
    Context ctx2;
    ctx2.scancode = SC_EQUALS;
    ctx2.outputScancode = SC_EQUALS;
    ctx2.isDown = true;
    ctx2.modifiers = 0;
    ctx2.action = Action::Forward;

    combo.process(ctx2);

    // Physical Equals key should trigger noModCombo and output +
    EXPECT_EQ(ctx2.action, Action::Replace);
    EXPECT_EQ(ctx2.outputScancode, SC_EQUALS);
    EXPECT_TRUE(ctx2.injectShift); // Should inject Shift for + symbol
}

/**
 * @brief Integration test: Verify complete pipeline with both bug fixes
 */
TEST(BugFixTest, CompleteIntegrationTest) {
    Rewire rewire;
    ModifierTracker modTracker;
    ComboAdvanced combo;

    // Setup from config.json:
    // 1. Remapping: Backslash → Equals, J → K
    rewire.setMapping(SC_BACKSLASH, SC_EQUALS);
    rewire.setMapping(SC_J, SC_K);

    // 2. Layer: LALT+J → Backslash
    combo.addCombo("LALT", SC_J, SC_BACKSLASH);

    // 3. Layer shiftMapping: RALT+J → Shift+Equals (+)
    // (User doesn't hold Shift, system injects it)
    combo.addComboWithShift("RALT", SC_J, SC_EQUALS);

    // 4. noModCombo: Physical Equals → Shift+Equals (+)
    combo.addNoModCombo(SC_EQUALS, SC_EQUALS, true);

    // Test 1: Press Backslash → should output Equals scancode (=)
    {
        Context ctx;
        ctx.scancode = SC_BACKSLASH;
        ctx.outputScancode = SC_BACKSLASH;
        ctx.isDown = true;
        ctx.modifiers = 0;
        ctx.action = Action::Forward;

        rewire.process(ctx);
        modTracker.process(ctx);
        combo.process(ctx);

        EXPECT_EQ(ctx.outputScancode, SC_EQUALS); // Should be = scancode
        EXPECT_FALSE(ctx.injectShift);            // Should NOT be + symbol
    }

    // Test 2: Press J → should be remapped to K
    {
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = 0;
        ctx.action = Action::Forward;

        rewire.process(ctx);
        modTracker.process(ctx);
        combo.process(ctx);

        EXPECT_EQ(ctx.outputScancode, SC_K); // Remapped to K
    }

    // Test 3: Simulate LALT down, then press J → should output Backslash
    {
        // First, LALT down
        Context altCtx;
        altCtx.scancode = SC_LALT;
        altCtx.outputScancode = SC_LALT;
        altCtx.isDown = true;
        altCtx.modifiers = 0;
        modTracker.process(altCtx);

        // Now press J with LALT held
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = 0;
        ctx.action = Action::Forward;

        rewire.process(ctx);             // J → K
        modTracker.process(ctx);         // Sets modifiers to LALT
        combo.process(ctx);              // LALT+J (physical) → Backslash

        EXPECT_EQ(ctx.outputScancode, SC_BACKSLASH); // Should output Backslash
        EXPECT_FALSE(ctx.injectShift);
    }

    // Test 4: Simulate RALT down, press J → should trigger shiftMapping and output +
    {
        // Reset modifier tracker
        ModifierTracker modTracker2;

        // RALT down
        Context raltCtx;
        raltCtx.scancode = SC_RALT;
        raltCtx.outputScancode = SC_RALT;
        raltCtx.isDown = true;
        raltCtx.modifiers = 0;
        modTracker2.process(raltCtx);

        // Press J with RALT (no Shift needed)
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = 0;
        ctx.action = Action::Forward;

        rewire.process(ctx);       // J → K
        modTracker2.process(ctx);  // Sets modifiers to RALT
        combo.process(ctx);        // RALT+J → Shift+Equals (+)

        EXPECT_EQ(ctx.outputScancode, SC_EQUALS);
        EXPECT_TRUE(ctx.injectShift); // Should output + symbol
    }

    // Test 5: Verify RALT+Shift+J also works (extra Shift doesn't interfere)
    {
        // Reset modifier tracker
        ModifierTracker modTracker3;

        // RALT down
        Context raltCtx;
        raltCtx.scancode = SC_RALT;
        raltCtx.outputScancode = SC_RALT;
        raltCtx.isDown = true;
        raltCtx.modifiers = 0;
        modTracker3.process(raltCtx);

        // LeftShift down
        Context shiftCtx;
        shiftCtx.scancode = SC_LSHIFT;
        shiftCtx.outputScancode = SC_LSHIFT;
        shiftCtx.isDown = true;
        shiftCtx.modifiers = 0;
        modTracker3.process(shiftCtx);

        // Press J with RALT+Shift (still works)
        Context ctx;
        ctx.scancode = SC_J;
        ctx.outputScancode = SC_J;
        ctx.isDown = true;
        ctx.modifiers = 0;
        ctx.action = Action::Forward;

        rewire.process(ctx);       // J → K
        modTracker3.process(ctx);  // Sets modifiers to RALT+LeftShift
        combo.process(ctx);        // RALT+J matches (extra Shift doesn't block)

        EXPECT_EQ(ctx.outputScancode, SC_EQUALS);
        EXPECT_TRUE(ctx.injectShift); // Should output + symbol
    }
}
