#pragma once

#include <cinttypes>

namespace torch::serving {

extern const char* TORCH_MODEL_FILE;
extern const char* ONNX_MODEL_FILE;
extern const char* FEATURE_SPEC_FILE;
extern const char* CHECK_FILE;
extern const char* MD5_FILE;
extern const char* WARMUP_FILE;

using ModelVersion = uint64_t;
}
