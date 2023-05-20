cmake_minimum_required(VERSION 3.22)
project(taskflow_on_fiber)

set(CMAKE_CXX_STANDARD 17)


find_package(Boost COMPONENTS fiber log REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)

aux_source_directory(src FLOW_SRC)

add_executable(taskflow_engine ${FLOW_SRC})
target_link_libraries(
        taskflow_engine
        ${Boost_LIBRARIES}
)
