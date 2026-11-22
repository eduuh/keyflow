#include "app/OutputDispatcher.h"
#include "fixtures/PipelineHarness.h"

#include <gtest/gtest.h>

#include <vector>

using namespace keyflow;

namespace {

class RecordingHardware : public IHardwareIO {
public:
  bool initialize() noexcept override { return true; }
  void shutdown() noexcept override {}
  std::optional<KeyEvent> waitForKey(int) noexcept override {
    return std::nullopt;
  }
  void sendKey(uint16_t key, bool down) noexcept override {
    events.emplace_back(key, down);
  }
  bool sendEvent(const KeyEvent &event) noexcept override {
    events.push_back(event);
    if (failNext) {
      failNext = false;
      return false;
    }
    return true;
  }
  bool isInitialized() const noexcept override { return true; }
  std::vector<KeyEvent> events;
  bool failNext = false;
};

void expectEvent(const KeyEvent &event, uint16_t key, KeyAction action) {
  EXPECT_EQ(event.scancode, key);
  EXPECT_EQ(event.action, action);
}

} // namespace

TEST(OutputDispatcherTest, ReleaseKeepsLayerOutputAfterTriggerRelease) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);

  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_RALT, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_K, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_RALT, false)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_K, KeyAction::Repeat)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_K, false)));

  ASSERT_EQ(hardware.events.size(), 3u);
  expectEvent(hardware.events[0], SC_LEFT, KeyAction::Press);
  expectEvent(hardware.events[1], SC_LEFT, KeyAction::Repeat);
  expectEvent(hardware.events[2], SC_LEFT, KeyAction::Release);
  EXPECT_EQ(harness.currentModifiers(), 0u);
}

TEST(OutputDispatcherTest, InjectedShiftDoesNotReleaseHeldPhysicalShift) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);

  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_GRAVE, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_CAPSLOCK, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_GRAVE, false)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_CAPSLOCK, false)));

  ASSERT_EQ(hardware.events.size(), 4u);
  expectEvent(hardware.events[0], SC_LSHIFT, KeyAction::Press);
  expectEvent(hardware.events[1], SC_SLASH, KeyAction::Press);
  expectEvent(hardware.events[2], SC_SLASH, KeyAction::Release);
  expectEvent(hardware.events[3], SC_LSHIFT, KeyAction::Release);
}

TEST(OutputDispatcherTest, OverlappingSourcesDoNotReleaseOtherHolder) {
  Pipeline pipeline;
  RecordingHardware hardware;
  OutputDispatcher dispatcher(pipeline, hardware);

  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Press, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Press, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Release, 1)));
  EXPECT_EQ(hardware.events.size(), 1u);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Repeat, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Release, 2)));

  ASSERT_EQ(hardware.events.size(), 3u);
  expectEvent(hardware.events.back(), SC_A, KeyAction::Release);
}

TEST(OutputDispatcherTest, DistinctPhysicalKeysShareModifierOwnership) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);

  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LSHIFT, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LCTRL, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LSHIFT, false)));
  EXPECT_EQ(harness.currentModifiers(),
            static_cast<uint32_t>(ModifierBit::LeftCtrl));
  EXPECT_EQ(hardware.events.size(), 1u);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LCTRL, false)));

  EXPECT_EQ(harness.currentModifiers(), 0u);
  ASSERT_EQ(hardware.events.size(), 2u);
  expectEvent(hardware.events[1], SC_LCTRL, KeyAction::Release);
}

TEST(OutputDispatcherTest, ShutdownReleasesOrdinaryAndInjectedKeys) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_GRAVE, true)));
  dispatcher.shutdown();
  const auto count = hardware.events.size();
  dispatcher.shutdown();

  ASSERT_EQ(count, 4u);
  EXPECT_EQ(hardware.events.size(), count);
  EXPECT_EQ(hardware.events[2].action, KeyAction::Release);
  EXPECT_EQ(hardware.events[3].action, KeyAction::Release);
}

TEST(OutputDispatcherTest, NativeUnknownKeyBypassesCanonicalRemapping) {
  Pipeline pipeline;
  RecordingHardware hardware;
  OutputDispatcher dispatcher(pipeline, hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(183, KeyAction::Press, 1, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(183, KeyAction::Release, 1, true)));

  ASSERT_EQ(hardware.events.size(), 2u);
  EXPECT_TRUE(hardware.events[0].native);
  expectEvent(hardware.events[1], 183, KeyAction::Release);
}

TEST(OutputDispatcherTest, FailedReleaseIsRetriedDuringShutdown) {
  Pipeline pipeline;
  RecordingHardware hardware;
  OutputDispatcher dispatcher(pipeline, hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, true)));
  hardware.failNext = true;
  EXPECT_FALSE(dispatcher.process(KeyEvent(SC_A, false)));
  dispatcher.shutdown();

  ASSERT_EQ(hardware.events.size(), 3u);
  expectEvent(hardware.events.at(2), SC_A, KeyAction::Release);
}

TEST(OutputDispatcherTest, OnlyOnePhysicalSourceOwnsRepeatAtATime) {
  Pipeline pipeline;
  RecordingHardware hardware;
  OutputDispatcher dispatcher(pipeline, hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Press, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Press, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Repeat, 2)));
  EXPECT_EQ(hardware.events.size(), 1u);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Repeat, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Release, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Repeat, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Release, 2)));
  ASSERT_EQ(hardware.events.size(), 4u);
  expectEvent(hardware.events.at(2), SC_A, KeyAction::Repeat);
  expectEvent(hardware.events.at(3), SC_A, KeyAction::Release);
}

