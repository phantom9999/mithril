#include "apollo_header.h"

#include "data.pb.h"
#include "apollo/apollo_config.h"

APOLLO_DEFINE(Prop1, Key1, example::Msg1)
APOLLO_DEFINE(Prop1, Key2, example::Msg2)
APOLLO_DEFINE(Prop2, Key3, example::Msg3)
APOLLO_DEFINE_JSON(Other1)
