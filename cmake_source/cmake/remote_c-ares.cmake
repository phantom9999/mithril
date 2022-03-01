include(ExternalProject)
set(CARES_VERSION 1_18_1)
set(CARES_URL URL https://github.com/c-ares/c-ares/archive/refs/tags/cares-${CARES_VERSION}.tar.gz)

ExternalProject_Add(c-ares
        PREFIX c-ares
        URL ${CARES_URL}
        CMAKE_CACHE_ARGS
        -DCARES_SHARED:BOOL=OFF
        -DCARES_STATIC:BOOL=ON
        -DCARES_STATIC_PIC:BOOL=ON
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/c-ares
        )