TEST(OutputDispatcherTest, EmergencyChordUsesPhysicalControlBeforeRemapping) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  EXPECT_FALSE(dispatcher.physicalCtrlHeld());
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_RCTRL, true)));
  EXPECT_TRUE(dispatcher.physicalCtrlHeld());
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_RCTRL, false)));
  EXPECT_FALSE(dispatcher.physicalCtrlHeld());
  ASSERT_EQ(hardware.events.size(), 2u);
  expectEvent(hardware.events.at(0), SC_PRINTSCREEN, KeyAction::Press);
}

TEST(OutputDispatcherTest,
     DesignerLayerKeepsNumberRowOutputForRepeatedWindowsDowns) {
  auto harness = keyflow::testing::PipelineHarness::fromConfigFile(
      KEYFLOW_DESIGNER_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LALT, KeyAction::Press, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_K, KeyAction::Press, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LALT, KeyAction::Release, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_K, KeyAction::Press, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_K, KeyAction::Release, 2)));
  ASSERT_EQ(hardware.events.size(), 3u);
  expectEvent(hardware.events.at(0), SC_1, KeyAction::Press);
  expectEvent(hardware.events.at(1), SC_1, KeyAction::Repeat);
  expectEvent(hardware.events.at(2), SC_1, KeyAction::Release);
  EXPECT_EQ(hardware.events.at(2).source, 2);
  EXPECT_EQ(harness.currentModifiers(), 0u);
}

TEST(OutputDispatcherTest,
     CollidingOrdinaryOutputsStayDownUntilTheirFinalOwnerReleases) {
  auto harness = keyflow::testing::PipelineHarness::fromConfigString(
      R"({"version":"1.0","remapping":{"A":"C","B":"C"}})");
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Press, 1)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_B, KeyAction::Press, 2)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_A, KeyAction::Release, 1)));
  ASSERT_EQ(hardware.events.size(), 1u);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_B, KeyAction::Release, 2)));
  ASSERT_EQ(hardware.events.size(), 2u);
  expectEvent(hardware.events.at(0), SC_C, KeyAction::Press);
  expectEvent(hardware.events.at(1), SC_C, KeyAction::Release);
  EXPECT_EQ(hardware.events.at(1).source, 1);
}

TEST(OutputDispatcherTest,
     RuntimeWinModifierUsesKeypadIdentityAndHonorsBlockOutput) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LWIN, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_J, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_LWIN, false)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_J, false)));

  bool blockOutput = true;
  for (const auto &customMod : harness.config().customModifiers) {
    if (customMod.modifierName == "WIN_MOD") {
      blockOutput = customMod.blockOutput;
      break;
    }
  }

  if (blockOutput) {
    ASSERT_EQ(hardware.events.size(), 2u);
    expectEvent(hardware.events.at(0), SC_KP_7, KeyAction::Press);
    expectEvent(hardware.events.at(1), SC_KP_7, KeyAction::Release);
  } else {
    ASSERT_EQ(hardware.events.size(), 4u);
    expectEvent(hardware.events.at(0), SC_LWIN, KeyAction::Press);
    expectEvent(hardware.events.at(1), SC_KP_7, KeyAction::Press);
    expectEvent(hardware.events.at(2), SC_LWIN, KeyAction::Release);
    expectEvent(hardware.events.at(3), SC_KP_7, KeyAction::Release);
  }
  EXPECT_EQ(harness.currentModifiers(), 0u);
}

TEST(OutputDispatcherTest, RuntimeArrowLayerOwnsItsShiftedEqualsChord) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_RALT, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_U, true)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_RALT, false)));
  ASSERT_TRUE(dispatcher.process(KeyEvent(SC_U, false)));
  ASSERT_EQ(hardware.events.size(), 4u);
  expectEvent(hardware.events.at(0), SC_LSHIFT, KeyAction::Press);
  expectEvent(hardware.events.at(1), SC_EQUALS, KeyAction::Press);
  expectEvent(hardware.events.at(2), SC_EQUALS, KeyAction::Release);
  expectEvent(hardware.events.at(3), SC_LSHIFT, KeyAction::Release);
  EXPECT_EQ(harness.currentModifiers(), 0u);
}

TEST(OutputDispatcherTest, RuntimeBaseAndNoModifierSymbolsHaveLiteralOutputs) {
  auto harness =
      keyflow::testing::PipelineHarness::fromConfigFile(KEYFLOW_CONFIG_PATH);
  RecordingHardware hardware;
  OutputDispatcher dispatcher(harness.pipeline(), hardware);
  for (auto input : {SC_BACKSLASH, SC_EQUALS, SC_B, SC_GRAVE, SC_J}) {
    ASSERT_TRUE(dispatcher.process(KeyEvent(input, true)));
    ASSERT_TRUE(dispatcher.process(KeyEvent(input, false)));
  }
  const std::vector<std::pair<uint16_t, KeyAction>> expected = {
      {SC_EQUALS, KeyAction::Press},    {SC_EQUALS, KeyAction::Release},
      {SC_BACKSLASH, KeyAction::Press}, {SC_BACKSLASH, KeyAction::Release},
      {SC_MINUS, KeyAction::Press},     {SC_MINUS, KeyAction::Release},
      {SC_LSHIFT, KeyAction::Press},    {SC_SLASH, KeyAction::Press},
      {SC_SLASH, KeyAction::Release},   {SC_LSHIFT, KeyAction::Release},
      {SC_K, KeyAction::Press},         {SC_K, KeyAction::Release}};
  ASSERT_EQ(hardware.events.size(), expected.size());
  for (size_t index = 0; index < expected.size(); ++index) {
    expectEvent(hardware.events.at(index), expected.at(index).first,
                expected.at(index).second);
  }
}
