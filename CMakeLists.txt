cmake_minimum_required(VERSION 3.10)
project(link)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    link/config.cc
    link/fiber.cc
    link/log.cc
    link/thread.cc
    link/util.cc
    )

add_library(link SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(link)

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
    link
    pthread
    yaml-cpp
    )

add_executable(test tests/test.cc)
add_dependencies(test link)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config link)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread link)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util link)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber link)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
