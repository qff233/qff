
cmake_minimum_required(VERSION 3.16)
project(qff)

set(CMAKE_VERBOSE_MAKEFILE  ON)

include_directories(./src)

add_compile_options(-std=c++11 -g)

find_package(yaml-cpp REQUIRED)

include_directories(${yaml-cpp_INCLUDE_DIRS})

aux_source_directory(./src src_list)
add_library(qff SHARED ${src_list})
target_link_libraries(qff yaml-cpp pthread)

# add_executable(test_log test/test_log)
# target_link_libraries(test_log qff233)

# add_executable(test_config test/test_config)
# target_link_libraries(test_config qff233)

# add_executable(test_thread test/test_thread)
# target_link_libraries(test_thread qff233)

add_executable(test_fiber test/test_fiber)
target_link_libraries(test_fiber qff)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
