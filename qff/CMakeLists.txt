cmake_minimum_required(VERSION 3.16)
project(qff)
include(../cmake/utils.cmake)

add_compile_options(-std=c++17  -ggdb -Wall -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations)

find_package(yaml-cpp REQUIRED)
include_directories(${yaml-cpp_INCLUDE_DIRS})

aux_source_directory(. src_list)
aux_source_directory(./http http_src_list)

ragelmaker(./http/http11_parser.rl src_list ${CMAKE_CURRENT_SOURCE_DIR}/http)
ragelmaker(./http/httpclient_parser.rl src_list ${CMAKE_CURRENT_SOURCE_DIR}/http)

add_library(qff SHARED ${src_list} ${http_src_list})

target_link_libraries(qff yaml-cpp pthread dl)
