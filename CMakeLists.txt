cmake_minimum_required(VERSION 3.10)
project(linko)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    linko/address.cc
    linko/bytearray.cc
    linko/socket.cc
    linko/config.cc
    linko/fiber.cc
    linko/http/http.cc
    linko/http/http_parser.cc
    linko/http/http11_parser.rl.cc
    linko/http/httpclient_parser.rl.cc
    linko/http/http_session.cc
    linko/http/http_connection.cc
    linko/http/http_server.cc
    linko/http/servlet.cc
    linko/hook.cc
    linko/log.cc
    linko/mutex.cc
    linko/fd_manager.cc
    linko/scheduler.cc
    linko/socket.cc
    linko/stream.cc
    linko/socket_stream.cc
    linko/iomanager.cc
    linko/thread.cc
    linko/timer.cc
    linko/tcp_server.cc
    linko/util.cc
    )

ragelmaker(linko/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/linko/http)
ragelmaker(linko/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/linko/http)
ragelmaker(linko/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/linko)

add_library(linko SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(linko)

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
        linko
        dl
        pthread
        yaml-cpp
    )

add_executable(test tests/test.cc)
add_dependencies(test linko)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config linko)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread linko)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util linko)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber linko)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler linko)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager linko)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook linko)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address linko)
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIBS})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket linko)
force_redefine_file_macro_for_sources(test_socket)
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray linko)
force_redefine_file_macro_for_sources(test_bytearray)
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http linko)
force_redefine_file_macro_for_sources(test_http)
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser linko)
force_redefine_file_macro_for_sources(test_http_parser)
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server linko)
force_redefine_file_macro_for_sources(test_tcp_server)
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server linko)
force_redefine_file_macro_for_sources(echo_server)
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server tests/test_http_server.cc)
add_dependencies(test_http_server linko)
force_redefine_file_macro_for_sources(test_http_server)
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_connection tests/test_http_connection.cc)
add_dependencies(test_http_connection linko)
force_redefine_file_macro_for_sources(test_http_connection)
target_link_libraries(test_http_connection ${LIBS})

add_executable(test_uri tests/test_uri.cc)
add_dependencies(test_uri linko)
force_redefine_file_macro_for_sources(test_uri)
target_link_libraries(test_uri ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
