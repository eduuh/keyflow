#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"
#include "processors/ComboAdvanced.h"
#include "processors/ModifierTracker.h"
#include "processors/Rewire.h"
#include <gtest/gtest.h>

using namespace keyflow;

/**
 * @brief Tests for phantom capitalization bug fix
 *
 * These tests verify that the shift cleanup logic prevents phantom
 * capitalization during fast typing after noModCombos that inject Shift.
 *
 * The bug occurred because:
 * 1. NoModCombo injects: SHIFT DOWN → KEY DOWN → KEY UP → SHIFT UP
 * 2. Next character arrives before SHIFT UP is fully processed
 * 3. OS still sees Shift pressed → phantom capitalization
 *
 * The fix:
 * 1. Track injected shift separately from physical shift
 * 2. Mark cleanup needed after shift injection
 * 3. Explicitly cleanup before processing next event
 */

// ===== Helper Function to Simulate Event Loop =====

/**
 * @brief Simulates the main event loop shift injection logic
 *
 * This replicates what happens in main.cpp without needing hardware.
 * NOTE: With the synchronous cleanup fix, cleanup happens immediately on key
 * release.
 */
struct SimulatedEventResult {
  bool shiftDown = false;  // Shift key pressed?
  bool shiftUp = false;    // Shift key released?
  bool keyDown = false;    // Output key pressed?
  bool keyUp = false;      // Output key released?
  uint16_t outputScancode; // What key was sent
};

SimulatedEventResult simulateKeyEvent(Pipeline &pipeline,
                                      ModifierTracker &tracker,
                                      const KeyEvent &event) {
  SimulatedEventResult result;

  // PROCESS PHASE
  auto processResult = pipeline.process(event);

  // OUTPUT PHASE
  if (processResult.action == Action::Replace && processResult.injectShift) {
    if (event.isDown) {
      result.shiftDown = true;
      result.keyDown = true;
      result.outputScancode = processResult.outputScancode;
      tracker.setInjectedShift(true);
    } else {
      result.keyUp = true;
      result.shiftUp = true;
      result.outputScancode = processResult.outputScancode;
      // Clear injected modifiers IMMEDIATELY (synchronous cleanup)
      if (processResult.cleanupInjectedShift) {
        tracker.clearInjectedModifiers();
      }
    }
  }

  return result;
}

// ===== Basic Cleanup Flag Tests =====

TEST(PhantomCapsFixTest, CleanupFlagSetForShiftInjection) {
  ComboAdvanced combo;
  combo.addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ? (with shift)

  Context ctx;
  ctx.scancode = SC_GRAVE;
  ctx.outputScancode = SC_GRAVE;
  ctx.isDown = true;
  ctx.modifiers = 0;

  combo.process(ctx);

  EXPECT_TRUE(ctx.injectShift) << "Shift should be injected";
  EXPECT_TRUE(ctx.cleanupInjectedShift) << "Cleanup flag should be set";
}

TEST(PhantomCapsFixTest, CleanupFlagNotSetForNoShift) {
  ComboAdvanced combo;
  combo.addNoModCombo(SC_2, SC_LBRACKET, false); // 2 → [ (no shift)

  Context ctx;
  ctx.scancode = SC_2;
  ctx.outputScancode = SC_2;
  ctx.isDown = true;
  ctx.modifiers = 0;

  combo.process(ctx);

  EXPECT_FALSE(ctx.injectShift) << "Shift should NOT be injected";
  EXPECT_FALSE(ctx.cleanupInjectedShift) << "Cleanup flag should NOT be set";
}

// ===== Processing Result Tests =====

TEST(PhantomCapsFixTest, ProcessingResultIncludesCleanupFlag) {
  Pipeline pipeline;
  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_1, SC_1, true); // 1 → ! (with shift)
  pipeline.addProcessor(std::move(combo));

  KeyEvent event{SC_1, true};
  auto result = pipeline.process(event);

  EXPECT_TRUE(result.injectShift) << "Shift injection requested";
  EXPECT_TRUE(result.cleanupInjectedShift) << "Cleanup flag should be set";
}

// ===== ModifierTracker Tests =====

TEST(PhantomCapsFixTest, ModifierTrackerTracksInjectedShift) {
  ModifierTracker tracker;

  EXPECT_FALSE(tracker.hasInjectedShift()) << "Initially no injected shift";

  tracker.setInjectedShift(true);
  EXPECT_TRUE(tracker.hasInjectedShift()) << "Injected shift should be tracked";

  tracker.clearInjectedModifiers();
  EXPECT_FALSE(tracker.hasInjectedShift()) << "Should be cleared";
}

// ===== Simulated Event Loop Tests =====

TEST(PhantomCapsFixTest, FastTypingAfterShiftCombo) {
  // Setup pipeline: Rewire + ModifierTracker + ComboAdvanced
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  pipeline.addProcessor(std::move(combo));

  // Event 1: Press backtick (should inject shift)
  KeyEvent event1{SC_GRAVE, true};
  auto result1 = simulateKeyEvent(pipeline, *trackerPtr, event1);

  EXPECT_TRUE(result1.shiftDown) << "Shift should be pressed";
  EXPECT_TRUE(result1.keyDown) << "? key should be pressed";
  EXPECT_EQ(result1.outputScancode, SC_SLASH);
  EXPECT_TRUE(trackerPtr->hasInjectedShift()) << "Injected shift tracked";

  // Event 2: Release backtick (should release shift and cleanup immediately)
  KeyEvent event2{SC_GRAVE, false};
  auto result2 = simulateKeyEvent(pipeline, *trackerPtr, event2);

  EXPECT_TRUE(result2.keyUp) << "? key should be released";
  EXPECT_TRUE(result2.shiftUp) << "Shift should be released";
  EXPECT_FALSE(trackerPtr->hasInjectedShift())
      << "Injected shift cleared immediately";

  // Event 3: Press 'A' very fast (injected shift already cleared)
  // With synchronous cleanup, no phantom capitalization occurs
  KeyEvent event3{SC_A, true};
  (void)simulateKeyEvent(pipeline, *trackerPtr, event3);

  EXPECT_FALSE(trackerPtr->hasInjectedShift()) << "No injected shift for 'A'";
  // Note: The shift cleanup happened synchronously in Event 2,
  // preventing phantom capitalization
}

