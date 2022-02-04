
set -x
WORKSPACE=$(cd `dirname $0`;pwd)
cd ${WORKSPACE}

#protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` helloworld.proto
#protoc --cpp_out=. helloworld.proto

PROTOBUF_HOME="/home/work/.conan/data/protobuf/3.17.1/_/_/package/f45ead7d52ff58ecb8744a1f730cb62a47f8ae8b"
PROTOC_BIN=${PROTOBUF_HOME}/bin/protoc

GRPC_HOME="/home/work/.conan/data/grpc/1.43.0/_/_/package/f6ea8ca86e85feb0bdd7021b3ba0eace81ebdaff"
GRPC_PLUGIN=${GRPC_HOME}/bin/grpc_cpp_plugin

cd ../proto;

${PROTOC_BIN} --grpc_out=. --plugin=protoc-gen-grpc="${GRPC_PLUGIN}" service.proto
${PROTOC_BIN} --cpp_out=. service.proto





