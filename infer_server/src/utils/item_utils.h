#pragma once

#include <cstddef>

namespace inference {
class ModelInferRequest;
}

namespace torch::serving {

size_t CountItem(const inference::ModelInferRequest& request);

}

