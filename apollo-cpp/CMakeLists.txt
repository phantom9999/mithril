cmake_minimum_required(VERSION 3.10)
project(apollo-cpp)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(leveldb REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(absl REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

if (NOT APPLE)
    find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
    find_library(BRPC_LIB NAMES brpc)
    if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
        message(FATAL_ERROR "Fail to find brpc")
    endif()
    include_directories(${BRPC_INCLUDE_PATH})
endif()

include_directories(${Boost_INCLUDE_DIRS})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/apollo.proto)
list(GET PROTO_HDRS 0 PB_FILE)
get_filename_component(PB_FOLDER ${PB_FILE} DIRECTORY)
include_directories(${PB_FOLDER})

include_directories(src)
aux_source_directory(src/apollo SRC)

set(LIB_TARGET apollo-cpp)

add_library(${LIB_TARGET} ${SRC} ${PROTO_SRCS})
target_link_libraries(
        ${LIB_TARGET}
        Boost::filesystem
        Boost::system
        absl::str_format
        brpc
        protobuf::libprotobuf
        glog::glog
        leveldb::leveldb
        gflags_static
        OpenSSL::SSL
        ZLIB::ZLIB
        Threads::Threads
        dl
)
target_include_directories(${LIB_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
        )

set(APOLLO_CPP_HEADER
        src/apollo/apollo_all.h
        src/apollo/apollo_define.h
        src/apollo/apollo_deliver_service.h
        src/apollo/apollo_global.h
        src/apollo/apollo_source.h
        src/apollo/apollo_config.h
        src/apollo/apollo_interface.h
        src/apollo/apollo_sink.h
        ${PROTO_HDRS}
        )

set_target_properties(apollo-cpp PROPERTIES PUBLIC_HEADER "${APOLLO_CPP_HEADER}")

set(EXPORT_NAME ${LIB_TARGET}-targets)

install(TARGETS ${LIB_TARGET}
        EXPORT ${EXPORT_NAME}
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/apollo
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(EXPORT ${EXPORT_NAME} DESTINATION lib/cmake/${LIB_TARGET})

include(CMakePackageConfigHelpers)
configure_package_config_file(apollo-cpp.cmake.in
        ${CMAKE_BINARY_DIR}/${LIB_TARGET}-config.cmake
        INSTALL_DESTINATION lib/cmake/${LIB_TARGET})
write_basic_package_version_file(
        ${CMAKE_BINARY_DIR}/${LIB_TARGET}-config-version.cmake
        VERSION 1.0.0
        COMPATIBILITY AnyNewerVersion)

install(FILES
        ${CMAKE_BINARY_DIR}/${LIB_TARGET}-config.cmake
        ${CMAKE_BINARY_DIR}/${LIB_TARGET}-config-version.cmake
        DESTINATION lib/cmake/${LIB_TARGET})


protobuf_generate_cpp(EXAMPLE_PROTO_SRCS EXAMPLE_PROTO_HDRS example/data.proto)
list(GET EXAMPLE_PROTO_HDRS 0 EXAMPLE_PB_FILE)
get_filename_component(EXAMPLE_PB_FOLDER ${EXAMPLE_PB_FILE} DIRECTORY)
include_directories(${EXAMPLE_PB_FOLDER})

aux_source_directory(example EXAMPLE_SRC)

add_executable(apollo_example ${EXAMPLE_SRC} ${EXAMPLE_PROTO_SRCS})
target_link_libraries(
        apollo_example
        ${LIB_TARGET}
)
