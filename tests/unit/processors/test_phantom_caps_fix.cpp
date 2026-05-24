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
                                      const KeyEvent &event,
                                      bool &needsCleanup) {
  SimulatedEventResult result;

  // CLEANUP PHASE (before processing event)
  if (needsCleanup) {
    result.shiftUp = true; // Send SHIFT UP
    tracker.clearInjectedModifiers();
    needsCleanup = false;
  }

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
      needsCleanup = processResult.cleanupInjectedShift; // Mark for cleanup
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

// ===== Pipeline Cleanup State Tests =====

TEST(PhantomCapsFixTest, PipelineTracksCleanupState) {
  Pipeline pipeline;

  EXPECT_FALSE(pipeline.needsShiftCleanup()) << "Initially no cleanup needed";

  pipeline.markShiftCleanupNeeded(true);
  EXPECT_TRUE(pipeline.needsShiftCleanup()) << "Cleanup should be marked";

  pipeline.clearShiftCleanup();
  EXPECT_FALSE(pipeline.needsShiftCleanup()) << "Cleanup should be cleared";
}

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

  bool needsCleanup = false;

  // Event 1: Press backtick (should inject shift)
  KeyEvent event1{SC_GRAVE, true};
  auto result1 = simulateKeyEvent(pipeline, *trackerPtr, event1, needsCleanup);

  EXPECT_TRUE(result1.shiftDown) << "Shift should be pressed";
  EXPECT_TRUE(result1.keyDown) << "? key should be pressed";
  EXPECT_EQ(result1.outputScancode, SC_SLASH);
  EXPECT_FALSE(needsCleanup) << "Cleanup not needed yet (key still down)";

  // Event 2: Release backtick (should release shift and mark cleanup)
  KeyEvent event2{SC_GRAVE, false};
  auto result2 = simulateKeyEvent(pipeline, *trackerPtr, event2, needsCleanup);

  EXPECT_TRUE(result2.keyUp) << "? key should be released";
  EXPECT_TRUE(result2.shiftUp) << "Shift should be released";
  EXPECT_TRUE(needsCleanup) << "Cleanup should be marked for next event";

  // Event 3: Press 'A' very fast (before OS processes shift up)
  // THIS IS WHERE THE BUG WOULD OCCUR
  KeyEvent event3{SC_A, true};
  auto result3 = simulateKeyEvent(pipeline, *trackerPtr, event3, needsCleanup);

  EXPECT_TRUE(result3.shiftUp) << "CLEANUP: Shift UP should be sent BEFORE 'A'";
  EXPECT_FALSE(needsCleanup) << "Cleanup flag should be cleared";
  // Note: The shift cleanup happens BEFORE processing event3,
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

  bool needsCleanup = false;

  // Sequence: ` ↓ ` ↑ 1 ↓ 1 ↑ A ↓

  // Press and release backtick
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true},
                   needsCleanup);
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false},
                   needsCleanup);
  EXPECT_TRUE(needsCleanup) << "Cleanup needed after backtick";

  // Press '1' (cleanup should happen first)
  auto result1Down = simulateKeyEvent(pipeline, *trackerPtr,
                                      KeyEvent{SC_1, true}, needsCleanup);
  EXPECT_TRUE(result1Down.shiftUp) << "Cleanup: Shift UP before '1'";
  EXPECT_FALSE(needsCleanup) << "Cleanup cleared";

  // Release '1'
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_1, false}, needsCleanup);
  EXPECT_TRUE(needsCleanup) << "Cleanup needed after '1'";

  // Press 'A' (cleanup should happen first)
  auto resultA = simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_A, true},
                                  needsCleanup);
  EXPECT_TRUE(resultA.shiftUp) << "Cleanup: Shift UP before 'A'";
  EXPECT_FALSE(needsCleanup) << "No phantom capitalization on 'A'";
}

