#pragma once

#include "pipeline/IProcessor.h"
#include <gmock/gmock.h>

namespace keyflow::test {

class MockProcessor : public IProcessor {
public:
  MOCK_METHOD(bool, process, (Context & ctx), (override));
};

} // namespace keyflow::test
