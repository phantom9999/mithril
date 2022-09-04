#include "server_flags.h"

#include <gflags/gflags.h>

DEFINE_uint32(port, 8080, "port");
DEFINE_string(ids_filename, "ids.txt", "name of ids");
DEFINE_string(faiss_filename, "faiss.index", "name of index");
DEFINE_string(index_path, ".", "path of index");
DEFINE_string(server_config, "../conf/server.pb_txt", "");

