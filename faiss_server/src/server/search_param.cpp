#include "server/search_param.h"

std::string ToString(SearchStatus status) {
  switch (status) {
    case SearchStatus::OK: return "ok";
    case SearchStatus::MODEL_NOT_FOUND: return "model not found";
    case SearchStatus::INDEX_NOT_FOUND: return "index not found";
    case SearchStatus::DIM_ERROR: return "dim error";
    case SearchStatus::FAISS_ERROR: return "faiss error";
    default: return "";
  }
}
