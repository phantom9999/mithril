#pragma once

#include <string>
#include "index_constants.pb.h"

std::string GenTimePath(const std::string& dir, const std::string& sub_dir);

bool MkdirIfNotExist(const std::string& dir);

std::string GetIndexFileName(proto::Constants::IndexType index_type);
