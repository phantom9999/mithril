#include "apollo_sink.h"
#include <fcntl.h>
#include <iostream>

#include <absl/strings/str_format.h>
#include <absl/strings/str_split.h>
#include <absl/strings/numbers.h>
#include <absl/cleanup/cleanup.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>


#include <fstream>
#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "apollo_flags.h"
#include "apollo.pb.h"

namespace {
using FileMeta = std::pair<std::string, int32_t>;
const char* LIVE_FILE = "alive_file";

bool GetAllTxtFiles(const std::string &dir, std::vector<boost::filesystem::path> *filelist) {
  if (filelist == nullptr) {
    return false;
  }
  boost::system::error_code ec;
  if (!boost::filesystem::exists(dir, ec)) {
    LOG(WARNING) << dir << " not exist";
    return false;
  }
  using DirIt = boost::filesystem::directory_iterator;
  DirIt dir_begin(dir);
  DirIt dir_end;
  for (; dir_begin != dir_end; ++dir_begin) {
    if (boost::filesystem::is_directory(*dir_begin, ec)) {
      continue;
    }
    auto &path = dir_begin->path();
    if (path.extension() != ".data") {
      continue;
    }
    filelist->push_back(path);
  }
  return true;
}

bool GetMetaFromFilename(const std::string &filename, FileMeta* meta) {
  if (meta == nullptr) {
    return false;
  }
  std::vector<absl::string_view> fields = absl::StrSplit(filename, "_split_");
  if (fields.size() != 2) {
    return false;
  }
  int32_t number = 0;
  if (!absl::SimpleAtoi(fields[1], &number)) {
    return false;
  }
  meta->first = std::string(fields[0]);
  meta->second = number;
  return true;
}

bool ReadAndParse(const std::string& filename, apollo::ApiResponse* response) {
  if (response == nullptr) {
    return false;
  }
  auto file = open(filename.c_str(), O_RDONLY);
  if (file < 0) {
    LOG(WARNING) << "open " << filename << " fail";
    return false;
  }
  absl::Cleanup file_closer = [file](){
    close(file);
  };
  google::protobuf::io::FileInputStream reader{file};
  reader.SetCloseOnDelete(true);
  if (!google::protobuf::TextFormat::Parse(&reader, response)) {
    LOG(WARNING) << "parse " << filename << " error";
    return false;
  }
  return true;
}

std::string GenFilename(const std::string& ns, uint32_t version) {
  return absl::StrFormat("%s/%s_split_%d.data", FLAGS_apollo_sink_dir.c_str(), ns.c_str(), version);
}

bool MkDir(const std::string& dir) {
  boost::system::error_code ec;
  if (boost::filesystem::exists(dir, ec)) {
    if (!boost::filesystem::is_directory(dir, ec)) {
      if (!boost::filesystem::remove(dir, ec)) {
        return false;
      }
    }
    return true;
  }
  return boost::filesystem::create_directories(dir, ec);
}

bool NeedRmAllFile(const std::string& filename) {
  boost::system::error_code ec;
  if (boost::filesystem::exists(filename, ec)) {
    auto last = boost::filesystem::last_write_time(filename, ec);
    if (std::difftime(std::time(nullptr), last) < FLAGS_apollo_sink_timeout) {
      return false;
    }
  }
  return true;
}

}

namespace apollo {

bool ApolloSink::Init() {
  avail_ = true;
  if (!MkDir(FLAGS_apollo_sink_dir)) {
    LOG(WARNING) << "mkdir " << FLAGS_apollo_sink_dir << " error";
    return false;
  }
  auto alive_file = boost::filesystem::path(FLAGS_apollo_sink_dir) / LIVE_FILE;
  alive_file_ = FLAGS_apollo_sink_dir + "/" + LIVE_FILE;

  std::vector<boost::filesystem::path> filelist;
  if (!GetAllTxtFiles(FLAGS_apollo_sink_dir, &filelist)) {
    return false;
  }

  if (NeedRmAllFile(alive_file_)) {
    LOG(INFO) << "dir " << FLAGS_apollo_sink_dir << " outdated";
    // 删除文件
    boost::system::error_code ec;
    for (const auto& file : filelist) {
      boost::filesystem::remove(file, ec);
      LOG(INFO) << "rm " << file.string();
    }
    version_.store(0);
  } else {
    // 计算最大version
    uint32_t max_version = 0;
    for (const auto& file : filelist) {
      FileMeta meta;
      if (!GetMetaFromFilename(file.stem().string(), &meta)) {
        continue;
      }
      if (max_version < meta.second) {
        max_version = meta.second;
      }
    }
    LOG(INFO) << "current max version: " << max_version;
    version_.store(max_version + 1);
  }
  return true;
}
void ApolloSink::Close() {

}
void ApolloSink::KeepALive() {
  boost::system::error_code ec;
  if (!boost::filesystem::exists(alive_file_, ec)) {
    std::ofstream writer(alive_file_);
    if (!writer.is_open()) {
      writer << " ";
    }
    writer.close();
  } else {
    boost::filesystem::last_write_time(alive_file_, std::time(nullptr), ec);
  }
}
bool ApolloSink::GetConfig(const std::vector<std::pair<std::string, bool>> &tasks, const UpdateCallback& callback) {
  if (!avail_) {
    return false;
  }

  std::vector<boost::filesystem::path> filelist;
  if (!GetAllTxtFiles(FLAGS_apollo_sink_dir, &filelist)) {
    return false;
  }
  std::unordered_map<std::string, int32_t> version_map;
  std::unordered_map<std::string, std::unique_ptr<ApiResponse>> res_map;
  for (const auto& file : filelist) {
    FileMeta meta;
    if (!GetMetaFromFilename(file.stem().string(), &meta)) {
      LOG(WARNING) << "get (" << file.stem().string() << ") get meta error";
      continue;
    }
    if (auto it = version_map.find(meta.first); it != version_map.end()) {
      if (it->second > meta.second) {
        continue;
      }
    }
    auto response = std::make_unique<ApiResponse>();
    if (ReadAndParse(file.string(), response.get())) {
      version_map[meta.first] = meta.second;
      res_map[meta.first] = std::move(response);
    }
  }

  return std::all_of(tasks.begin(), tasks.end(), [&](const std::pair<std::string, bool>& entry)->bool {
    auto rit = res_map.find(entry.first);
    auto vit = version_map.find(entry.first);
    if (rit == res_map.end() || vit == version_map.end()) {
      LOG(WARNING) << "namespace: [" << entry.first << "] not found";
      return false;
    }
    std::string filename = GenFilename(vit->first, vit->second);
    LOG(INFO) << "try to resume from " << filename;
    if (!callback(entry.first, entry.second, *rit->second)) {
      LOG(WARNING) << entry.first << " resume from file " << filename << " error";
      return false;
    }
    return true;
  });
}

void ApolloSink::SinkConfig(const std::string &ns, const ApiResponse &response) {
  std::string filename = GenFilename(ns, version_.fetch_add(1));
  std::ofstream writer{filename};
  if (!writer.is_open()) {
    LOG(WARNING) << filename << " open error";
    return;
  }
  writer << response.DebugString();
  writer.close();
  LOG(INFO) << "sink [" << ns << "] to " << filename;
}
}
