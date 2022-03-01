set(GRPC_VERSION v1.44.0)
set(GRPC_URL https://github.com/grpc/grpc/archive/refs/tags/${GRPC_VERSION}.tar.gz)

ExternalProject_Add(grpc
        PREFIX grpc
        URL ${GRPC_URL}
        DEPENDS c-ares protobuf zlib abseil-cpp
        CMAKE_ARGS
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN:BOOL=OFF
        CMAKE_CACHE_ARGS
        -DgRPC_INSTALL:BOOL=ON
        -DgRPC_BUILD_TESTS:BOOL=OFF
        -DgRPC_PROTOBUF_PROVIDER:STRING=package
        -DgRPC_PROTOBUF_PACKAGE_TYPE:STRING=CONFIG
        -DProtobuf_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib64/cmake/protobuf
        -DgRPC_ZLIB_PROVIDER:STRING=package
        -DZLIB_ROOT:STRING=${CMAKE_CURRENT_BINARY_DIR}/zlib
        -DgRPC_CARES_PROVIDER:STRING=package
        -Dc-ares_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}/c-ares/lib/cmake/c-ares
        -DgRPC_SSL_PROVIDER:STRING=package
        -DgRPC_ABSL_PROVIDER:STRING=package
        -Dabsl_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}/abseil-cpp/lib64/cmake/absl
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/grpc
        )