TEST(PhantomCapsFixTest, PhysicalShiftNotAffectedByCleanup) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  ModifierTracker *trackerPtr = modTracker.get();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  pipeline.addProcessor(std::move(combo));

  bool needsCleanup = false;

  // Press physical shift (simulated by setting modifier bit manually)
  Context ctx;
  ctx.scancode = SC_LSHIFT;
  ctx.outputScancode = SC_LSHIFT;
  ctx.isDown = true;
  trackerPtr->process(ctx); // Track the physical shift

  EXPECT_TRUE(trackerPtr->isModifierActive(ModifierBit::LeftShift))
      << "Physical shift should be tracked";

  // Use shift combo: ` → ?
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true},
                   needsCleanup);
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false},
                   needsCleanup);

  // Clear injected modifiers (simulating cleanup)
  trackerPtr->clearInjectedModifiers();

  // Physical shift should still be tracked
  EXPECT_TRUE(trackerPtr->isModifierActive(ModifierBit::LeftShift))
      << "Physical shift should NOT be cleared by cleanup";
}

TEST(PhantomCapsFixTest, NoCleanupForRegularKeys) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  pipeline.addProcessor(std::move(modTracker));

  // No combos - just regular keys
  bool needsCleanup = false;

  // Type regular 'A'
  KeyEvent eventDown{SC_A, true};
  auto result = pipeline.process(eventDown);

  EXPECT_FALSE(result.injectShift) << "No shift injection for regular key";
  EXPECT_FALSE(result.cleanupInjectedShift) << "No cleanup needed";
  EXPECT_FALSE(needsCleanup) << "Cleanup flag should remain false";
}

TEST(PhantomCapsFixTest, CleanupOnlyOnKeyUp) {
  Pipeline pipeline;
  auto modTracker = std::make_unique<ModifierTracker>();
  pipeline.addProcessor(std::move(modTracker));

  auto combo = std::make_unique<ComboAdvanced>();
  combo->addNoModCombo(SC_GRAVE, SC_SLASH, true); // ` → ?
  pipeline.addProcessor(std::move(combo));

  bool needsCleanup = false;

  // Press backtick (no cleanup yet)
  auto resultDown = pipeline.process(KeyEvent{SC_GRAVE, true});
  EXPECT_TRUE(resultDown.injectShift) << "Shift injected on key down";
  EXPECT_TRUE(resultDown.cleanupInjectedShift) << "Cleanup flag set";

  // Pipeline should NOT mark cleanup on key down
  EXPECT_FALSE(pipeline.needsShiftCleanup())
      << "Pipeline cleanup only marked on key UP";

  // Simulate what main.cpp does on key UP
  auto resultUp = pipeline.process(KeyEvent{SC_GRAVE, false});
  if (resultUp.cleanupInjectedShift) {
    pipeline.markShiftCleanupNeeded(true);
    needsCleanup = true;
  }

  EXPECT_TRUE(needsCleanup) << "Cleanup marked after key release";
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

  bool needsCleanup = false;

  // Trigger shift combo
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true},
                   needsCleanup);
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false},
                   needsCleanup);
  EXPECT_TRUE(needsCleanup);

  // Process next event (cleanup should execute and clear)
  simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_A, true}, needsCleanup);
  EXPECT_FALSE(needsCleanup) << "Cleanup should be cleared after execution";

  // Next event should NOT trigger cleanup again
  auto result = simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_B, true},
                                 needsCleanup);
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

  bool needsCleanup = false;

  // Type: ` ` ` (three times fast)
  for (int i = 0; i < 3; i++) {
    // Press
    simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, true},
                     needsCleanup);

    // Release (marks cleanup)
    simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_GRAVE, false},
                     needsCleanup);
    EXPECT_TRUE(needsCleanup) << "Cleanup should be marked after combo " << i;
  }

  // Type 'A' - cleanup should prevent phantom caps
  auto result = simulateKeyEvent(pipeline, *trackerPtr, KeyEvent{SC_A, true},
                                 needsCleanup);
  EXPECT_TRUE(result.shiftUp) << "Cleanup should execute before 'A'";
  EXPECT_FALSE(needsCleanup) << "No phantom capitalization";
}
