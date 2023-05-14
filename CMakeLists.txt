cmake_minimum_required(VERSION 3.10)
project(link)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    link/util.cc
    link/log.cc
    )

add_library(link SHARED ${LIB_SRC})

add_executable(test tests/test.cc)
add_dependencies(test link)
target_link_libraries(test link)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
