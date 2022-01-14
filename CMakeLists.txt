cmake_minimum_required(VERSION 3.16)
project(qff233)

set(CMAKE_VERBOSE_MAKEFILE  ON)

include_directories(./src)

add_compile_options(-std=c++17 -g)

aux_source_directory(./src src_list)
add_library(qff233 SHARED ${src_list})


add_executable(test_log test/test_log)
target_link_libraries(test_log qff233)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

