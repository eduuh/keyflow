#pragma once

#include "../hardware/KeyEvent.h"
#include "Context.h"
#include "IProcessor.h"

#include <memory>
#include <vector>

namespace keyflow {

/**
 * @brief Result of pipeline processing
 */
struct ProcessingResult {
    Action action;           // What to do with keystroke
    uint16_t outputScancode; // Output scancode (if Replace)
    uint32_t modifiers;      // Active modifiers during processing
    bool injectShift;        // Inject Shift modifier with output

    ProcessingResult(Action a, uint16_t sc, uint32_t mods, bool shift = false) noexcept
        : action(a), outputScancode(sc), modifiers(mods), injectShift(shift) {}
};

/**
 * @brief Pipeline orchestrator
 *
 * Runs processors in sequence until one stops the chain.
 *
 * Responsibilities:
 * - Initialize context from KeyEvent
 * - Run each processor in order
 * - Stop if processor returns false
 * - Return final result
 *
 * Target: ~50 lines of implementation
 */
class Pipeline {
  public:
    /**
     * @brief Add a processor to the end of the pipeline
     */
    void addProcessor(std::unique_ptr<IProcessor> processor) {
        processors_.push_back(std::move(processor));
    }

    /**
     * @brief Process a key event through the pipeline
     * @param event Hardware keystroke
     * @return Processing result (action + output scancode)
     */
    [[nodiscard]] ProcessingResult process(const KeyEvent& event) noexcept {
        // Initialize context
        context_.initialize(event);

        // Run processors in sequence
        for (auto& processor : processors_) {
            if (!processor->process(context_)) {
                break; // Processor stopped the chain
            }
        }

        // Return result (including modifiers and shift injection for debugging)
        return ProcessingResult(context_.action, context_.outputScancode, context_.modifiers,
                                context_.injectShift);
    }

    /**
     * @brief Get number of processors in pipeline
     */
    [[nodiscard]] size_t processorCount() const noexcept { return processors_.size(); }

    /**
     * @brief Clear all processors
     */
    void clear() noexcept { processors_.clear(); }

  private:
    std::vector<std::unique_ptr<IProcessor>> processors_;
    Context context_; // Reused for each keystroke (avoid allocations)
};

} // namespace keyflow
