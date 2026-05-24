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

// ===== ProcessingResult Tests =====

TEST(ProcessingResultTest, Construction) {
  ProcessingResult result(Action::Forward, SC_A, 0);

  EXPECT_EQ(result.action, Action::Forward);
  EXPECT_EQ(result.outputScancode, SC_A);
  EXPECT_EQ(result.modifiers, 0u);
  EXPECT_FALSE(result.injectShift);
}

TEST(ProcessingResultTest, ConstructionWithShift) {
  ProcessingResult result(Action::Replace, SC_B, 0x01, true);

  EXPECT_EQ(result.action, Action::Replace);
  EXPECT_EQ(result.outputScancode, SC_B);
  EXPECT_EQ(result.modifiers, 0x01u);
  EXPECT_TRUE(result.injectShift);
}

TEST(ProcessingResultTest, ShouldForward) {
  ProcessingResult forward(Action::Forward, SC_A, 0);
  ProcessingResult replace(Action::Replace, SC_B, 0);
  ProcessingResult consume(Action::Consume, 0, 0);

  EXPECT_TRUE(forward.shouldForward());
  EXPECT_FALSE(replace.shouldForward());
  EXPECT_FALSE(consume.shouldForward());
}

TEST(ProcessingResultTest, ShouldReplace) {
  ProcessingResult forward(Action::Forward, SC_A, 0);
  ProcessingResult replace(Action::Replace, SC_B, 0);
  ProcessingResult consume(Action::Consume, 0, 0);

  EXPECT_FALSE(forward.shouldReplace());
  EXPECT_TRUE(replace.shouldReplace());
  EXPECT_FALSE(consume.shouldReplace());
}

TEST(ProcessingResultTest, ShouldConsume) {
  ProcessingResult forward(Action::Forward, SC_A, 0);
  ProcessingResult replace(Action::Replace, SC_B, 0);
  ProcessingResult consume(Action::Consume, 0, 0);

  EXPECT_FALSE(forward.shouldConsume());
  EXPECT_FALSE(replace.shouldConsume());
  EXPECT_TRUE(consume.shouldConsume());
}

TEST(ProcessingResultTest, Constexpr) {
  constexpr ProcessingResult result(Action::Forward, SC_A, 0);
  static_assert(result.action == Action::Forward,
                "Constexpr construction failed");
}

// ===== Pipeline Basic Tests =====

TEST(PipelineTest, DefaultConstruction) {
  Pipeline pipeline;
  EXPECT_EQ(pipeline.processorCount(), 0u);
}

TEST(PipelineTest, EmptyPipelineForwards) {
  Pipeline pipeline;
  KeyEvent event(SC_A, true);

  ProcessingResult result = pipeline.process(event);

  EXPECT_EQ(result.action, Action::Forward);
  EXPECT_EQ(result.outputScancode, SC_A);
}

TEST(PipelineTest, AddSingleProcessor) {
  Pipeline pipeline;
  pipeline.addProcessor(std::make_unique<MockProcessor>());

  EXPECT_EQ(pipeline.processorCount(), 1u);
}

TEST(PipelineTest, AddMultipleProcessors) {
  Pipeline pipeline;
  pipeline.addProcessor(std::make_unique<MockProcessor>());
  pipeline.addProcessor(std::make_unique<MockProcessor>());
  pipeline.addProcessor(std::make_unique<MockProcessor>());

  EXPECT_EQ(pipeline.processorCount(), 3u);
}

TEST(PipelineTest, ClearProcessors) {
  Pipeline pipeline;
  pipeline.addProcessor(std::make_unique<MockProcessor>());
  pipeline.addProcessor(std::make_unique<MockProcessor>());

  EXPECT_EQ(pipeline.processorCount(), 2u);

  pipeline.clear();
  EXPECT_EQ(pipeline.processorCount(), 0u);
}

// ===== Single Processor Tests =====

TEST(PipelineProcessingTest, SingleProcessorContinues) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_)).WillOnce(Return(true));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_A, true);
  ProcessingResult result = pipeline.process(event);

  // Default action is Forward
  EXPECT_EQ(result.action, Action::Forward);
}

TEST(PipelineProcessingTest, SingleProcessorStops) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_)).WillOnce(Return(false));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_A, true);
  ProcessingResult result = pipeline.process(event);

  // Even if stopped, result is returned
  EXPECT_EQ(result.action, Action::Forward);
}

// ===== Multi-Processor Chain Tests =====

TEST(PipelineProcessingTest, TwoProcessorsBothContinue) {
  Pipeline pipeline;
  auto mock1 = std::make_unique<MockProcessor>();
  auto mock2 = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mock1, process(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock2, process(_)).WillOnce(Return(true));

  pipeline.addProcessor(std::move(mock1));
  pipeline.addProcessor(std::move(mock2));

  KeyEvent event(SC_A, true);
  [[maybe_unused]] auto result = pipeline.process(event);
}

