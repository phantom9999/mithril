#!/bin/bash

set -x

source /opt/dev.sh;
vcpkg install gflags glog;
cat /opt/vcpkg-2022.02.23/buildtrees/detect_compiler/config-x64-linux-rel-err.log
yum install -y tree;
cd /opt/vcpkg-2022.02.23;
tree -h




