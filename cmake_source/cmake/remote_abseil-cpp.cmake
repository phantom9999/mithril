include(ExternalProject)
set(ABSEIL_VERSION 20211102.0)
set(ABSEIL_URL URL https://github.com/abseil/abseil-cpp/archive/refs/tags/${ABSEIL_VERSION}.tar.gz)

ExternalProject_Add(abseil-cpp
        PREFIX abseil-cpp
        URL ${ABSEIL_URL}
        CMAKE_CACHE_ARGS
        -DCARES_SHARED:BOOL=OFF
        -DCARES_STATIC:BOOL=ON
        -DCARES_STATIC_PIC:BOOL=ON
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/abseil-cpp
        )