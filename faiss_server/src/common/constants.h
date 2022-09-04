#pragma once

#include <vector>
#include <tuple>
#include "index_constants.pb.h"

constexpr char kSourceFileName[] = "database";
constexpr char kQueryFileName[] = "query";
constexpr char kSourceMetaSuffix[] = ".meta.txt";
constexpr char kSourceFileSuffix[] = ".source.txt";
constexpr char kSourceBinarySuffix[] = ".dat";
constexpr char kFaissIndexSuffix[] = ".index";
constexpr char kFaissIdsName[] = "ids.txt";

// <proto中索引类型名，faiss索引类型,文件后缀>
extern const std::vector<std::pair<proto::Constants::IndexType, std::string>> index_type_names;