TEST(PhantomCapsFixTest, MultipleShiftCombosInSequence) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  combo->addNoModCombo(SC_1, SC_1, true);         // 1 → !
  pipeline.addProcessor(std::move(combo));

  // Sequence: ` ↓ ` ↑ 1 ↓ 1 ↑ A ↓

  // Press and release backtick
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true});
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false});
  EXPECT_FALSE(trackerPtr->hasInjectedShift())
      << "Cleanup done immediately after backtick";

  // Press '1' (no cleanup needed, already done)
  auto result1Down =
      simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_1, true});
  EXPECT_TRUE(result1Down.shiftDown) << "Shift injected for '1'";
  EXPECT_TRUE(trackerPtr->hasInjectedShift()) << "Injected shift tracked";

  // Release '1'
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_1, false});
  EXPECT_FALSE(trackerPtr->hasInjectedShift())
      << "Cleanup done immediately after '1'";

  // Press 'A' (no injected shift to worry about)
  (void)simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_A, true});
  EXPECT_FALSE(trackerPtr->hasInjectedShift())
      << "No phantom capitalization on 'A'";
}

TEST(PhantomCapsFixTest, PhysicalShiftNotAffectedByCleanup) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  pipeline.addProcessor(std::move(combo));

  // Press physical shift (simulated by setting modifier bit manually)
  Context ctx;
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  trackerPtr->process(ctx); // Track the physical shift

  EXPECT_TRUE(trackerPtr->isModifierActive(ModifierBit::LeftShift))
      << "Physical shift should be tracked";

  // Use shift combo: ` → ?
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true});
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false});

  // Physical shift should still be tracked after cleanup
  EXPECT_TRUE(trackerPtr->isModifierActive(ModifierBit::LeftShift))
      << "Physical shift should NOT be cleared by cleanup";
}

TEST(PhantomCapsFixTest, NoCleanupForRegularKeys) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  pipeline.addProcessor(std::move(modTracker));

  // No combos - just regular keys

  // Type regular 'A'
  KeyEvent eventDown{SC_A, true};
  auto result = pipeline.process(eventDown);

  EXPECT_FALSE(result.injectShift) << "No shift injection for regular key";
  EXPECT_FALSE(result.cleanupInjectedShift) << "No cleanup needed";
}

TEST(PhantomCapsFixTest, CleanupHappensOnKeyUp) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  pipeline.addProcessor(std::move(combo));

  // Press backtick (shift tracked but not cleaned up yet)
  auto resultDown = pipeline.process(KeyEvent{SC_GRAVE, true});
  EXPECT_TRUE(resultDown.injectShift) << "Shift injected on key down";
  EXPECT_TRUE(resultDown.cleanupInjectedShift) << "Cleanup flag set";

  // Simulate shift tracking
  trackerPtr->setInjectedShift(true);
  EXPECT_TRUE(trackerPtr->hasInjectedShift()) << "Shift tracked after key down";

  // Release backtick (cleanup happens immediately)
  auto resultUp = pipeline.process(KeyEvent{SC_GRAVE, false});
  EXPECT_TRUE(resultUp.cleanupInjectedShift) << "Cleanup flag set on key up";

  // Simulate synchronous cleanup
  if (resultUp.cleanupInjectedShift) {
    trackerPtr->clearInjectedModifiers();
  }

  EXPECT_FALSE(trackerPtr->hasInjectedShift())
      << "Cleanup done immediately on key release";
}

// ===== Edge Case Tests =====

TEST(PhantomCapsFixTest, CleanupClearedAfterExecution) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true);
  pipeline.addProcessor(std::move(combo));

  // Trigger shift combo
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true});
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false});
  EXPECT_FALSE(trackerPtr->hasInjectedShift()) << "Cleanup done synchronously";

  // Process next event (no cleanup needed)
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_A, true});
  EXPECT_FALSE(trackerPtr->hasInjectedShift()) << "No injected shift for 'A'";

  // Next event should also have no cleanup
  auto result = simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_B, true});
  EXPECT_FALSE(result.shiftUp) << "No shift cleanup on unrelated key";
}

TEST(PhantomCapsFixTest, RapidRepeatedCombo) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  pipeline.addProcessor(std::move(combo));

  // Type: ` ` ` (three times fast)
  for (int i = 0; i < 3; i++) {
    // Press
    simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true});
    EXPECT_TRUE(trackerPtr->hasInjectedShift())
        << "Shift injected on press " << i;

    // Release (cleanup happens immediately)
    simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false});
    EXPECT_FALSE(trackerPtr->hasInjectedShift())
        << "Cleanup done immediately after combo " << i;
  }

  // Type 'A' - no phantom caps since cleanup already done
  (void)simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_A, true});
  EXPECT_FALSE(trackerPtr->hasInjectedShift()) << "No phantom capitalization";
}
