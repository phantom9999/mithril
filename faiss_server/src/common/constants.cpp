#include "common/constants.h"

// https://github.com/facebookresearch/faiss/wiki/The-index-factory
// https://github.com/facebookresearch/faiss/wiki/Faiss-indexes
const std::vector<std::pair<proto::Constants::IndexType, std::string>> index_type_names {
  {proto::Constants::DEFAULT, "Flat"},
  {proto::Constants::IVF100, "IVF100,Flat"},
  {proto::Constants::PQ32, "PQ32"},
  {proto::Constants::PCA80_Flat, "PCA80,Flat"},
  {proto::Constants::IVF4096_Flat, "IVF4096,Flat"},
  {proto::Constants::IVF4096_PQ8_16, "IVF4096,PQ8+16"},
  {proto::Constants::IVF4096_PQ32, "IVF4096,PQ32"},
  {proto::Constants::IMI2x8_PQ32, "IMI2x8,PQ32"},
  {proto::Constants::IMI2x8_PQ8_16, "IMI2x8,PQ8+16"},
  {proto::Constants::OPQ16_64_IMI2x8_PQ8_16, "OPQ16+64,IMI2x8,PQ8+16"}
};
