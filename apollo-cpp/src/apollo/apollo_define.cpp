#include "apollo_define.h"

namespace apollo {
Register::Register(const std::string& ns, const std::string& key, std::unique_ptr<Processor> process_base) {
  NamespaceSet::process_map[ns][key] = std::move(process_base);
}

OtherRegister::OtherRegister(const std::string &ns) {
  NamespaceSet::data_map[ns].store(nullptr);
}
}
