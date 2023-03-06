#pragma once

#include <gflags/gflags_declare.h>

DECLARE_uint32(apollo_interval);
DECLARE_bool(apollo_use_deliver);
DECLARE_bool(apollo_use_sink);

DECLARE_string(apollo_server);
DECLARE_string(apollo_app);
DECLARE_string(apollo_cluster);
DECLARE_string(apollo_tokens);
DECLARE_uint32(apollo_timeout);

DECLARE_string(apollo_sink_dir);
DECLARE_uint32(apollo_sink_timeout);

DECLARE_string(apollo_deliver_server);
