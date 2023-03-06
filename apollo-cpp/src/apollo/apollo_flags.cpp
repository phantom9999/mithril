#include "apollo_flags.h"

#include <gflags/gflags.h>

DEFINE_uint32(apollo_interval, 10, "update interval");
DEFINE_bool(apollo_use_deliver, false, "");
DEFINE_bool(apollo_use_sink, true, "");

DEFINE_string(apollo_server, "", "the server of apollo");
DEFINE_string(apollo_app, "", "the app id");
DEFINE_string(apollo_cluster, "", "the cluster");
DEFINE_string(apollo_tokens, "", "the tokens of api");
DEFINE_uint32(apollo_timeout, 2000, "time out of api");

DEFINE_string(apollo_sink_dir, "/tmp/apollo_dump", "the dir of dump");
DEFINE_uint32(apollo_sink_timeout, 24*60*60, "file timeout");

DEFINE_string(apollo_deliver_server, "", "the server of deliver server");
