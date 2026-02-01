#pragma once

#include "Context.h"

namespace keyflow {

/**
 * @brief Interface for all pipeline processors
 *
 * Each processor implements a single responsibility:
 * - Filter by device
 * - Detect taps
 * - Remap keys
 * - Match combos
 *
 * Design:
 * - Single virtual method for minimal overhead
 * - Processes context in-place (zero-copy)
 * - Returns true to continue pipeline, false to stop
 *
 * Example:
 * ```cpp
 * class MyProcessor : public IProcessor {
 * public:
 *     bool process(Context& ctx) override {
 *         if (ctx.scancode == 0x3A) {  // CapsLock
 *             ctx.outputScancode = 0x1D;  // Map to LeftCtrl
 *             ctx.action = Action::Replace;
 *         }
 *         return true;  // Continue to next processor
 *     }
 * };
 * ```
 */
class IProcessor {
  public:
    virtual ~IProcessor() = default;

    /**
     * @brief Process a keystroke context
     * @param ctx Mutable context to read/modify
     * @return true to continue pipeline, false to stop immediately
     */
    [[nodiscard]] virtual bool process(Context& ctx) = 0;

    /**
     * @brief Get processor name for debugging/logging
     */
    [[nodiscard]] virtual const char* name() const noexcept { return "IProcessor"; }
};

} // namespace keyflow
