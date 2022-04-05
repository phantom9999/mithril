#include "config_manager.h"
#include <fcntl.h>
#include <gflags/gflags.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <glog/logging.h>

DEFINE_string(suffix, "conf", "ths suffix of config");
DEFINE_string(env, "prod", "test, pre, prod");
DEFINE_validator(env, [](const char* flagsname, const std::string& value)->bool {
    return value == "test" || value == "pre" || value == "prod";
});

namespace {

void getConfig(const std::string &path, google::protobuf::Message *msg) {
  int file = open(path.c_str(), O_RDONLY);
  if (file < 0) {
    LOG(WARNING) << "open " << path << " fail";
    return;
  }
  google::protobuf::io::FileInputStream reader{file};
  reader.SetCloseOnDelete(true);
  if (!google::protobuf::TextFormat::Parse(&reader, msg)) {
    LOG(WARNING) << "parse " << path << " error";
  }
  DLOG(INFO) << path << " get config: " << msg->ShortDebugString();
}

#define CASE_REPEATED_FIELD_TYPE(cpptype, method) \
  case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype: { \
    rf->Add##method(dst, field, rf->GetRepeated##method(*src, field, j)); \
    break;                                                   \
}

#define CASE_FIELD_TYPE(cpptype, method) \
  case google::protobuf::FieldDescriptor::CPPTYPE_##cpptype: { \
    rf->Set##method(dst, field, rf->Get##method(*src, field)); \
    break;                                          \
}


/**
 * src -> dst
 * @param src
 * @param dst
 */
void mergeConfig(google::protobuf::Message *src, google::protobuf::Message *dst) {
  if (src == nullptr || dst == nullptr) {
    return;
  }
  if (src->GetDescriptor() != dst->GetDescriptor()) {
    return;
  }
  auto* rf = dst->GetReflection();
  auto* descr = dst->GetDescriptor();

  int field_size = descr->field_count();
  for (int i=0;i<field_size;++i) {
    auto* field = descr->field(i);
    auto& field_name = field->name();
    if (field->is_repeated()) {
      // repeated
      int dst_size = rf->FieldSize(*dst, field);
      int src_size = rf->FieldSize(*src, field);
      if (dst_size != 0 || src_size == 0) {
        // 目标不为0, 或者源为0, 不进行合并
        continue;
      }
      for (int j=0;j<src_size;++j) {
        switch (field->cpp_type()) {
          CASE_REPEATED_FIELD_TYPE(INT32, Int32)
          CASE_REPEATED_FIELD_TYPE(UINT32, UInt32)
          CASE_REPEATED_FIELD_TYPE(INT64, Int64)
          CASE_REPEATED_FIELD_TYPE(UINT64, UInt64)
          CASE_REPEATED_FIELD_TYPE(FLOAT, Float)
          CASE_REPEATED_FIELD_TYPE(DOUBLE, Double)
          CASE_REPEATED_FIELD_TYPE(BOOL, Bool)
          CASE_REPEATED_FIELD_TYPE(STRING, String)
          case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
            rf->AddEnumValue(dst, field, rf->GetRepeatedEnumValue(*src, field, j));
            break;
          }
          case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
            rf->AddMessage(dst, field)->CopyFrom(rf->GetRepeatedMessage(*src, field, j));
            break;
          }
        }
      }
    } else {
      bool src_has_field = rf->HasField(*src, field);
      bool dst_has_field = rf->HasField(*dst, field);
      if (src_has_field && dst_has_field && field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        // 合并
        auto* dst_sub = rf->MutableMessage(dst, field);
        auto* src_sub = rf->MutableMessage(src, field);
        mergeConfig(src_sub, dst_sub);
        continue;
      } else if (dst_has_field || !src_has_field) {
        // 目标已经赋值; 源没有赋值
        continue;
      }
      switch (field->cpp_type()) {
        CASE_FIELD_TYPE(INT32, Int32)
        CASE_FIELD_TYPE(UINT32, UInt32)
        CASE_FIELD_TYPE(INT64, Int64)
        CASE_FIELD_TYPE(UINT64, UInt64)
        CASE_FIELD_TYPE(FLOAT, Float)
        CASE_FIELD_TYPE(DOUBLE, Double)
        CASE_FIELD_TYPE(BOOL, Bool)
        CASE_FIELD_TYPE(STRING, String)
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
          rf->SetEnumValue(dst, field, rf->GetEnumValue(*src, field));
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
          rf->MutableMessage(dst, field)->CopyFrom(rf->GetMessage(*src, field));
          break;
        }
      }
    }
  }
}

}

bool ConfigManager::parse(const std::string &path, google::protobuf::Message *message) {
    if (message == nullptr) {
        LOG(WARNING) << "nullptr";
        return false;
    }

    std::string baseconf = path + "." + FLAGS_suffix;
    DLOG(INFO) << "base conf " << baseconf;
    std::string customconf = path + "-" + FLAGS_env + "." + FLAGS_suffix;
    DLOG(INFO) << "custom conf " << customconf;
    message->Clear();
    auto* base = message->New();
    getConfig(customconf, message);
    getConfig(baseconf, base);
    mergeConfig(base, message);
    DLOG(INFO) << "final config " << message->ShortDebugString();
    delete base;
    return true;
}
