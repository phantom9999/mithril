#pragma once

#include <string>
#include <vector>
#include <functional>
#include <utility>

namespace apollo {
class ApiResponse;

using UpdateCallback = std::function<bool(const std::string&, bool, const apollo::ApiResponse&)>;
using ApolloTask = std::pair<std::string, bool>;

class ApolloSource {
 public:
  virtual ~ApolloSource() = default;
  virtual bool Init() = 0;
  virtual void Close() = 0;
  virtual bool GetConfig(const std::vector<ApolloTask>& tasks, const UpdateCallback& callback) = 0;
};
}
