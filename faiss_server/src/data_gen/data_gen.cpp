#include <random>
#include <fstream>
#include <sstream>

#include <gflags/gflags.h>
/*#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>*/

#include <absl/strings/str_cat.h>

#include "source.pb.h"
#include "common/constants.h"

DEFINE_string(output_dir, ".", "");
DEFINE_uint32(dim, 128, "");
DEFINE_uint32(data_size, 200000, "");
DEFINE_uint32(query_size, 10000, "");


/*void MakeDataByStream(const std::string& path) {
  std::mt19937 rng{std::random_device{}()};
  std::uniform_real_distribution<> distrib;

  std::ofstream file_writer{path, std::ios::binary};
  google::protobuf::io::OstreamOutputStream output_stream(&file_writer);
  google::protobuf::io::CodedOutputStream coded_output_stream(&output_stream);

  proto::SourceMeta meta;
  meta.set_dim(FLAGS_dim);
  meta.set_length(FLAGS_data_size);
  auto meta_buffer = meta.SerializeAsString();

  coded_output_stream.WriteVarint64(meta.ByteSizeLong());
  meta.SerializeToCodedStream(&coded_output_stream);
  size_t total{0};
  for (int i=0;i<FLAGS_data_size;++i) {
    proto::Source source;
    source.set_label("label_" + std::to_string(i));
    for (int j=0;j<FLAGS_dim;++j) {
      source.add_vec(distrib(rng));
    }
    total += source.ByteSizeLong();
    coded_output_stream.WriteVarint64(source.ByteSizeLong());
    if (!source.SerializeToCodedStream(&coded_output_stream)) {
      std::cout << "write error " << source.DebugString();
      break;
    }
  }
  std::cout << "write size " << total << std::endl;
  file_writer.close();
}*/

void MakeData(const std::string& path) {
  std::mt19937 rng{std::random_device{}()};
  std::uniform_real_distribution<> distrib;
  std::ofstream writer{absl::StrCat(FLAGS_output_dir, "/", kSourceFileName, kSourceBinarySuffix), std::ios::binary};
  {
    proto::SourceMeta meta;
    meta.set_dim(FLAGS_dim);
    meta.set_length(FLAGS_data_size);
    auto buffer = meta.SerializeAsString();
    size_t len = buffer.size();
    writer.write((char *) &len, sizeof(uint64_t));
    writer.write(buffer.c_str(), len);
  }
  for (int i = 0; i < FLAGS_data_size; ++i) {
    proto::Source source;
    source.set_label("label_" + std::to_string(i));
    for (int j = 0; j < FLAGS_dim; ++j) {
      source.add_vec(distrib(rng));
    }
    std::string buffer = source.SerializeAsString();
    uint64_t len = buffer.size();
    writer.write((char *) &len, sizeof(uint64_t));
    writer.write(buffer.c_str(), len);
  }
}

void MakeTextData() {
  std::mt19937 rng{std::random_device{}()};
  std::uniform_real_distribution<> distrib;
  {
    std::ofstream writer{absl::StrCat(FLAGS_output_dir, "/", kQueryFileName, kSourceFileSuffix)};
    for (int i=0;i<FLAGS_query_size;++i) {
      proto::Source source;
      source.set_label("query_" + std::to_string(i));
      for (int j=0;j<FLAGS_dim;++j) {
        source.add_vec(distrib(rng));
      }
      writer << source.ShortDebugString() << "\n";
    }
    writer.close();
  }
  {
    std::ofstream writer{absl::StrCat(FLAGS_output_dir, "/", kQueryFileName, kSourceMetaSuffix)};
    proto::SourceMeta meta;
    meta.set_dim(FLAGS_dim);
    meta.set_length(FLAGS_query_size);
    writer << meta.ShortDebugString() << "\n";
    writer.close();
  }
}


int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  MakeData(absl::StrCat(FLAGS_output_dir, "/", kSourceFileName, kSourceBinarySuffix));

  MakeTextData();
  return 0;
}