TEST(PipelineProcessingTest, FirstProcessorStopsChain) {
  Pipeline pipeline;
  auto mock1 = std::make_unique<MockProcessor>();
  auto mock2 = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mock1, process(_)).WillOnce(Return(false));
  EXPECT_CALL(*mock2, process(_)).Times(0); // Should not be called

  pipeline.addProcessor(std::move(mock1));
  pipeline.addProcessor(std::move(mock2));

  KeyEvent event(SC_A, true);
  [[maybe_unused]] auto result = pipeline.process(event);
}

TEST(PipelineProcessingTest, SecondProcessorStopsChain) {
  Pipeline pipeline;
  auto mock1 = std::make_unique<MockProcessor>();
  auto mock2 = std::make_unique<MockProcessor>();
  auto mock3 = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mock1, process(_)).WillOnce(Return(true));
  EXPECT_CALL(*mock2, process(_)).WillOnce(Return(false));
  EXPECT_CALL(*mock3, process(_)).Times(0); // Should not be called

  pipeline.addProcessor(std::move(mock1));
  pipeline.addProcessor(std::move(mock2));
  pipeline.addProcessor(std::move(mock3));

  KeyEvent event(SC_A, true);
  [[maybe_unused]] auto result = pipeline.process(event);
}

// ===== Context Initialization Tests =====

TEST(PipelineProcessingTest, ContextInitializedWithKeyDown) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .WillOnce(::testing::Invoke([](const Context &ctx) {
        EXPECT_EQ(ctx.scancode, SC_SPACE);
        EXPECT_TRUE(ctx.isDown);
        EXPECT_EQ(ctx.outputScancode, SC_SPACE);
        EXPECT_EQ(ctx.action, Action::Forward);
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_SPACE, true);
  [[maybe_unused]] auto result = pipeline.process(event);
}

TEST(PipelineProcessingTest, ContextInitializedWithKeyUp) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .WillOnce(::testing::Invoke([](const Context &ctx) {
        EXPECT_EQ(ctx.scancode, SC_ENTER);
        EXPECT_FALSE(ctx.isDown);
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_ENTER, false);
  [[maybe_unused]] auto result = pipeline.process(event);
}

// ===== Action Propagation Tests =====

TEST(PipelineProcessingTest, ProcessorSetsReplace) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .WillOnce(::testing::Invoke([](Context &ctx) {
        ctx.action = Action::Replace;
        ctx.outputScancode = SC_B;
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_A, true);
  ProcessingResult result = pipeline.process(event);

  EXPECT_EQ(result.action, Action::Replace);
  EXPECT_EQ(result.outputScancode, SC_B);
}

TEST(PipelineProcessingTest, ProcessorSetsConsume) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .WillOnce(::testing::Invoke([](Context &ctx) {
        ctx.action = Action::Consume;
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_A, true);
  ProcessingResult result = pipeline.process(event);

  EXPECT_EQ(result.action, Action::Consume);
}

TEST(PipelineProcessingTest, ProcessorSetsModifiers) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .WillOnce(::testing::Invoke([](Context &ctx) {
        ctx.modifiers = 0x0F;
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_A, true);
  ProcessingResult result = pipeline.process(event);

  EXPECT_EQ(result.modifiers, 0x0Fu);
}

TEST(PipelineProcessingTest, ProcessorSetsShiftInjection) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .WillOnce(::testing::Invoke([](Context &ctx) {
        ctx.injectShift = true;
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event(SC_A, true);
  ProcessingResult result = pipeline.process(event);

  EXPECT_TRUE(result.injectShift);
}

// ===== Context Reuse Tests =====

TEST(PipelineProcessingTest, ContextReusedBetweenCalls) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_)).Times(2).WillRepeatedly(Return(true));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event1(SC_A, true);
  [[maybe_unused]] auto result1 = pipeline.process(event1);

  KeyEvent event2(SC_B, true);
  ProcessingResult result = pipeline.process(event2);

  // Second event should have fresh context
  EXPECT_EQ(result.outputScancode, SC_B);
}

// ===== Multiple Events Tests =====

TEST(PipelineProcessingTest, MultipleEventsProcessedIndependently) {
  Pipeline pipeline;
  auto mockProcessor = std::make_unique<MockProcessor>();

  EXPECT_CALL(*mockProcessor, process(_))
      .Times(3)
      .WillRepeatedly(::testing::Invoke([](Context &ctx) {
        if (ctx.scancode == SC_A) {
          ctx.action = Action::Replace;
          ctx.outputScancode = SC_B;
        }
        return true;
      }));

  pipeline.addProcessor(std::move(mockProcessor));

  KeyEvent event1(SC_A, true);
  ProcessingResult result1 = pipeline.process(event1);
  EXPECT_EQ(result1.action, Action::Replace);
  EXPECT_EQ(result1.outputScancode, SC_B);

  KeyEvent event2(SC_C, true);
  ProcessingResult result2 = pipeline.process(event2);
  EXPECT_EQ(result2.action, Action::Forward);

  KeyEvent event3(SC_A, false);
  ProcessingResult result3 = pipeline.process(event3);
  EXPECT_EQ(result3.action, Action::Replace);
}
