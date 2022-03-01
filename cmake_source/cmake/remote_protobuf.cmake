include(ExternalProject)
set(PROTOBUF_VERSION v3.6.1)
set(PROTOBUF_URL https://github.com/google/protobuf/archive/${PROTOBUF_VERSION}.tar.gz)


ExternalProject_Add(protobuf
        PREFIX protobuf
        URL ${PROTOBUF_URL}
        SOURCE_SUBDIR cmake
        DEPENDS zlib
        CMAKE_CACHE_ARGS
        -Dprotobuf_BUILD_TESTS:BOOL=OFF
        -Dprotobuf_WITH_ZLIB:BOOL=ON
        -Dprotobuf_BUILD_SHARED_LIBS:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/protobuf
        )
set(protobuf_ROOT ${CMAKE_CURRENT_BINARY_DIR}/protobuf)

