#include "fixtures/MockProcessor.h"
#include "hardware/KeyEvent.h"
#include "hardware/Scancodes.h"
#include "pipeline/Pipeline.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace keyflow;
using namespace keyflow::test;
using ::testing::_;
using ::testing::Return;

// Empty pipeline must forward the event unchanged — boundary behavior.
// Without this assertion a degenerate config (no processors) would silently
// fail or crash.
TEST(PipelineTest, EmptyPipelineForwards) {
  Pipeline pipeline;
  KeyEvent event(SC_A, true);

  ProcessingResult result = pipeline.process(event);

  EXPECT_EQ(result.action, Action::Forward);
  EXPECT_EQ(result.outputScancode, SC_A);
}

// Pipeline must honor a processor's stop-the-chain return value. If this
// invariant breaks, ConfigBuilder's "blocker consumes the trigger key but
// later processors must not run" assumption silently fails downstream.
TEST(PipelineTest, FirstProcessorStopsChain) {
  Pipeline pipeline;
  auto mock1 = std::make_unique<MockProcessor>();
  auto mock2 = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mock1, process(_)).WillOnce(Return(false));
  EXPECT_CALL(*mock2, process(_)).Times(0);

  pipeline.addProcessor(std::move(mock1));
  pipeline.addProcessor(std::move(mock2));

  KeyEvent event(SC_A, true);
  [[maybe_unused]] auto result = pipeline.process(event);
}
