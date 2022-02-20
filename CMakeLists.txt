
cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE  ON)

add_subdirectory(qff)
include_directories(qff)

add_compile_options(-std=c++17  -ggdb -Wall -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations)
# add_executable(test_log test/test_log)
# target_link_libraries(test_log qff233)

# add_executable(test_config test/test_config)
# target_link_libraries(test_config qff233)

# add_executable(test_thread test/test_thread)
# target_link_libraries(test_thread qff233)

#add_executable(test_fiber test/test_fiber)
#target_link_libraries(test_fiber qff)

add_executable(test_iomanager test/test_iomanager)
target_link_libraries(test_iomanager qff)

add_executable(test_socket test/test_socket)
target_link_libraries(test_socket qff)

add_executable(test_http test/test_http)
target_link_libraries(test_http qff)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

