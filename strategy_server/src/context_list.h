#pragma once

#include <cstdint>
#include <memory>

struct ContextA {
  uint64_t logid;
};

using ContextAPtr = std::shared_ptr<ContextA>;

struct ContextB {
  uint64_t logid;
};

using ContextBPtr = std::shared_ptr<ContextB>;

struct ContextC {
  uint64_t logid;
};

using ContextCPtr = std::shared_ptr<ContextC>;
