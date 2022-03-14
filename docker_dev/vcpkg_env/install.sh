#!/bin/bash

set -x

source /opt/dev.sh;
vcpkg install --clean-after-build gflags glog folly grpc boost fmt;
#cd /opt/vcpkg_home;
#rm -rf buildtrees/*
#rm -f downloads/*.gz
