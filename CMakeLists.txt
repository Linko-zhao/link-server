cmake_minimum_required(VERSION 3.10)
project(link)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
link_libraries(/usr/local/lib)

find_library(YAMLCPP yaml-cpp)

set(LIB_SRC
    link/util.cc
    link/log.cc
    link/config.cc
    )

add_library(link SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(link)

add_executable(test tests/test.cc)
add_dependencies(test link)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test link ${YAMLCPP})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config link)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config link ${YAMLCPP})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
