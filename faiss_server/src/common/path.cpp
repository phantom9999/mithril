#include "path.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>
#include <absl/strings/str_join.h>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "common/constants.h"

std::string GenTimePath(const std::string& dir, const std::string& sub_dir) {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d%H%M");
  return absl::StrCat(dir, "/", sub_dir, "/", ss.str());
}

bool MkdirIfNotExist(const std::string& dir) {
  boost::system::error_code ec;
  if (boost::filesystem::exists(dir, ec)) {
    return true;
  }
  return boost::filesystem::create_directories(dir, ec);
}

std::string GetIndexFileName(proto::Constants::IndexType index_type) {
  return absl::StrCat(proto::Constants::IndexType_Name(index_type), kFaissIndexSuffix);
}

